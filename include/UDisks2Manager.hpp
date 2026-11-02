// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "platform/PlatformServices.hpp"

#ifdef PM_DEVICE_BACKEND_UDISKS2

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDBusObjectPath>
#include <QVariantMap>
#include "DeviceManager.hpp"

using IfaceMap = QMap<QString, QVariantMap>;

class UDisks2Manager : public QObject {
    Q_OBJECT
public:
    static constexpr const char* SERVICE   = "org.freedesktop.UDisks2";
    static constexpr const char* OBJ_MGMT  = "/org/freedesktop/UDisks2/Manager";
    static constexpr const char* IFACE_MGR = "org.freedesktop.UDisks2.Manager";
    static constexpr const char* IFACE_BLK = "org.freedesktop.UDisks2.Block";
    static constexpr const char* IFACE_FS  = "org.freedesktop.UDisks2.Filesystem";
    static constexpr const char* IFACE_DRV = "org.freedesktop.UDisks2.Drive";
    static constexpr const char* IFACE_OBJ = "org.freedesktop.DBus.ObjectManager";

    explicit UDisks2Manager(QObject* parent = nullptr);

    QList<StorageDevice> listDevices() const;
    bool mount(const QString& devPath, QString* errorOut = nullptr);
    bool unmount(const QString& devPath, QString* errorOut = nullptr);
    bool format(const QString& devPath, const QString& filesystem,
                QString* errorOut = nullptr);

signals:
    void deviceAdded(const StorageDevice& dev);
    void deviceRemoved(const QString& devPath);

private slots:
    void onInterfacesAdded(const QDBusObjectPath& path, const QVariantMap& interfaces);
    void onInterfacesRemoved(const QDBusObjectPath& path, const QStringList& interfaces);

private:
    static QString objectPathToDevPath(const QDBusObjectPath& objPath);
    static StorageDevice buildDevice(const QDBusObjectPath& objPath, const IfaceMap& allIfaces);
    QDBusObjectPath devPathToObjectPath(const QString& devPath) const;

    static const QStringList BLOCKED_DEVICES;
    static const QStringList ALLOWED_FILESYSTEMS;
    static bool isRootDevice(const QString& devPath);
    static bool isFilesystemAllowed(const QString& fs);
};

#endif // PM_DEVICE_BACKEND_UDISKS2
