// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once

/**
 * @file LibRawPreview.hpp
 * @brief Anteprima e metadata per file RAW fotografici tramite LibRaw.
 *
 * Richiede: LibRaw >= 0.21
 * Build CMake: pkg_check_modules(LIBRAW REQUIRED libraw)
 */

#include <QPixmap>
#include <QString>

struct RawMetadata {
    QString make;           ///< Produttore fotocamera (es. "Canon")
    QString model;          ///< Modello (es. "EOS R5")
    QString lensModel;      ///< Modello obiettivo
    int isoSpeed      = 0;
    float aperture    = 0.f;
    float shutterSpeed = 0.f; ///< in secondi
    float focalLength = 0.f;  ///< in mm
    int width         = 0;
    int height        = 0;
    QString timestamp;        ///< Data/ora scatto (ISO-8601)
};

class LibRawPreview {
public:
    /**
     * @brief Estrae l'anteprima JPEG embedded nel file RAW (se presente),
     *        oppure decodifica il RAW con dcraw_process() e converte in QPixmap.
     *
     * Strategia:
     *   1. Prova unpack_thumb() → thumb JPEG embedded → QPixmap
     *   2. Fallback: dcraw_process() + dcraw_make_mem_image() → conversione RGB
     *
     * @param filePath  Percorso assoluto del file RAW.
     * @param size      Dimensione massima del lato (px, 0 = originale).
     * @return QPixmap  Valido o QPixmap() in caso di errore.
     */
    static QPixmap extractThumbnail(const QString& filePath, int size = 256);

    /**
     * @brief Legge i metadata EXIF/MAKERNOTE dal file RAW.
     */
    static RawMetadata readMetadata(const QString& filePath);
};
