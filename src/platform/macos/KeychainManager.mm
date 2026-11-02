// SPDX-License-Identifier: Apache-2.0
// macOS Keychain secret backend — wraps Security framework SecKeychainItem API.
// Replaces KWalletManager on macOS.
#include <QString>
#include <QDebug>
#import <Foundation/Foundation.h>
#import <Security/Security.h>

namespace PandaManager {
namespace SecretManager {

static constexpr const char* kService = "PandaManager";

bool isAvailable() { return true; }

QString readSecret(const QString& key)
{
    const QByteArray k = key.toUtf8();
    void* data = nullptr;
    UInt32 len  = 0;
    OSStatus st = SecKeychainFindGenericPassword(
        nullptr,
        static_cast<UInt32>(strlen(kService)), kService,
        static_cast<UInt32>(k.size()),         k.constData(),
        &len, &data,
        nullptr);
    if (st != errSecSuccess) {
        qDebug() << "[Keychain] Key not found:" << key << "status:" << st;
        return {};
    }
    QString result = QString::fromUtf8(static_cast<const char*>(data), static_cast<int>(len));
    SecKeychainItemFreeContent(nullptr, data);
    return result;
}

bool writeSecret(const QString& key, const QString& secret)
{
    const QByteArray k = key.toUtf8();
    const QByteArray v = secret.toUtf8();
    // Try to update first
    SecKeychainItemRef item = nullptr;
    OSStatus st = SecKeychainFindGenericPassword(
        nullptr,
        static_cast<UInt32>(strlen(kService)), kService,
        static_cast<UInt32>(k.size()),         k.constData(),
        nullptr, nullptr, &item);
    if (st == errSecSuccess && item) {
        st = SecKeychainItemModifyAttributesAndData(
            item, nullptr,
            static_cast<UInt32>(v.size()), v.constData());
        CFRelease(item);
    } else {
        st = SecKeychainAddGenericPassword(
            nullptr,
            static_cast<UInt32>(strlen(kService)), kService,
            static_cast<UInt32>(k.size()),         k.constData(),
            static_cast<UInt32>(v.size()),         v.constData(),
            nullptr);
    }
    if (st != errSecSuccess)
        qWarning() << "[Keychain] Failed to write key:" << key << "status:" << st;
    return st == errSecSuccess;
}

bool removeSecret(const QString& key)
{
    const QByteArray k = key.toUtf8();
    SecKeychainItemRef item = nullptr;
    OSStatus st = SecKeychainFindGenericPassword(
        nullptr,
        static_cast<UInt32>(strlen(kService)), kService,
        static_cast<UInt32>(k.size()),         k.constData(),
        nullptr, nullptr, &item);
    if (st != errSecSuccess || !item) return false;
    st = SecKeychainItemDelete(item);
    CFRelease(item);
    return st == errSecSuccess;
}

} // namespace SecretManager
} // namespace PandaManager
