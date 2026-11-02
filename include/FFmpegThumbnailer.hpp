// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#pragma once

/**
 * @file FFmpegThumbnailer.hpp
 * @brief Wrapper per libavformat/libavcodec per thumbnail video e metadata codec.
 *
 * Richiede: libavformat, libavcodec, libavutil, libswscale (FFmpeg >= 6.0)
 * Build CMake: find_package(PkgConfig REQUIRED); pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
 */

#include <QPixmap>
#include <QString>

struct VideoMetadata {
    QString codec;
    QString container;
    int width      = 0;
    int height     = 0;
    qint64 duration = 0; // ms
    double fps     = 0.0;
    qint64 bitrate = 0;  // bps
};

class FFmpegThumbnailer {
public:
    /**
     * @brief Estrae un frame dal video alla posizione @p seekSecs secondi
     *        e lo restituisce come QPixmap scalato a @p size x @p size.
     *
     * Internamente usa:
     *   avformat_open_input → avformat_find_stream_info →
     *   av_seek_frame → avcodec_send_packet / avcodec_receive_frame →
     *   sws_scale → QImage::fromData
     *
     * @param filePath  Percorso assoluto del file video.
     * @param size      Dimensione massima del lato del thumbnail (px).
     * @param seekSecs  Posizione di seek in secondi (default 5 s).
     * @return QPixmap  Thumbnail valido, o QPixmap() se il file non è supportato.
     */
    static QPixmap extractVideoThumbnail(const QString& filePath,
                                         int size = 256,
                                         double seekSecs = 5.0);

    /**
     * @brief Legge i metadata del video (codec, dimensioni, durata, fps, bitrate).
     */
    static VideoMetadata readVideoMetadata(const QString& filePath);
};
