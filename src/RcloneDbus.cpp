// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file RcloneDbus.cpp
 * @brief Implementazione binding rclone rcd JSON-RPC via Qt Network.
 *
 * Dipendenze CMake:
 *   find_package(Qt6 REQUIRED COMPONENTS Network)
 *   target_link_libraries(pandamanager PRIVATE Qt6::Network)
 */

#include "RcloneDbus.hpp"
#include "KWalletManager.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QEventLoop>
#include <QDebug>

static constexpr int kRpcTimeoutMs = 5000;

RcloneDbus::RcloneDbus(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

RcloneDbus::~RcloneDbus() = default;

// ── startDaemon ───────────────────────────────────────────────────────────────
bool RcloneDbus::startDaemon(const QString& rcUser, const QString& rcPass)
{
    m_rcUser = rcUser;
    m_rcPass = rcPass;

    // Se la password è vuota, recuperiamola da KWallet
    if (m_rcPass.isEmpty()) {
        KWalletManager wallet;
        m_rcPass = wallet.readSecret(QStringLiteral("rclone-rcd-password"));
    }
    if (m_rcPass.isEmpty()) {
        qWarning() << "[RcloneDbus] No rclone rcd password found in KWallet";
        return false;
    }

    // Controlla se il daemon è già attivo con un ping
    bool alreadyRunning = false;
    {
        QNetworkRequest req(QUrl(QStringLiteral("http://127.0.0.1:%1/rc/noop").arg(m_port)));
        req.setRawHeader("Authorization",
                         "Basic " + QByteArray(QString(m_rcUser + ':' + m_rcPass)
                                                   .toUtf8()).toBase64());
        req.setTransferTimeout(500);
        auto* reply = m_nam->post(req, QByteArray("{}"));
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        alreadyRunning = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
    }
    if (alreadyRunning) {
        qDebug() << "[RcloneDbus] Daemon already running on port" << m_port;
        return true;
    }

    // Avvia rclone rcd — pass credentials via env vars to avoid leaking
    // them in /proc/<pid>/cmdline (visible to all local users via `ps aux`).
    //
    // NOTA: QProcess::startDetached() (metodo di istanza) ignora
    // setProcessEnvironment() in Qt6 — l'ambiente configurato non viene
    // ereditato dal processo figlio.  Usiamo invece QProcess::start() con
    // setDetachable(true) (Qt 6.0+) che esegue il fork/exec rispettando
    // correttamente l'ambiente impostato con setProcessEnvironment().
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("RCLONE_RC_USER"), m_rcUser);
    env.insert(QStringLiteral("RCLONE_RC_PASS"), m_rcPass);

    const QStringList args = {
        QStringLiteral("rcd"),
        QStringLiteral("--rc-addr"),          QStringLiteral("127.0.0.1:%1").arg(m_port),
        QStringLiteral("--rc-no-auth=false"),
        QStringLiteral("--no-check-certificate")
    };

    QProcess* proc = new QProcess();
    proc->setProcessEnvironment(env);
    proc->setProgram(QStringLiteral("rclone"));
    proc->setArguments(args);
    proc->setDetachable(true);   // Qt 6.0+ — il processo sopravvive al parent
    proc->start();
    const bool ok = proc->waitForStarted(3000);
    if (ok)
        proc->detach();          // distacca dopo la conferma di avvio
    proc->deleteLater();
    if (!ok)
        qWarning() << "[RcloneDbus] Failed to start rclone rcd";
    else
        qDebug() << "[RcloneDbus] rclone rcd started on port" << m_port;
    return ok;
}

// ── stopDaemon ────────────────────────────────────────────────────────────────
void RcloneDbus::stopDaemon()
{
    postRpc("core/quit", {}, [](const QVariantMap&){});
}

// ── postRpc ───────────────────────────────────────────────────────────────────
void RcloneDbus::postRpc(const QString& method, const QVariantMap& params,
                          std::function<void(const QVariantMap&)> onSuccess)
{
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/%2").arg(m_port).arg(method));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     "Basic " + QByteArray(QString(m_rcUser + ':' + m_rcPass)
                                               .toUtf8()).toBase64());
    req.setTransferTimeout(kRpcTimeoutMs);

    const QByteArray body = params.isEmpty()
        ? QByteArray("{}")
        : QJsonDocument(QJsonObject::fromVariantMap(params)).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = m_nam->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, method, onSuccess, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit rpcError(method, reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit rpcError(method, QStringLiteral("Invalid JSON response"));
            return;
        }
        const QJsonObject obj = doc.object();
        if (obj.contains("error")) {
            emit rpcError(method, obj["error"].toString());
            return;
        }
        onSuccess(obj.toVariantMap());
    });
}

// ── listRemotes ───────────────────────────────────────────────────────────────
void RcloneDbus::listRemotes()
{
    postRpc("config/listremotes", {}, [this](const QVariantMap& res) {
        const QStringList remotes = res.value("remotes").toStringList();
        emit remotesList(remotes);
    });
}

// ── mountRemote ───────────────────────────────────────────────────────────────
void RcloneDbus::mountRemote(const QString& remote, const QString& mountPoint,
                               const QVariantMap& options)
{
    QVariantMap params;
    params["fs"]         = remote + ':';
    params["mountPoint"] = mountPoint;
    if (!options.isEmpty())
        params["mountOpt"] = options;

    postRpc("mount/mount", params, [this, mountPoint](const QVariantMap&) {
        emit mountDone(mountPoint);
    });
}

// ── unmountRemote ─────────────────────────────────────────────────────────────
void RcloneDbus::unmountRemote(const QString& mountPoint)
{
    postRpc("mount/unmount", {{"mountPoint", mountPoint}},
            [this, mountPoint](const QVariantMap&) {
        emit unmountDone(mountPoint);
    });
}

// ── listMounts ────────────────────────────────────────────────────────────────
void RcloneDbus::listMounts()
{
    postRpc("mount/listmounts", {}, [this](const QVariantMap& res) {
        emit mountsList(res);
    });
}

// ── noop ──────────────────────────────────────────────────────────────────────
// Lightweight keep-alive: sends rc/noop to reset the rclone session idle timer.
void RcloneDbus::noop()
{
    postRpc("rc/noop", {}, [](const QVariantMap&){});
}
