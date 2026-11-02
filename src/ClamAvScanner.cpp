// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "ClamAvScanner.hpp"
#include <QDebug>
#include <QLocalSocket>
#include <QFileInfo>

static constexpr const char *kClamdSocket = "/run/clamav/clamd.ctl";
static constexpr int          kTimeoutMs  = 60000;

namespace PandaManager {

ClamAvScanner &ClamAvScanner::instance()
{
    static ClamAvScanner inst;
    return inst;
}

bool ClamAvScanner::isAvailable() noexcept
{
    if (m_checked) return m_available;
    m_checked = true;

    QLocalSocket sock;
    sock.connectToServer(kClamdSocket);
    m_available = sock.waitForConnected(2000);
    if (!m_available)
        qWarning() << "[ClamAV] clamd non disponibile su" << kClamdSocket;
    else
        qInfo() << "[ClamAV] clamd raggiungibile";
    return m_available;
}

ScanResult ClamAvScanner::scan(const QString &filePath) noexcept
{
    ScanResult result;
    result.filePath = filePath;

    if (!isAvailable()) {
        qWarning() << "[ClamAV] Scansione saltata (clamd non disponibile):" << filePath;
        result.clean = true; // non bloccante
        result.rawResponse = QStringLiteral("CLAMD_UNAVAILABLE");
        return result;
    }

    QLocalSocket sock;
    sock.connectToServer(kClamdSocket);
    if (!sock.waitForConnected(5000)) {
        result.clean = true;
        result.rawResponse = QStringLiteral("CONNECT_FAILED");
        return result;
    }

    // Use ZSCAN (null-terminated path) instead of "SCAN <path>\n" to
    // safely handle paths containing spaces, newlines, or other special chars
    // that would corrupt the line-based SCAN protocol.
    // Protocol: "zSCAN\0<path>\0"
    QByteArray cmd;
    cmd.append("zSCAN\0", 6);
    cmd.append(filePath.toUtf8());
    cmd.append('\0');
    sock.write(cmd);
    sock.waitForBytesWritten(5000);
    sock.waitForReadyRead(kTimeoutMs);

    const QByteArray response = sock.readAll().trimmed();
    result.rawResponse = QString::fromUtf8(response);

    // Risposta OK: "<path>: OK"
    // Risposta infetta: "<path>: <ThreatName> FOUND"
    if (response.endsWith(" OK")) {
        result.clean = true;
    } else if (response.contains(" FOUND")) {
        result.clean = false;
        // Estrai nome minaccia
        const int foundIdx = response.lastIndexOf(" FOUND");
        const int colonIdx = response.indexOf(": ");
        if (colonIdx >= 0 && foundIdx > colonIdx) {
            result.threatName = QString::fromUtf8(
                response.mid(colonIdx + 2, foundIdx - colonIdx - 2));
        }
        qWarning() << "[ClamAV] MINACCIA RILEVATA in" << filePath << ":" << result.threatName;
    } else {
        result.clean = true; // risposta sconosciuta — non bloccante
        qWarning() << "[ClamAV] Risposta inattesa:" << result.rawResponse;
    }

    return result;
}

} // namespace PandaManager
