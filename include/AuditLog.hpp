// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
#include <QString>
/**
 * @file AuditLog.hpp
 * @brief Audit log locale delle operazioni distruttive — cifrato (AES-256-GCM).
 *
 * Registra: formattazione disco, cancellazione sicura, smontaggio forzato.
 * I log sono salvati in XDG_DATA_HOME/pandamanager/audit.log.enc,
 * cifrati con una chiave derivata (HKDF-SHA256) dal secret KWallet dell'utente.
 * GDPR Art. 5(1)(f) — integrità e riservatezza.
 */
namespace PandaManager {

enum class AuditEvent {
    FormatDisk,       ///< Formattazione disco
    SecureDelete,     ///< Cancellazione sicura (Art. 17 GDPR)
    ForcedUmount,     ///< Smontaggio forzato
    CloudDownload,    ///< Download da provider cloud
    PermissionChange, ///< Modifica permessi file
};

class AuditLog {
public:
    static AuditLog &instance();

    /**
     * @brief Apre/crea il file di log cifrato.
     * @return true se il log è pronto per la scrittura.
     */
    bool open() noexcept;

    /**
     * @brief Registra un evento distruttivo.
     * @param event  Tipo di evento
     * @param target Percorso o dispositivo coinvolto
     * @param detail Informazioni aggiuntive opzionali
     */
    void record(AuditEvent event, const QString &target, const QString &detail = {}) noexcept;

private:
    AuditLog() = default;
    QString m_logPath;
    bool    m_ready = false;
};

} // namespace PandaManager
