// SPDX-License-Identifier: Apache-2.0
// Stub: no-op device watcher — device hot-plug is not supported on this platform.
// On macOS use DiskArbitrationWatcher; on Windows use WMIDeviceWatcher.
#include "DeviceManager.hpp"
#include <QDebug>

namespace PandaManager {

QList<StorageDevice> listRemovableDevicesStub()
{
    qDebug() << "[DeviceWatcher] Stub: device enumeration not available on this platform.";
    return {};
}

} // namespace PandaManager
