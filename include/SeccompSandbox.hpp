// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
/**
 * @file SeccompSandbox.hpp
 * @brief seccomp-bpf sandbox — disponibile solo su Linux.
 *        Su macOS/Windows install() è no-op e isSupported() restituisce false.
 */
namespace PandaManager {

class SeccompSandbox {
public:
    /** Installa il filtro seccomp-bpf (Linux only). */
    static bool install() noexcept;
    /** Verifica se seccomp è supportato (Linux only). */
    static bool isSupported() noexcept;
};

} // namespace PandaManager
