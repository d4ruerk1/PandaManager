// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
#include <QString>
/**
 * @file ClamAvScanner.hpp
 * @brief Scansione malware opzionale via ClamAV per file scaricati da cloud.
 *
 * Comunica con il daemon clamd via socket Unix (/run/clamav/clamd.ctl).
 * Se ClamAV non è installato o il daemon non è attivo, la scansione viene
 * saltata con un avviso (non bloccante).
 */
namespace PandaManager {

struct ScanResult {
    QString filePath;
    bool    clean;       ///< true = nessuna minaccia rilevata
    QString threatName;  ///< Nome del malware (vuoto se clean)
    QString rawResponse; ///< Risposta grezza di clamd
};

class ClamAvScanner {
public:
    static ClamAvScanner &instance();

    /** @brief true se clamd è raggiungibile. */
    bool isAvailable() noexcept;

    /**
     * @brief Scansiona @p filePath tramite clamd.
     * @return ScanResult con esito della scansione.
     */
    ScanResult scan(const QString &filePath) noexcept;

private:
    ClamAvScanner() = default;
    bool m_checked = false;
    bool m_available = false;
};

} // namespace PandaManager
