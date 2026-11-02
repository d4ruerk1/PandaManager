// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file KWalletManager.cpp
 * @brief Implementazione KWallet API per token cloud cifrati.
 *
 * Dipendenze CMake:
 *   find_package(KF6 REQUIRED COMPONENTS Wallet)
 *   target_link_libraries(pandamanager PRIVATE KF6::Wallet)
 */

#include "KWalletManager.hpp"

#include <KWallet>
#include <QDebug>

KWalletManager::KWalletManager(QObject* parent) : QObject(parent) {}

KWalletManager::~KWalletManager()
{
    if (m_wallet) {
        // Close the wallet instance we actually opened, not a hardcoded name
        m_wallet->close(false);
        delete m_wallet;
        m_wallet = nullptr;
    }
}

// ── ensureOpen ────────────────────────────────────────────────────────────────
bool KWalletManager::ensureOpen() const
{
    if (m_wallet && m_wallet->isOpen())
        return true;

    // Apri il portafoglio di rete (kdewallet) in modo sincrono
    // (WId = 0 → nessuna finestra parent, usato da agent/daemon)
    m_wallet = KWallet::Wallet::openWallet(
        KWallet::Wallet::NetworkWallet(),
        0, // WId: 0 per apertura senza UI parent
        KWallet::Wallet::Synchronous);

    if (!m_wallet || !m_wallet->isOpen()) {
        qWarning() << "[KWalletManager] Cannot open KWallet (NetworkWallet)";
        return false;
    }

    // Crea il folder dedicato se non esiste
    if (!m_wallet->hasFolder(QLatin1String(kFolder)))
        m_wallet->createFolder(QLatin1String(kFolder));

    m_wallet->setFolder(QLatin1String(kFolder));
    return true;
}

// ── isAvailable ───────────────────────────────────────────────────────────────
bool KWalletManager::isAvailable() const
{
    if (!KWallet::Wallet::isEnabled()) {
        qDebug() << "[KWalletManager] KWallet not enabled on this system";
        return false;
    }
    return ensureOpen();
}

// ── readSecret ────────────────────────────────────────────────────────────────
QString KWalletManager::readSecret(const QString& key) const
{
    if (!ensureOpen()) return {};

    QString value;
    if (m_wallet->readPassword(key, value) != 0) {
        qDebug() << "[KWalletManager] Key not found:" << key;
        return {};
    }
    return value;
}

// ── writeSecret ───────────────────────────────────────────────────────────────
bool KWalletManager::writeSecret(const QString& key, const QString& secret)
{
    if (!ensureOpen()) return false;

    const int rc = m_wallet->writePassword(key, secret);
    if (rc != 0) {
        qWarning() << "[KWalletManager] Failed to write key:" << key << "rc=" << rc;
        return false;
    }
    emit secretWritten(key);
    return true;
}

// ── removeSecret ──────────────────────────────────────────────────────────────
bool KWalletManager::removeSecret(const QString& key)
{
    if (!ensureOpen()) return false;

    if (!m_wallet->hasEntry(key)) return true; // già assente → ok

    const int rc = m_wallet->removeEntry(key);
    if (rc != 0) {
        qWarning() << "[KWalletManager] Failed to remove key:" << key;
        return false;
    }
    emit secretRemoved(key);
    return true;
}

// ── savedProviders ────────────────────────────────────────────────────────────
QStringList KWalletManager::savedProviders() const
{
    if (!ensureOpen()) return {};
    return m_wallet->entryList();
}
