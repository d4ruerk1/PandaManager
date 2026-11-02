// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <functional>

#ifdef Q_OS_LINUX
class UDisks2Manager;
class SolidDeviceWatcher;
#endif

/**
 * @brief Represents a physical or virtual storage device.
 */
struct StorageDevice {
    QString devPath;
    QString label;
    QString filesystem;
    QString mountPoint;
    bool    removable   = false;
    bool    mounted     = false;
    qint64  totalBytes  = 0;
};

/**
 * @brief DeviceManager — cross-platform high-level device API.
 *
 * Delegates to:
 *   - Linux:   UDisks2Manager (D-Bus) + SolidDeviceWatcher (KDE Solid)
 *   - macOS:   DiskArbitrationWatcher (callbacks → Qt signals)
 *   - Windows: WMIDeviceWatcher + WM_DEVICECHANGE (callbacks → Qt signals)
 *
 * Safety: root/system devices are always blocked from mount/unmount/format.
 * On physical disconnect, a best-effort unmount is attempted before the
 * deviceRemoved() signal is emitted to prevent data loss.
 */
class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(QObject* parent = nullptr);
    ~DeviceManager() override;

    QList<StorageDevice> listDevices() const;
    bool mount(const QString& devPath);
    bool unmount(const QString& devPath);
    bool format(const QString& devPath, const QString& filesystem);

    static bool isRootDevice(const QString& devPath);
    static bool isFilesystemAllowed(const QString& fs);

signals:
    void deviceAdded(const StorageDevice& dev);
    void deviceRemoved(const QString& devPath);

private slots:
    void onDeviceAdded(const StorageDevice& dev);
    void onDeviceRemoved(const QString& devPath);

private:
    static const QStringList BLOCKED_DEVICES;
    static const QStringList ALLOWED_FILESYSTEMS;

#ifdef Q_OS_LINUX
    UDisks2Manager*     m_udisks2 = nullptr;
    SolidDeviceWatcher* m_solid   = nullptr;
#endif
};
