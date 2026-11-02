// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file CloudManager.cpp
 * @brief Gestore cloud sicuro — integra RcloneDbus e KWalletManager.
 *
 * Principio di gestione risorse:
 *   - Lazy start: il daemon rclone rcd viene avviato solo alla prima operazione.
 *   - Idle timeout (10 min): se nessun mount è attivo e non arrivano richieste,
 *     il daemon viene arrestato per liberare risorse.
 *   - Keep-alive (ogni 4 min): mentre vi sono mount attivi, viene inviato
 *     rc/noop per evitare che la sessione rclone scada (timeout tipico 5 min).
 *
 * Provider supportati (tramite rclone):
 *   Google Drive, OneDrive, Dropbox, MEGA, pCloud, Proton Drive,
 *   WebDAV, SFTP, SMB/CIFS, NFS, MinIO, S3-compatible
 */

#include "CloudManager.hpp"
#include "KWalletManager.hpp"
#include "RcloneDbus.hpp"

#include <QDir>
#include <QRegularExpression>
#include <QDebug>

// ── Provider supportati ───────────────────────────────────────────────────────
const QStringList CloudManager::supportedProviders() {
    return {
        "drive",      // Google Drive
        "onedrive",   // Microsoft OneDrive
        "dropbox",    // Dropbox
        "mega",       // MEGA
        "pcloud",     // pCloud
        "protondrive",// Proton Drive
        "webdav",     // WebDAV
        "sftp",       // SFTP
        "smb",        // SMB/CIFS
        "nfs",        // NFS
        "s3",         // S3-compatible (MinIO, AWS, Backblaze B2, ecc.)
    };
}

// ── Costruttore ───────────────────────────────────────────────────────────────
CloudManager::CloudManager(QObject* parent)
    : QObject(parent)
    , m_wallet(new KWalletManager(this))
    , m_rclone(new RcloneDbus(this))
    , m_idleTimer(new QTimer(this))
    , m_keepaliveTimer(new QTimer(this))
{
    // ── Segnali RcloneDbus ────────────────────────────────────────────────────
    connect(m_rclone, &RcloneDbus::mountDone, this, [this](const QString& mp) {
        m_activeMounts.insert(mp);
        // Con mount attivi teniamo il keep-alive attivo.
        if (!m_keepaliveTimer->isActive())
            m_keepaliveTimer->start(kKeepaliveMs);
        // Un mount fresco resetta il timer di inattività.
        resetIdleTimer();
        emit mountReady(mp);
    });

    connect(m_rclone, &RcloneDbus::unmountDone, this, [this](const QString& mp) {
        m_activeMounts.remove(mp);
        // Se non ci sono più mount, fermiamo il keep-alive e avviamo l'idle timer.
        if (m_activeMounts.isEmpty()) {
            m_keepaliveTimer->stop();
            m_idleTimer->start(kIdleTimeoutMs);
        }
    });

    connect(m_rclone, &RcloneDbus::rpcError, this,
            [this](const QString& method, const QString& err) {
        emit mountError(QStringLiteral("[%1] %2").arg(method, err));
    });
    connect(m_rclone, &RcloneDbus::remotesList, this, &CloudManager::remotesAvailable);

    // ── Idle timer: scatta una sola volta ────────────────────────────────────
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &CloudManager::onIdleTimeout);

    // ── Keep-alive: ripetuto ─────────────────────────────────────────────────
    connect(m_keepaliveTimer, &QTimer::timeout, this, &CloudManager::sendKeepalive);
}

CloudManager::~CloudManager()
{
    // Fermiamo tutto in modo ordinato.
    m_idleTimer->stop();
    m_keepaliveTimer->stop();
    if (m_daemonRunning)
        m_rclone->stopDaemon();
}

// ── onIdleTimeout ─────────────────────────────────────────────────────────────
void CloudManager::onIdleTimeout()
{
    if (!m_activeMounts.isEmpty()) return; // mount ancora attivi → non spegniamo
    if (!m_daemonRunning) return;
    qDebug() << "[CloudManager] Idle timeout: stopping rclone daemon";
    m_rclone->stopDaemon();
    m_daemonRunning = false;
}

// ── sendKeepalive ─────────────────────────────────────────────────────────────
void CloudManager::sendKeepalive()
{
    if (!m_daemonRunning || m_activeMounts.isEmpty()) {
        m_keepaliveTimer->stop();
        return;
    }
    // rc/noop è l'endpoint più leggero disponibile.
    m_rclone->noop();
    qDebug() << "[CloudManager] Keep-alive sent to rclone daemon";
}

// ── ensureDaemon ─────────────────────────────────────────────────────────────
bool CloudManager::ensureDaemon()
{
    if (m_daemonRunning) {
        resetIdleTimer();
        return true;
    }
    if (!m_rclone->startDaemon()) {
        emit mountError(QStringLiteral("Cannot start rclone daemon"));
        return false;
    }
    m_daemonRunning = true;
    // Avviamo subito il timer di inattività; verrà resettato a ogni operazione.
    m_idleTimer->start(kIdleTimeoutMs);
    return true;
}

// ── resetIdleTimer ────────────────────────────────────────────────────────────
void CloudManager::resetIdleTimer()
{
    // Ogni operazione proroga il timeout di inattività.
    m_idleTimer->start(kIdleTimeoutMs);
}

// ── isKWalletAvailable ────────────────────────────────────────────────────────
bool CloudManager::isKWalletAvailable() const
{
    return m_wallet->isAvailable();
}

// ── sanitizeInput ─────────────────────────────────────────────────────────────
QString CloudManager::sanitizeInput(const QString& input)
{
    static const QRegularExpression dangerous(R"([;&|`$<>\\]|\.\./|\.\.\\)");
    QString safe = input;
    safe.remove(dangerous);
    return safe.trimmed();
}

// ── isPathSafe ────────────────────────────────────────────────────────────────
bool CloudManager::isPathSafe(const QString& path)
{
    const QDir dir(path);
    const QString canonical = dir.canonicalPath();
    static const QStringList blocked = {
        "/", "/etc", "/usr", "/bin", "/sbin", "/lib", "/lib64",
        "/boot", "/sys", "/proc", "/dev", "/run", "/tmp",
        "/var", "/var/tmp", "/var/run", "/snap", "/media", "/mnt",
        "/opt", "/srv", "/root"
    };
    if (blocked.contains(canonical)) return false;
    return canonical.startsWith(QStringLiteral("/home/")) ||
           canonical.startsWith(QStringLiteral("/run/user/"));
}

// ── mountRemote ───────────────────────────────────────────────────────────────
bool CloudManager::mountRemote(const QString& remoteName, const QString& mountPoint)
{
    const QString safeRemote     = sanitizeInput(remoteName);
    const QString safeMountPoint = sanitizeInput(mountPoint);

    if (safeRemote.isEmpty()) {
        emit mountError(QStringLiteral("Remote name is empty or invalid"));
        return false;
    }
    if (!isPathSafe(safeMountPoint)) {
        emit mountError(QStringLiteral("Mount point is not in a safe location: %1")
                            .arg(safeMountPoint));
        return false;
    }

    QDir().mkpath(safeMountPoint);

    // Avvia il daemon solo se necessario (lazy).
    if (!ensureDaemon()) return false;

    const QVariantMap opts = {
        {"AllowOther",    false},
        {"AllowRoot",     false},
        {"ReadOnly",      false},
        {"VfsCacheMode",  QStringLiteral("writes")},
        {"NoAppleDouble", true},
        {"NoAppleXattr",  true},
    };

    m_rclone->mountRemote(safeRemote, safeMountPoint, opts);
    return true;
}

// ── unmountRemote ─────────────────────────────────────────────────────────────
void CloudManager::unmountRemote(const QString& mountPoint)
{
    const QString safe = sanitizeInput(mountPoint);
    if (safe.isEmpty()) return;
    resetIdleTimer();
    m_rclone->unmountRemote(safe);
}

// ── availableRemotes ──────────────────────────────────────────────────────────
QStringList CloudManager::availableRemotes() const
{
    // Avvia il daemon se non è attivo, poi richiede l'elenco in modo asincrono.
    if (!m_daemonRunning) {
        // const_cast: operazione logicamente read-only ma richiede daemon.
        const_cast<CloudManager*>(this)->ensureDaemon();
    } else {
        const_cast<CloudManager*>(this)->resetIdleTimer();
    }
    m_rclone->listRemotes();
    return {}; // La lista reale arriva via segnale remotesAvailable().
}

// ── saveProviderToken ─────────────────────────────────────────────────────────
bool CloudManager::saveProviderToken(const QString& provider, const QString& token)
{
    if (!supportedProviders().contains(provider)) {
        qWarning() << "[CloudManager] Unknown provider:" << provider;
        return false;
    }
    return m_wallet->writeSecret(provider + "-token", token);
}

// ── removeProviderToken ───────────────────────────────────────────────────────
bool CloudManager::removeProviderToken(const QString& provider)
{
    return m_wallet->removeSecret(provider + "-token");
}
