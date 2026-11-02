// SPDX-License-Identifier: Apache-2.0
// Windows Credential Store secret backend — wraps CredReadW / CredWriteW / CredDeleteW.
// Replaces KWalletManager on Windows.
#include <QString>
#include <QDebug>
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincred.h>

namespace PandaManager {
namespace SecretManager {

static constexpr wchar_t kPrefix[] = L"PandaManager/";

static std::wstring makeTarget(const QString& key)
{
    return (QString("PandaManager/") + key).toStdWString();
}

bool isAvailable() { return true; }

QString readSecret(const QString& key)
{
    PCREDENTIALW cred = nullptr;
    std::wstring target = makeTarget(key);
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
        qDebug() << "[WinCred] Key not found:" << key;
        return {};
    }
    QString result = QString::fromUtf8(
        reinterpret_cast<const char*>(cred->CredentialBlob),
        static_cast<int>(cred->CredentialBlobSize));
    CredFree(cred);
    return result;
}

bool writeSecret(const QString& key, const QString& secret)
{
    const QByteArray v = secret.toUtf8();
    std::wstring target = makeTarget(key);
    CREDENTIALW cred    = {};
    cred.Type                = CRED_TYPE_GENERIC;
    cred.TargetName          = const_cast<LPWSTR>(target.c_str());
    cred.CredentialBlobSize  = static_cast<DWORD>(v.size());
    cred.CredentialBlob      = reinterpret_cast<LPBYTE>(const_cast<char*>(v.constData()));
    cred.Persist             = CRED_PERSIST_LOCAL_MACHINE;
    if (!CredWriteW(&cred, 0)) {
        qWarning() << "[WinCred] Failed to write key:" << key
                   << "error:" << GetLastError();
        return false;
    }
    return true;
}

bool removeSecret(const QString& key)
{
    std::wstring target = makeTarget(key);
    if (!CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0)) {
        qWarning() << "[WinCred] Failed to remove key:" << key
                   << "error:" << GetLastError();
        return false;
    }
    return true;
}

} // namespace SecretManager
} // namespace PandaManager
