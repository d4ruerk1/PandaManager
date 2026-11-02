// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "PrivilegedHelper.hpp"
#include <memory>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QDebug>

static constexpr const char *kHelperService   = "org.cachyos.PandaManager.Helper";
static constexpr const char *kHelperPath      = "/org/cachyos/PandaManager/Helper";
static constexpr const char *kHelperInterface = "org.cachyos.PandaManager.Helper";
static constexpr int          kTimeoutMs      = 30000;

namespace PandaManager {

static std::unique_ptr<QDBusInterface> helperIface()
{
    return std::make_unique<QDBusInterface>(kHelperService, kHelperPath, kHelperInterface,
                          QDBusConnection::systemBus());
}

bool PrivilegedHelper::ensureRunning() noexcept
{
    auto iface = helperIface();
    if (iface->isValid()) return true;

    // Avvia il helper tramite D-Bus activation (configurato in /usr/share/dbus-1/system-services/)
    QDBusConnection::systemBus().interface()->startService(kHelperService);
    iface = helperIface();
    if (!iface->isValid()) {
        qWarning() << "[PrivilegedHelper] Impossibile avviare il servizio helper D-Bus";
        return false;
    }
    return true;
}

bool PrivilegedHelper::requestMount(const QString &device, const QString &mountPoint) noexcept
{
    if (!ensureRunning()) return false;
    auto iface = helperIface();
    QDBusReply<bool> reply = iface->call(QDBus::Block, "Mount", device, mountPoint);
    if (!reply.isValid()) {
        qWarning() << "[PrivilegedHelper] Mount fallito:" << reply.error().message();
        return false;
    }
    return reply.value();
}

bool PrivilegedHelper::requestUmount(const QString &mountPoint) noexcept
{
    if (!ensureRunning()) return false;
    auto iface = helperIface();
    QDBusReply<bool> reply = iface->call(QDBus::Block, "Umount", mountPoint);
    if (!reply.isValid()) {
        qWarning() << "[PrivilegedHelper] Umount fallito:" << reply.error().message();
        return false;
    }
    return reply.value();
}

bool PrivilegedHelper::requestFormat(const QString &device, const QString &filesystem) noexcept
{
    static const QStringList allowed = { "btrfs", "ext4", "exfat", "ntfs", "vfat" };
    if (!allowed.contains(filesystem)) {
        qWarning() << "[PrivilegedHelper] Filesystem non consentito:" << filesystem;
        return false;
    }
    if (!ensureRunning()) return false;
    auto iface = helperIface();
    QDBusReply<bool> reply = iface->call(QDBus::Block, "Format", device, filesystem);
    if (!reply.isValid()) {
        qWarning() << "[PrivilegedHelper] Format fallito:" << reply.error().message();
        return false;
    }
    return reply.value();
}

bool PrivilegedHelper::requestSecureDelete(const QString &path) noexcept
{
    if (!ensureRunning()) return false;
    auto iface = helperIface();
    QDBusReply<bool> reply = iface->call(QDBus::BlockWithGui, "SecureDelete", path);
    if (!reply.isValid()) {
        qWarning() << "[PrivilegedHelper] SecureDelete fallito:" << reply.error().message();
        return false;
    }
    return reply.value();
}

} // namespace PandaManager
