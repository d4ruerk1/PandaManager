// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
#include <QString>
#include <QByteArray>
/**
 * @file ChecksumVerifier.hpp
 * @brief Verifica integrità file tramite SHA-256 e BLAKE3.
 *
 * Usato dopo ogni trasferimento cloud o locale per garantire che il file
 * non sia stato corrotto o alterato in transito.
 */
namespace PandaManager {

enum class HashAlgorithm { SHA256, BLAKE3 };

struct ChecksumReport {
    QString       filePath;
    HashAlgorithm algorithm;
    QByteArray    expectedHash; ///< Hash atteso (hex)
    QByteArray    actualHash;   ///< Hash calcolato (hex)
    bool          verified;     ///< true se expectedHash == actualHash
};

class ChecksumVerifier {
public:
    /** @brief Calcola il checksum SHA-256 di @p filePath. */
    static QByteArray sha256(const QString &filePath) noexcept;

    /** @brief Calcola il checksum BLAKE3 di @p filePath (richiede libblake3). */
    static QByteArray blake3(const QString &filePath) noexcept;

    /**
     * @brief Verifica @p filePath contro @p expectedHex e restituisce un report.
     */
    static ChecksumReport verify(const QString &filePath,
                                 const QByteArray &expectedHex,
                                 HashAlgorithm algo = HashAlgorithm::SHA256) noexcept;
};

} // namespace PandaManager
