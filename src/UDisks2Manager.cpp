// SPDX-License-Identifier: Apache-2.0
/**
 * UDisks2Manager.cpp
 * Qt D-Bus binding for the org.freedesktop.UDisks2 interface.
 *
 * Checkpoint 6 — UDisks2 via D-Bus (real device enumeration, mount, unmount,
 * format with Polkit authorization).
 */
#include "UDisks2Manager.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QVariant>
#include <QVariantMap>
#include <QDebug>

// ---------------------------------------------------------------------------
// D-Bus type aliases used by UDisks2
// ---------------------------------------------------------------------------
// GetManagedObjects returns  a{oa{sa{sv}}}
// QMap<QDBusObjectPath, QMap<QString, QVariantMap>>
using IfaceMap    = QMap<QString, QVariantMap>;          // a{sa{sv}}
using ObjectMap   = QMap<QDBusObjectPath, IfaceMap>;     // a{oa{sa{sv}}}

Q_DECLARE_METATYPE(IfaceMap)
Q_DECLARE_METATYPE(ObjectMap)

static void registerMetaTypes()
{
    static bool done = false;
    if (done) return;
    done = true;
    qDBusRegisterMetaType<IfaceMap>();
    qDBusRegisterMetaType<ObjectMap>();
}

// ---------------------------------------------------------------------------
// Static safety lists (mirror DeviceManager constants)
// ---------------------------------------------------------------------------
const QStringList UDisks2Manager::BLOCKED_DEVICES    = {"/dev/sda", "/dev/nvme0n1", "/dev/mmcblk0"};
const QStringList UDisks2Manager::ALLOWED_FILESYSTEMS = {"btrfs", "ext4", "exfat", "ntfs", "vfat"};

bool UDisks2Manager::isRootDevice(const QString& devPath)
{
    for (const auto& blocked : BLOCKED_DEVICES) {
        // Block the whole device AND any partition on it
        if (devPath == blocked || devPath.startsWith(blocked))
            return true;
    }
    return false;
}

bool UDisks2Manager::isFilesystemAllowed(const QString& fs)
{
    return ALLOWED_FILESYSTEMS.contains(fs.toLower());
}

// ---------------------------------------------------------------------------
// Constructor — connect UDisks2 signal for hot-plug
// ---------------------------------------------------------------------------
UDisks2Manager::UDisks2Manager(QObject* parent) : QObject(parent)
{
    registerMetaTypes();

    // Connect to UDisks2 ObjectManager signals for hot-plug notifications
    QDBusConnection sys = QDBusConnection::systemBus();
    sys.connect(
        QLatin1String(SERVICE),
        QLatin1String("/org/freedesktop/UDisks2"),
        QLatin1String("org.freedesktop.DBus.ObjectManager"),
        QLatin1String("InterfacesAdded"),
        this,
        SLOT(onInterfacesAdded(QDBusObjectPath, QVariantMap)));

    sys.connect(
        QLatin1String(SERVICE),
        QLatin1String("/org/freedesktop/UDisks2"),
        QLatin1String("org.freedesktop.DBus.ObjectManager"),
        QLatin1String("InterfacesRemoved"),
        this,
        SLOT(onInterfacesRemoved(QDBusObjectPath, QStringList)));
}

// ---------------------------------------------------------------------------
// Helper: extract /dev/<x> from Block.Device byte-array property
// ---------------------------------------------------------------------------
static QString devicePropertyToPath(const QVariant& v)
{
    // UDisks2 returns Device as ay (byte array) in D-Bus
    const auto bytes = v.toByteArray();
    if (!bytes.isEmpty())
        return QString::fromLocal8Bit(bytes).trimmed().remove(QChar('\0'));
    return {};
}

// ---------------------------------------------------------------------------
// buildDevice — fill StorageDevice from iface map
// ---------------------------------------------------------------------------
StorageDevice UDisks2Manager::buildDevice(const QDBusObjectPath& /*objPath*/,
                                          const IfaceMap& allIfaces)
{
    StorageDevice dev;

    // Block interface
    const QVariantMap blk = allIfaces.value(QLatin1String(IFACE_BLK));
    dev.devPath   = devicePropertyToPath(blk.value(QLatin1String("Device")));
    dev.label     = blk.value(QLatin1String("IdLabel")).toString();
    dev.filesystem= blk.value(QLatin1String("IdType")).toString();
    dev.totalBytes= blk.value(QLatin1String("Size")).toLongLong();

    // Filesystem interface (may be absent for unformatted devices)
    const QVariantMap fs = allIfaces.value(QLatin1String(IFACE_FS));
    if (!fs.isEmpty()) {
        const auto mps = fs.value(QLatin1String("MountPoints"));
        // MountPoints is aay — list of byte-arrays
        const QDBusArgument arg = mps.value<QDBusArgument>();
        QList<QByteArray> mountPoints;
        arg.beginArray();
        while (!arg.atEnd()) {
            QByteArray mp;
            arg >> mp;
            mountPoints << mp;
        }
        arg.endArray();
        if (!mountPoints.isEmpty()) {
            dev.mountPoint = QString::fromLocal8Bit(mountPoints.first()).trimmed().remove(QChar('\0'));
            dev.mounted    = true;
        }
    }

    // Drive interface (removable flag)
    const QVariantMap drv = allIfaces.value(QLatin1String(IFACE_DRV));
    dev.removable = drv.value(QLatin1String("Removable"), false).toBool();

    return dev;
}

// ---------------------------------------------------------------------------
// objectPathToDevPath — resolve via D-Bus Property
// ---------------------------------------------------------------------------
QString UDisks2Manager::objectPathToDevPath(const QDBusObjectPath& objPath)
{
    QDBusInterface blkIface(QLatin1String(SERVICE), objPath.path(),
                            QLatin1String("org.freedesktop.DBus.Properties"),
                            QDBusConnection::systemBus());
    QDBusReply<QVariant> reply = blkIface.call(
        QLatin1String("Get"),
        QLatin1String(IFACE_BLK),
        QLatin1String("Device"));
    if (reply.isValid())
        return devicePropertyToPath(reply.value());
    return {};
}

// ---------------------------------------------------------------------------
// devPathToObjectPath — scan managed objects to find matching block device
// ---------------------------------------------------------------------------
QDBusObjectPath UDisks2Manager::devPathToObjectPath(const QString& devPath) const
{
    QDBusInterface objMgr(QLatin1String(SERVICE),
                          QLatin1String("/org/freedesktop/UDisks2"),
                          QLatin1String("org.freedesktop.DBus.ObjectManager"),
                          QDBusConnection::systemBus());
    QDBusReply<ObjectMap> reply = objMgr.call(QLatin1String("GetManagedObjects"));
    if (!reply.isValid()) return {};

    for (auto it = reply.value().cbegin(); it != reply.value().cend(); ++it) {
        const QVariantMap blk = it.value().value(QLatin1String(IFACE_BLK));
        if (blk.isEmpty()) continue;
        if (devicePropertyToPath(blk.value(QLatin1String("Device"))) == devPath)
            return it.key();
    }
    return {};
}

// ---------------------------------------------------------------------------
// listDevices
// ---------------------------------------------------------------------------
QList<StorageDevice> UDisks2Manager::listDevices() const
{
    registerMetaTypes();
    QList<StorageDevice> result;

    QDBusInterface objMgr(QLatin1String(SERVICE),
                          QLatin1String("/org/freedesktop/UDisks2"),
                          QLatin1String("org.freedesktop.DBus.ObjectManager"),
                          QDBusConnection::systemBus());
    if (!objMgr.isValid()) {
        qWarning() << "UDisks2Manager: cannot connect to UDisks2 on system bus";
        return result;
    }

    QDBusReply<ObjectMap> reply = objMgr.call(QLatin1String("GetManagedObjects"));
    if (!reply.isValid()) {
        qWarning() << "UDisks2Manager: GetManagedObjects failed:" << reply.error().message();
        return result;
    }

    for (auto it = reply.value().cbegin(); it != reply.value().cend(); ++it) {
        const IfaceMap& ifaces = it.value();

        // We only want block devices (must have Block interface)
        if (!ifaces.contains(QLatin1String(IFACE_BLK))) continue;

        // Skip loop devices
        const QString path = it.key().path();
        if (path.contains(QLatin1String("loop"))) continue;

        // Build a flat QVariantMap from iface map for buildDevice()
        QVariantMap flat;
        for (auto jt = ifaces.cbegin(); jt != ifaces.cend(); ++jt)
            flat.insert(jt.key(), QVariant::fromValue(jt.value()));

        StorageDevice dev = buildDevice(it.key(), flat);
        if (dev.devPath.isEmpty()) continue;
        if (isRootDevice(dev.devPath)) continue;   // safety: skip root disk

        result.append(dev);
    }

    return result;
}

// ---------------------------------------------------------------------------
// mount
// ---------------------------------------------------------------------------
bool UDisks2Manager::mount(const QString& devPath, QString* errorOut)
{
    if (isRootDevice(devPath)) {
        if (errorOut) *errorOut = QStringLiteral("Blocked: root device");
        return false;
    }

    const QDBusObjectPath objPath = devPathToObjectPath(devPath);
    if (objPath.path().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Device not found in UDisks2");
        return false;
    }

    QDBusInterface fsIface(QLatin1String(SERVICE), objPath.path(),
                           QLatin1String(IFACE_FS),
                           QDBusConnection::systemBus());
    // Mount options: empty map → defaults (uid=current user, auto-detect FS)
    QVariantMap opts;
    QDBusReply<QString> reply = fsIface.call(QLatin1String("Mount"), QVariant::fromValue(opts));
    if (!reply.isValid()) {
        if (errorOut) *errorOut = reply.error().message();
        qWarning() << "UDisks2Manager: mount failed:" << reply.error().message();
        return false;
    }
    qDebug() << "UDisks2Manager: mounted" << devPath << "at" << reply.value();
    return true;
}

// ---------------------------------------------------------------------------
// unmount
// ---------------------------------------------------------------------------
bool UDisks2Manager::unmount(const QString& devPath, QString* errorOut)
{
    if (isRootDevice(devPath)) {
        if (errorOut) *errorOut = QStringLiteral("Blocked: root device");
        return false;
    }

    const QDBusObjectPath objPath = devPathToObjectPath(devPath);
    if (objPath.path().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Device not found in UDisks2");
        return false;
    }

    QDBusInterface fsIface(QLatin1String(SERVICE), objPath.path(),
                           QLatin1String(IFACE_FS),
                           QDBusConnection::systemBus());
    QVariantMap opts;
    QDBusReply<void> reply = fsIface.call(QLatin1String("Unmount"), QVariant::fromValue(opts));
    if (!reply.isValid()) {
        if (errorOut) *errorOut = reply.error().message();
        qWarning() << "UDisks2Manager: unmount failed:" << reply.error().message();
        return false;
    }
    qDebug() << "UDisks2Manager: unmounted" << devPath;
    return true;
}

// ---------------------------------------------------------------------------
// format — Polkit auth_admin triggered automatically by UDisks2
// ---------------------------------------------------------------------------
bool UDisks2Manager::format(const QString& devPath, const QString& filesystem,
                             QString* errorOut)
{
    if (isRootDevice(devPath)) {
        if (errorOut) *errorOut = QStringLiteral("Blocked: root device");
        return false;
    }
    if (!isFilesystemAllowed(filesystem)) {
        if (errorOut) *errorOut = QStringLiteral("Filesystem not in whitelist: ") + filesystem;
        return false;
    }

    const QDBusObjectPath objPath = devPathToObjectPath(devPath);
    if (objPath.path().isEmpty()) {
        if (errorOut) *errorOut = QStringLiteral("Device not found in UDisks2");
        return false;
    }

    QDBusInterface blkIface(QLatin1String(SERVICE), objPath.path(),
                            QLatin1String(IFACE_BLK),
                            QDBusConnection::systemBus());
    // Format options: auth_no_user_interaction=false lets Polkit agent prompt
    QVariantMap opts;
    opts[QLatin1String("auth_no_user_interaction")] = QVariant(false);

    // Use a long timeout: formatting can take minutes
    blkIface.setTimeout(300000); // 5 minutes
    QDBusReply<void> reply = blkIface.call(
        QLatin1String("Format"),
        filesystem.toLower(),
        QVariant::fromValue(opts));

    if (!reply.isValid()) {
        if (errorOut) *errorOut = reply.error().message();
        qWarning() << "UDisks2Manager: format failed:" << reply.error().message();
        return false;
    }
    qDebug() << "UDisks2Manager: formatted" << devPath << "as" << filesystem;
    return true;
}

// ---------------------------------------------------------------------------
// Hot-plug slots
// ---------------------------------------------------------------------------
void UDisks2Manager::onInterfacesAdded(const QDBusObjectPath& path,
                                        const QVariantMap& interfaces)
{
    if (!interfaces.contains(QLatin1String(IFACE_BLK))) return;
    if (path.path().contains(QLatin1String("loop"))) return;

    StorageDevice dev = buildDevice(path, interfaces);
    if (dev.devPath.isEmpty() || isRootDevice(dev.devPath)) return;

    qDebug() << "UDisks2Manager: device added:" << dev.devPath;
    emit deviceAdded(dev);
}

void UDisks2Manager::onInterfacesRemoved(const QDBusObjectPath& path,
                                          const QStringList& interfaces)
{
    if (!interfaces.contains(QLatin1String(IFACE_BLK))) return;

    const QString devPath = objectPathToDevPath(path);
    if (devPath.isEmpty()) return;

    qDebug() << "UDisks2Manager: device removed:" << devPath;
    emit deviceRemoved(devPath);
}
