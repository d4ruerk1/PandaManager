// SPDX-License-Identifier: Apache-2.0
// Stub: in-memory / no-op secret manager — used when neither KWallet, Keychain
// nor WinCred is compiled in. Logs a warning and returns empty strings.
// Replace with a real backend by enabling HAVE_KF6, PLATFORM_MACOS, or PLATFORM_WINDOWS.
#include <QString>
#include <QDebug>

namespace PandaManager {
namespace SecretManager {

bool isAvailable() { return false; }

QString readSecret(const QString& key)
{
    qWarning() << "[SecretManager] No secret backend — cannot read key:" << key;
    return {};
}

bool writeSecret(const QString& key, const QString&)
{
    qWarning() << "[SecretManager] No secret backend — cannot write key:" << key;
    return false;
}

bool removeSecret(const QString& key)
{
    qWarning() << "[SecretManager] No secret backend — cannot remove key:" << key;
    return false;
}

} // namespace SecretManager
} // namespace PandaManager
