// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "AuditLog.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

// Nota: la cifratura reale usa OpenSSL EVP_aead_aes_256_gcm() con chiave da KWallet.
// In questa implementazione di riferimento scriviamo in chiaro con header di
// placeholder; la classe KWalletManager espone getOrCreateAuditKey() da integrare.

namespace PandaManager {

AuditLog &AuditLog::instance()
{
    static AuditLog inst;
    return inst;
}

bool AuditLog::open() noexcept
{
    const QString dataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_logPath = dataDir + QStringLiteral("/audit.log.enc");
    m_ready   = true;
    qInfo() << "[AuditLog] Log distruttivo inizializzato:" << m_logPath;
    return true;
}

static QString eventName(AuditEvent e)
{
    switch (e) {
    case AuditEvent::FormatDisk:       return QStringLiteral("FORMAT_DISK");
    case AuditEvent::SecureDelete:     return QStringLiteral("SECURE_DELETE");
    case AuditEvent::ForcedUmount:     return QStringLiteral("FORCED_UMOUNT");
    case AuditEvent::CloudDownload:    return QStringLiteral("CLOUD_DOWNLOAD");
    case AuditEvent::PermissionChange: return QStringLiteral("PERMISSION_CHANGE");
    }
    return QStringLiteral("UNKNOWN");
}

void AuditLog::record(AuditEvent event, const QString &target, const QString &detail) noexcept
{
    if (!m_ready) {
        qWarning() << "[AuditLog] Log non inizializzato, chiamare open() prima di record()";
        return;
    }

    QFile f(m_logPath);
    if (!f.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[AuditLog] Impossibile aprire il file di log:" << m_logPath;
        return;
    }

    QTextStream out(&f);
    // Formato: ISO-8601 | EVENT | target | detail
    out << QDateTime::currentDateTimeUtc().toString(Qt::ISODate)
        << " | " << eventName(event)
        << " | " << target
        << " | " << detail
        << '\n';

    qInfo() << "[AuditLog]" << eventName(event) << target;
}

} // namespace PandaManager
