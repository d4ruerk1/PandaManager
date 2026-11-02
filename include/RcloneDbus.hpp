// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "platform/PlatformServices.hpp"

#ifdef HAVE_KF6
// Full rclone-rcd JSON-RPC over HTTP (all platforms with Qt Network)
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <functional>

class QNetworkAccessManager;

class RcloneDbus : public QObject {
    Q_OBJECT
public:
    explicit RcloneDbus(QObject* parent = nullptr);
    ~RcloneDbus() override;

    /** Start rclone rcd if not already running. Returns true on success. */
    bool startDaemon(const QString& rcUser = QStringLiteral("pandamanager"),
                     const QString& rcPass = QString());

    /** Ask the rclone daemon to quit gracefully. */
    void stopDaemon();

    /** Send a no-op ping to keep the rclone session alive. */
    void noop();

    /** List configured remotes (async — result via remotesList signal). */
    void listRemotes();

    /** Mount @p remote at @p mountPoint with @p options. */
    void mountRemote(const QString& remote, const QString& mountPoint,
                     const QVariantMap& options = {});

    /** Unmount @p mountPoint. */
    void unmountRemote(const QString& mountPoint);

    /** List active mounts (async — result via mountsList signal). */
    void listMounts();

signals:
    void mountDone(const QString& mountPoint);
    void unmountDone(const QString& mountPoint);
    void mountsList(const QVariantMap& mounts);
    void remotesList(const QStringList& remotes);
    void rpcError(const QString& method, const QString& error);

private:
    void postRpc(const QString& method, const QVariantMap& params,
                 std::function<void(const QVariantMap&)> onSuccess);

    QNetworkAccessManager* m_nam    = nullptr;
    QString                m_rcUser;
    QString                m_rcPass;
    quint16                m_port   = 5572;
};

#else
// On platforms without D-Bus, rclone is launched as a QProcess (RcloneProcess.cpp)
namespace PandaManager {
bool startRcloneRcd(const QString& configPath, quint16 port, const QString& password);
} // namespace PandaManager
#endif // HAVE_KF6
