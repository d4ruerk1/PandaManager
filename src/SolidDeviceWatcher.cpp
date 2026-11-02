// SPDX-License-Identifier: Apache-2.0
/**
 * SolidDeviceWatcher.cpp
 * KDE Solid hot-plug integration for PandaManager.
 *
 * Checkpoint 6 — KDE Solid for USB hot-plug and device notifications.
 *
 * Key Solid classes used:
 *   Solid::DeviceNotifier          — hot-plug signals (added/removed)
 *   Solid::Device                  — wraps any hardware object
 *   Solid::StorageDrive            — physical drive metadata
 *   Solid::StorageVolume           — partition/volume metadata
 *   Solid::StorageAccess           — mount/unmount access
 *   Solid::Device::listFromType()  — enumerate all storage volumes
 */
#include "SolidDeviceWatcher.hpp"

#include <Solid/DeviceNotifier>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/DeviceInterface>

#include <QDebug>

// ---------------------------------------------------------------------------
// Constructor — subscribe to Solid::DeviceNotifier
// ---------------------------------------------------------------------------
SolidDeviceWatcher::SolidDeviceWatcher(QObject* parent) : QObject(parent)
{
    Solid::DeviceNotifier* notifier = Solid::DeviceNotifier::instance();

    connect(notifier, &Solid::DeviceNotifier::deviceAdded,
            this,     &SolidDeviceWatcher::onDeviceAdded);
    connect(notifier, &Solid::DeviceNotifier::deviceRemoved,
            this,     &SolidDeviceWatcher::onDeviceRemoved);

    qDebug() << "SolidDeviceWatcher: listening for hot-plug events";
}

// ---------------------------------------------------------------------------
// solidDeviceToStorageDevice — map Solid device to our struct
// ---------------------------------------------------------------------------
StorageDevice SolidDeviceWatcher::solidDeviceToStorageDevice(const Solid::Device& device)
{
    StorageDevice sd;

    // StorageAccess — mount point and accessibility
    const auto* access = device.as<Solid::StorageAccess>();
    if (access) {
        sd.mounted    = access->isAccessible();
        sd.mountPoint = access->filePath();
    }

    // StorageVolume — filesystem type, label, capacity
    const auto* vol = device.as<Solid::StorageVolume>();
    if (vol) {
        sd.filesystem  = vol->fsType();
        sd.label       = vol->label();
        sd.totalBytes  = static_cast<qint64>(vol->size());
    }

    // StorageDrive — removable flag; walk up parent chain to find it
    Solid::Device d = device;
    while (d.isValid()) {
        const auto* drv = d.as<Solid::StorageDrive>();
        if (drv) {
            sd.removable = drv->isHotpluggable() || drv->isRemovable();
            break;
        }
        d = Solid::Device(d.parentUdi());
    }

    // Best-effort: derive /dev/<x> from UDI  e.g. /org/freedesktop/Hal/devices/…
    // UDisks2 UDI looks like /org/freedesktop/UDisks2/block_devices/sdb1
    const QString udi = device.udi();
    const int idx = udi.lastIndexOf(QLatin1Char('/'));
    if (idx >= 0)
        sd.devPath = QLatin1String("/dev/") + udi.mid(idx + 1);

    return sd;
}

// ---------------------------------------------------------------------------
// listRemovableDevices
// ---------------------------------------------------------------------------
QList<StorageDevice> SolidDeviceWatcher::listRemovableDevices() const
{
    QList<StorageDevice> result;

    // Enumerate all storage volumes
    const QList<Solid::Device> devices =
        Solid::Device::listFromType(Solid::DeviceInterface::StorageVolume);

    for (const Solid::Device& device : devices) {
        const auto* vol = device.as<Solid::StorageVolume>();
        if (!vol) continue;
        // Skip ignored volumes (swap, unformatted, etc.)
        if (vol->usage() == Solid::StorageVolume::Unused ||
            vol->usage() == Solid::StorageVolume::PartitionTable) continue;

        StorageDevice sd = solidDeviceToStorageDevice(device);
        if (!sd.devPath.isEmpty())
            result.append(sd);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Hot-plug slots
// ---------------------------------------------------------------------------
void SolidDeviceWatcher::onDeviceAdded(const QString& udi)
{
    Solid::Device device(udi);

    // We only care about storage volumes
    if (!device.is<Solid::StorageVolume>()) return;

    StorageDevice sd = solidDeviceToStorageDevice(device);
    if (sd.devPath.isEmpty()) return;

    qDebug() << "SolidDeviceWatcher: device added:" << sd.devPath
             << "label:" << sd.label << "fs:" << sd.filesystem;

    emit deviceAdded(sd);
}

void SolidDeviceWatcher::onDeviceRemoved(const QString& udi)
{
    // Derive /dev/<x> from the UDI (same logic as solidDeviceToStorageDevice)
    // so that deviceRemoved carries a consistent devPath, not a raw UDI string.
    QString devPath;
    const int idx = udi.lastIndexOf(QLatin1Char('/'));
    if (idx >= 0)
        devPath = QLatin1String("/dev/") + udi.mid(idx + 1);

    qDebug() << "SolidDeviceWatcher: device removed:" << udi << "→" << devPath;
    emit deviceRemoved(devPath.isEmpty() ? udi : devPath);
}
