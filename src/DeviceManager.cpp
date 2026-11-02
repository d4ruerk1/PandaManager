// SPDX-License-Identifier: Apache-2.0
/**
 * DeviceManager.cpp
 *
 * Gestione dispositivi removibili cross-platform.
 *
 * Linux   → UDisks2Manager (D-Bus) + SolidDeviceWatcher (KDE Solid)
 * macOS   → DiskArbitrationWatcher (startDeviceWatcher / stopDeviceWatcher)
 * Windows → WMIDeviceWatcher      (startDeviceWatcher / stopDeviceWatcher)
 *
 * Regole di sicurezza:
 *   - I dispositivi radice (/, /dev/sda …) non possono essere montati,
 *     smontati o formattati.
 *   - Prima di segnalare la rimozione di un dispositivo montato, viene
 *     tentato uno smontaggio sicuro per evitare perdite di dati.
 */

#include "DeviceManager.hpp"
#include "platform/PlatformServices.hpp"

#include <QDebug>

#ifdef Q_OS_LINUX
#  include "UDisks2Manager.hpp"
#  include "SolidDeviceWatcher.hpp"
#endif

#ifdef Q_OS_MACOS
// Declarations from DiskArbitrationWatcher.mm
namespace PandaManager {
    bool startDeviceWatcher(std::function<void(const StorageDevice&)> onAdded,
                            std::function<void(const QString&)>       onRemoved);
    void stopDeviceWatcher();
    QList<StorageDevice> listRemovableDevicesMacOS();
}
#endif

#ifdef Q_OS_WIN
// Declarations from WMIDeviceWatcher.cpp
namespace PandaManager {
    bool startDeviceWatcher(std::function<void(const StorageDevice&)> onAdded,
                            std::function<void(const QString&)>       onRemoved);
    void stopDeviceWatcher();
    QList<StorageDevice> listRemovableDevicesWindows();
}
#endif

// ── Safety blocklists ─────────────────────────────────────────────────────────
const QStringList DeviceManager::BLOCKED_DEVICES     = {
    "/dev/sda", "/dev/nvme0n1", "/dev/mmcblk0",
    // macOS internal disk names
    "disk0", "/dev/disk0",
    // Windows system drives (C:)
    "C:", "\\\\.\\PhysicalDrive0"
};
const QStringList DeviceManager::ALLOWED_FILESYSTEMS = {
    "btrfs", "ext4", "exfat", "ntfs", "vfat", "apfs", "hfs+", "fat32"
};

// ── Constructor ───────────────────────────────────────────────────────────────
DeviceManager::DeviceManager(QObject* parent) : QObject(parent)
{
#ifdef Q_OS_LINUX
    m_udisks2 = new UDisks2Manager(this);
    m_solid   = new SolidDeviceWatcher(this);

    connect(m_udisks2, &UDisks2Manager::deviceAdded,
            this, &DeviceManager::onDeviceAdded);
    connect(m_udisks2, &UDisks2Manager::deviceRemoved,
            this, &DeviceManager::onDeviceRemoved);
    connect(m_solid, &SolidDeviceWatcher::deviceAdded,
            this, &DeviceManager::onDeviceAdded);
    connect(m_solid, &SolidDeviceWatcher::deviceRemoved,
            this, &DeviceManager::onDeviceRemoved);

#elif defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    // DiskArbitration / WMI callbacks are invoked from their respective run-loops /
    // threads. Use queued lambdas so Qt signals are always emitted on the Qt main thread.
    PandaManager::startDeviceWatcher(
        [this](const StorageDevice& dev) {
            QMetaObject::invokeMethod(this, [this, dev] {
                onDeviceAdded(dev);
            }, Qt::QueuedConnection);
        },
        [this](const QString& devPath) {
            QMetaObject::invokeMethod(this, [this, devPath] {
                onDeviceRemoved(devPath);
            }, Qt::QueuedConnection);
        });
#endif
}

// ── Destructor ────────────────────────────────────────────────────────────────
DeviceManager::~DeviceManager()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    PandaManager::stopDeviceWatcher();
#endif
}

// ── onDeviceAdded / onDeviceRemoved ───────────────────────────────────────────
void DeviceManager::onDeviceAdded(const StorageDevice& dev)
{
    if (isRootDevice(dev.devPath)) return; // safety: ignore root devices
    qDebug() << "[DeviceManager] Device connected:" << dev.devPath
             << "(" << dev.label << ")";
    emit deviceAdded(dev);
}

void DeviceManager::onDeviceRemoved(const QString& devPath)
{
    if (isRootDevice(devPath)) return;
    qDebug() << "[DeviceManager] Device disconnected:" << devPath;

    // Best-effort safe unmount before signalling removal.
    // This prevents "device busy" errors and potential data loss when the user
    // physically unplugs without ejecting first.
#ifdef Q_OS_LINUX
    if (m_udisks2) {
        QString err;
        m_udisks2->unmount(devPath, &err);
        if (!err.isEmpty())
            qDebug() << "[DeviceManager] Pre-removal unmount:" << err;
    }
#elif defined(Q_OS_MACOS)
    // DiskArbitration calls diskDisappeared after the OS has already
    // unmounted; no manual unmount is needed.
#elif defined(Q_OS_WIN)
    // WM_DEVICECHANGE is fired after Windows has already performed safe removal.
#endif

    emit deviceRemoved(devPath);
}

// ── Safety helpers ────────────────────────────────────────────────────────────
bool DeviceManager::isRootDevice(const QString& devPath)
{
    for (const auto& blocked : BLOCKED_DEVICES) {
        if (devPath == blocked || devPath.startsWith(blocked))
            return true;
    }
    return false;
}

bool DeviceManager::isFilesystemAllowed(const QString& fs)
{
    return ALLOWED_FILESYSTEMS.contains(fs.toLower());
}

// ── listDevices ───────────────────────────────────────────────────────────────
QList<StorageDevice> DeviceManager::listDevices() const
{
#ifdef Q_OS_LINUX
    QList<StorageDevice> devices = m_udisks2->listDevices();
    const QList<StorageDevice> solidDevs = m_solid->listRemovableDevices();
    QSet<QString> known;
    for (const auto& d : std::as_const(devices))
        known.insert(d.devPath);
    for (const auto& d : solidDevs)
        if (!known.contains(d.devPath))
            devices.append(d);
    return devices;
#elif defined(Q_OS_MACOS)
    return PandaManager::listRemovableDevicesMacOS();
#elif defined(Q_OS_WIN)
    return PandaManager::listRemovableDevicesWindows();
#else
    return {};
#endif
}

// ── mount / unmount / format ──────────────────────────────────────────────────
bool DeviceManager::mount(const QString& devPath)
{
    if (isRootDevice(devPath)) return false;
#ifdef Q_OS_LINUX
    QString err;
    const bool ok = m_udisks2->mount(devPath, &err);
    if (!ok) qWarning() << "DeviceManager::mount failed:" << err;
    return ok;
#else
    qWarning() << "DeviceManager::mount not implemented on this platform";
    return false;
#endif
}

bool DeviceManager::unmount(const QString& devPath)
{
    if (isRootDevice(devPath)) return false;
#ifdef Q_OS_LINUX
    QString err;
    const bool ok = m_udisks2->unmount(devPath, &err);
    if (!ok) qWarning() << "DeviceManager::unmount failed:" << err;
    return ok;
#else
    qWarning() << "DeviceManager::unmount not implemented on this platform";
    return false;
#endif
}

bool DeviceManager::format(const QString& devPath, const QString& filesystem)
{
    if (isRootDevice(devPath)) return false;
    if (!isFilesystemAllowed(filesystem)) return false;
#ifdef Q_OS_LINUX
    QString err;
    const bool ok = m_udisks2->format(devPath, filesystem, &err);
    if (!ok) qWarning() << "DeviceManager::format failed:" << err;
    return ok;
#else
    qWarning() << "DeviceManager::format not implemented on this platform";
    return false;
#endif
}
