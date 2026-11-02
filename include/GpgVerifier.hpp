// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once
#include <QString>
/**
 * @file GpgVerifier.hpp
 * @brief Verifica firma PGP/GPG dei pacchetti scaricati dal cloud.
 *
 * Usa gpgme (GnuPG Made Easy) o subprocess gpg2 per verificare
 * le firme detached (.asc / .sig) dei file scaricati.
 */
namespace PandaManager {

struct GpgVerifyResult {
    QString filePath;
    QString sigPath;
    bool    valid;        ///< true = firma valida e chiave fidata
    QString fingerprint;  ///< Fingerprint della chiave che ha firmato
    QString signerUid;    ///< UID del firmatario
    QString errorDetail;  ///< Dettaglio errore se !valid
};

class GpgVerifier {
public:
    /**
     * @brief Verifica @p sigPath (firma detached) contro @p filePath.
     * @param filePath  File da verificare
     * @param sigPath   File firma (.asc o .sig); se vuoto viene cercato <filePath>.asc
     */
    static GpgVerifyResult verify(const QString &filePath,
                                  const QString &sigPath = {}) noexcept;

    /** @brief Importa una chiave pubblica PGP dal keyserver keys.openpgp.org. */
    static bool importKey(const QString &fingerprint) noexcept;
};

} // namespace PandaManager
