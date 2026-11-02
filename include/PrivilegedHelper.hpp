// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
#include <QString>
/**
 * @file PrivilegedHelper.hpp
 * @brief Privilege separation — processo helper separato per operazioni privilegiate.
 *
 * Il processo UI principale (senza privilegi) comunica via D-Bus con
 * org.cachyos.PandaManager.Helper, un processo suid/setcap separato che
 * esegue: mount, umount, mkfs (formato disco), operazioni di cancellazione sicura.
 * Ogni richiesta viene autorizzata da Polkit prima dell'esecuzione.
 */
namespace PandaManager {

class PrivilegedHelper {
public:
    /** @brief Avvia il processo helper se non è già in ascolto. */
    static bool ensureRunning() noexcept;

    /** @brief Richiede al helper di eseguire mount su @p device in @p mountPoint. */
    static bool requestMount(const QString &device, const QString &mountPoint) noexcept;

    /** @brief Richiede al helper di eseguire umount su @p mountPoint. */
    static bool requestUmount(const QString &mountPoint) noexcept;

    /**
     * @brief Richiede al helper di formattare @p device con @p filesystem.
     * @param filesystem Uno tra: btrfs, ext4, exfat, ntfs, vfat
     */
    static bool requestFormat(const QString &device, const QString &filesystem) noexcept;

    /**
     * @brief Richiede al helper di cancellare in modo sicuro @p path
     *        (sovrascrittura DoD 5220.22-M a 7 passi).
     */
    static bool requestSecureDelete(const QString &path) noexcept;
};

} // namespace PandaManager
