// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once

/**
 * @file TagLibReader.hpp
 * @brief Lettura tag audio (ID3v2, Vorbis Comment, FLAC, APEv2) tramite TagLib.
 *
 * Richiede: taglib >= 1.13
 * Build CMake: find_package(Taglib REQUIRED) oppure pkg_check_modules(TAGLIB REQUIRED taglib)
 */

#include <QString>

struct AudioTags {
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    QString comment;
    int     year        = 0;
    int     track       = 0;
    int     discNumber  = 0;
    int     durationSec = 0;  ///< Durata totale in secondi
    int     bitrate     = 0;  ///< kbps
    int     sampleRate  = 0;  ///< Hz
    int     channels    = 0;
    QByteArray coverArt;      ///< Copertina embedded (JPEG/PNG raw bytes), può essere vuota
};

class TagLibReader {
public:
    /**
     * @brief Legge tutti i tag e le proprietà audio di @p filePath.
     *
     * Supporta automaticamente:
     *   - MP3 → ID3v2 / ID3v1
     *   - OGG/Opus → Vorbis Comment
     *   - FLAC → FLAC Comment + ID3 embedded
     *   - AAC/M4A → MP4 tags
     *   - APE/WavPack → APEv2
     *   - AIFF → ID3v2 chunk
     *
     * @return AudioTags  Struttura popolata; campi stringa vuoti se tag assenti.
     */
    static AudioTags readTags(const QString& filePath);

    /**
     * @brief Estrae la copertina embedded (APIC frame ID3 / FLAC PICTURE).
     * @return QByteArray con i byte JPEG/PNG, o vuota se assente.
     */
    static QByteArray extractCoverArt(const QString& filePath);
};
