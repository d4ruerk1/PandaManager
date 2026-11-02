// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "platform/PlatformServices.hpp"

#ifdef PM_DEVICE_BACKEND_UDISKS2

#include <QObject>
#include <QString>
#include "DeviceManager.hpp"

namespace Solid { class Device; }

class SolidDeviceWatcher : public QObject {
    Q_OBJECT
public:
    explicit SolidDeviceWatcher(QObject* parent = nullptr);
    QList<StorageDevice> listRemovableDevices() const;

signals:
    void deviceAdded(const StorageDevice& dev);
    void deviceRemoved(const QString& udi);

private slots:
    void onDeviceAdded(const QString& udi);
    void onDeviceRemoved(const QString& udi);

private:
    static StorageDevice solidDeviceToStorageDevice(const Solid::Device& device);
};

#endif // PM_DEVICE_BACKEND_UDISKS2
