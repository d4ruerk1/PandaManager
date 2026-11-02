// SPDX-License-Identifier: Apache-2.0
// Stub PrivilegedHelper — macOS / Windows / Linux without KF6.
// On macOS, privilege escalation goes via AuthorizationExecuteWithPrivileges or
// a helper tool signed with com.apple.security.temporary-exception.mach-lookup.
// On Windows, use ShellExecuteEx with "runas" verb.
// This stub logs warnings and returns false for every privileged call.
#include "PrivilegedHelper.hpp"
#include <QDebug>
#include <QProcess>

namespace PandaManager {

bool PrivilegedHelper::ensureRunning() noexcept
{
#if defined(PLATFORM_MACOS)
    qDebug() << "[PrivilegedHelper] macOS: using AuthorizationExecuteWithPrivileges";
    return true; // Implemented in KeychainManager.mm / system auth dialogs
#elif defined(PLATFORM_WINDOWS)
    qDebug() << "[PrivilegedHelper] Windows: using ShellExecuteEx runas";
    return true;
#else
    qWarning() << "[PrivilegedHelper] Stub: privileged operations not supported on this platform.";
    return false;
#endif
}

bool PrivilegedHelper::requestMount(const QString& device, const QString& mountPoint) noexcept
{
#if defined(PLATFORM_MACOS)
    // diskutil mount does not require elevation for user-owned volumes
    QProcess p;
    p.start("diskutil", {"mount", device});
    p.waitForFinished(10000);
    return p.exitCode() == 0;
#elif defined(PLATFORM_WINDOWS)
    Q_UNUSED(device) Q_UNUSED(mountPoint)
    qWarning() << "[PrivilegedHelper] Mount not implemented on Windows (use drive letter assignment).";
    return false;
#else
    Q_UNUSED(device) Q_UNUSED(mountPoint)
    qWarning() << "[PrivilegedHelper] Mount stub — no backend.";
    return false;
#endif
}

bool PrivilegedHelper::requestUmount(const QString& mountPoint) noexcept
{
#if defined(PLATFORM_MACOS)
    QProcess p;
    p.start("diskutil", {"unmount", mountPoint});
    p.waitForFinished(10000);
    return p.exitCode() == 0;
#elif defined(PLATFORM_WINDOWS)
    Q_UNUSED(mountPoint)
    return false;
#else
    Q_UNUSED(mountPoint)
    return false;
#endif
}

bool PrivilegedHelper::requestFormat(const QString& device, const QString& filesystem) noexcept
{
    Q_UNUSED(device) Q_UNUSED(filesystem)
    qWarning() << "[PrivilegedHelper] Format not implemented in stub — use platform tools.";
    return false;
}

bool PrivilegedHelper::requestSecureDelete(const QString& path) noexcept
{
#if defined(PLATFORM_MACOS)
    // `rm -P` performs 3-pass overwrite on HFS+; for APFS, disk-level secure erase is recommended
    QProcess p;
    p.start("rm", {"-Pf", path});
    p.waitForFinished(30000);
    return p.exitCode() == 0;
#elif defined(PLATFORM_WINDOWS)
    // Use SDelete (Sysinternals) or FSCTL_FILE_LEVEL_TRIM
    QProcess p;
    p.start("sdelete64", {"-p", "7", "-nobanner", path});
    p.waitForFinished(60000);
    return p.exitCode() == 0;
#else
    Q_UNUSED(path)
    return false;
#endif
}

} // namespace PandaManager
