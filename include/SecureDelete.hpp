// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
#include <QString>
/**
 * @file SecureDelete.hpp
 * @brief Cancellazione sicura file (GDPR Art. 17 — Diritto all'oblio).
 *
 * Implementa sovrascrittura multipla secondo DoD 5220.22-M (7 passi)
 * e pattern Gutmann (35 passi) selezionabili dall'utente.
 */
namespace PandaManager {

enum class WipeStandard {
    DoD_7pass,    ///< DoD 5220.22-M — 7 sovrascritture
    Gutmann_35,   ///< Gutmann — 35 sovrascritture
    SinglePass,   ///< 1 sovrascrittura random (veloce, per SSD/NVMe)
};

class SecureDelete {
public:
    /**
     * @brief Cancella in modo sicuro il file in @p path.
     * @param path     Percorso del file da cancellare
     * @param standard Metodo di sovrascrittura
     * @return true se completato senza errori
     */
    static bool wipeFile(const QString &path,
                         WipeStandard standard = WipeStandard::DoD_7pass) noexcept;

    /**
     * @brief Cancella in modo sicuro tutti i file in una directory (ricorsivo).
     */
    static bool wipeDirectory(const QString &path,
                              WipeStandard standard = WipeStandard::DoD_7pass) noexcept;
};

} // namespace PandaManager
