// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
/**
 * @file KWalletManager.hpp
 * @brief Token storage via KDE KWallet (Linux/KF6).
 *        On macOS/Windows, use PandaManager::SecretManager (platform/PlatformServices.hpp).
 */
#include "platform/PlatformServices.hpp"

#ifdef HAVE_KF6

#include <QObject>
#include <QString>

namespace KWallet { class Wallet; }

class KWalletManager : public QObject {
    Q_OBJECT
public:
    explicit KWalletManager(QObject* parent = nullptr);
    ~KWalletManager() override;

    bool isAvailable() const;
    QString readSecret(const QString& key) const;
    bool writeSecret(const QString& key, const QString& secret);
    bool removeSecret(const QString& key);
    QStringList savedProviders() const;

signals:
    void walletOpened();
    void walletClosed();
    void secretWritten(const QString& key);
    void secretRemoved(const QString& key);

private:
    bool ensureOpen() const;
    mutable KWallet::Wallet* m_wallet = nullptr;
    static constexpr char kFolder[]  = "PandaManager";
    static constexpr char kAppName[] = "pandamanager";
};

#endif // HAVE_KF6
