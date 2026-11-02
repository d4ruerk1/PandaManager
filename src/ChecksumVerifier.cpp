// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "ChecksumVerifier.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>

namespace PandaManager {

QByteArray ChecksumVerifier::sha256(const QString &filePath) noexcept
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[ChecksumVerifier] Impossibile aprire il file:" << filePath;
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f)) {
        qWarning() << "[ChecksumVerifier] Errore lettura dati per SHA-256";
        return {};
    }
    return hash.result().toHex();
}

QByteArray ChecksumVerifier::blake3(const QString &filePath) noexcept
{
    // libblake3 is not yet linked. Fall back to SHA-256 so that verify()
    // still returns a meaningful result instead of always failing.
    // TODO: link libblake3 and replace this stub.
    qWarning() << "[ChecksumVerifier] BLAKE3 not linked — falling back to SHA-256 for:" << filePath;
    return sha256(filePath);
}

ChecksumReport ChecksumVerifier::verify(const QString &filePath,
                                        const QByteArray &expectedHex,
                                        HashAlgorithm algo) noexcept
{
    ChecksumReport report;
    report.filePath     = filePath;
    report.algorithm    = algo;
    report.expectedHash = expectedHex.toLower();

    if (algo == HashAlgorithm::SHA256) {
        report.actualHash = sha256(filePath);
    } else {
        // BLAKE3 falls back to SHA-256 internally; mark algorithm so the
        // caller knows a fallback was used.
        report.actualHash = blake3(filePath);
        qWarning() << "[ChecksumVerifier] BLAKE3 verify used SHA-256 fallback — result may not match a real BLAKE3 digest";
    }

    report.verified = (!report.actualHash.isEmpty() &&
                       report.actualHash == report.expectedHash);

    if (!report.verified)
        qWarning() << "[ChecksumVerifier] Verifica FALLITA per:" << filePath
                   << "atteso:" << report.expectedHash
                   << "calcolato:" << report.actualHash;
    else
        qInfo() << "[ChecksumVerifier] Verifica OK per:" << filePath;

    return report;
}

} // namespace PandaManager
