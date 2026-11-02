// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once

/**
 * @file CloudManager.hpp
 * @brief Gestore cloud sicuro — D-Bus rclone JSON-RPC + KWallet.
 *
 * Provider supportati: Google Drive, OneDrive, Dropbox, MEGA, pCloud,
 *   Proton Drive, WebDAV, SFTP, SMB/CIFS, NFS, MinIO, S3-compatible.
 *
 * Gestione risorse:
 *   - Il daemon rclone rcd viene avviato solo quando serve (lazy).
 *   - Un QTimer di inattività (kIdleTimeoutMs) arresta il daemon se non
 *     vi sono mount attivi, liberando CPU e RAM.
 *   - Un QTimer di keep-alive (kKeepaliveMs) invia rc/noop per mantenere
 *     la sessione rclone attiva il più a lungo possibile.
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QSet>

class KWalletManager;
class RcloneDbus;

class CloudManager : public QObject {
    Q_OBJECT
public:
    explicit CloudManager(QObject* parent = nullptr);
    ~CloudManager() override;

    /** @return Lista dei tipi di provider supportati (identificativi rclone). */
    static QStringList supportedProviders();

    /** @return true se KWallet è disponibile e il portafoglio aperto. */
    bool isKWalletAvailable() const;

    /** Monta un remote rclone in @p mountPoint (path sicuro sotto /home o /run/user). */
    bool mountRemote(const QString& remoteName, const QString& mountPoint);

    /** Smonta un punto di mount tramite rclone rcd JSON-RPC. */
    void unmountRemote(const QString& mountPoint);

    /**
     * @brief Lista i remote disponibili (asincrona).
     * Il risultato viene emesso tramite il segnale remotesAvailable().
     */
    QStringList availableRemotes() const;

    /** Salva il token OAuth di un provider nel KWallet cifrato. */
    bool saveProviderToken(const QString& provider, const QString& token);

    /** Rimuove il token di un provider dal KWallet (GDPR Art. 17). */
    bool removeProviderToken(const QString& provider);

signals:
    void mountReady(const QString& mountPoint);
    void mountError(const QString& error);
    void remotesAvailable(const QStringList& remotes);

private slots:
    void onIdleTimeout();
    void sendKeepalive();

private:
    static QString sanitizeInput(const QString& input);
    static bool    isPathSafe(const QString& path);

    /** Assicura che il daemon sia attivo; resetta il timer di inattività. */
    bool ensureDaemon();

    /** Reset del timer di inattività. Chiamato ad ogni operazione. */
    void resetIdleTimer();

    KWalletManager* m_wallet;
    RcloneDbus*     m_rclone;

    QSet<QString>   m_activeMounts;   ///< mount point correntemente attivi

    QTimer*         m_idleTimer;      ///< arresta il daemon se inattivo
    QTimer*         m_keepaliveTimer; ///< mantiene la sessione rclone viva

    bool            m_daemonRunning = false;

    // Timeout prima di fermare il daemon se non vi sono mount attivi (10 min).
    static constexpr int kIdleTimeoutMs   = 10 * 60 * 1000;
    // Intervallo keep-alive (4 minuti — prima della scadenza tipica di 5 min).
    static constexpr int kKeepaliveMs     = 4 * 60 * 1000;
};
