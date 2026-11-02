// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file FFmpegThumbnailer.cpp
 * @brief Implementazione thumbnail video tramite libavformat/libavcodec/libswscale.
 *
 * Dipendenze CMake (in CMakeLists.txt):
 *   pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
 *   target_link_libraries(pandamanager PRIVATE ${FFMPEG_LIBRARIES})
 *   target_include_directories(pandamanager PRIVATE ${FFMPEG_INCLUDE_DIRS})
 */

#include "FFmpegThumbnailer.hpp"

#include <QDebug>
#include <QImage>

// ── FFmpeg C headers ─────────────────────────────────────────────────────────
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// ── Helpers privati ──────────────────────────────────────────────────────────
namespace {

/// Converte un AVFrame RGB24 in QImage (deep copy dei dati).
QImage avFrameToQImage(AVFrame* frame)
{
    const int w = frame->width;
    const int h = frame->height;
    QImage img(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        std::memcpy(img.scanLine(y),
                    frame->data[0] + static_cast<ptrdiff_t>(y) * frame->linesize[0],
                    static_cast<size_t>(w) * 3);
    }
    return img;
}

} // namespace

// ── FFmpegThumbnailer::extractVideoThumbnail ─────────────────────────────────
QPixmap FFmpegThumbnailer::extractVideoThumbnail(const QString& filePath,
                                                  int size,
                                                  double seekSecs)
{
    const QByteArray path = filePath.toUtf8();

    // 1. Apri il container
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.constData(), nullptr, nullptr) < 0) {
        qWarning() << "[FFmpegThumbnailer] Cannot open:" << filePath;
        return {};
    }
    // RAII guard sul format context
    auto fmtGuard = qScopeGuard([&]{ avformat_close_input(&fmtCtx); });

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        qWarning() << "[FFmpegThumbnailer] Cannot find stream info:" << filePath;
        return {};
    }

    // 2. Trova il primo stream video
    int videoStreamIdx = -1;
    const AVCodec* codec = nullptr;
    videoStreamIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO,
                                         -1, -1, &codec, 0);
    if (videoStreamIdx < 0 || !codec) {
        qWarning() << "[FFmpegThumbnailer] No video stream:" << filePath;
        return {};
    }

    AVStream* stream = fmtCtx->streams[videoStreamIdx];

    // 3. Apri il codec
    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) return {};
    auto codecGuard = qScopeGuard([&]{ avcodec_free_context(&codecCtx); });

    avcodec_parameters_to_context(codecCtx, stream->codecpar);
    codecCtx->thread_count = 2;
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        qWarning() << "[FFmpegThumbnailer] Cannot open codec:" << filePath;
        return {};
    }

    // 4. Seek alla posizione desiderata (in secondi)
    if (seekSecs > 0.0 && fmtCtx->duration > 0) {
        const double clampedSecs = qMin(seekSecs,
            static_cast<double>(fmtCtx->duration) / AV_TIME_BASE * 0.9);
        const int64_t ts = static_cast<int64_t>(clampedSecs * AV_TIME_BASE);
        av_seek_frame(fmtCtx, -1, ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codecCtx);
    }

    // 5. Decodifica il primo frame completo
    AVPacket* pkt  = av_packet_alloc();
    AVFrame*  frame = av_frame_alloc();
    auto pkGuard   = qScopeGuard([&]{ av_packet_free(&pkt); av_frame_free(&frame); });

    QImage result;
    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index != videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }
        if (avcodec_send_packet(codecCtx, pkt) < 0) {
            av_packet_unref(pkt);
            continue;
        }
        av_packet_unref(pkt);

        if (avcodec_receive_frame(codecCtx, frame) == 0) {
            // 6. Scala a RGB24 con swscale
            const int outW = size;
            const int outH = (frame->height > 0)
                ? qMax(1, frame->height * size / frame->width)
                : size;

            SwsContext* swsCtx = sws_getContext(
                frame->width, frame->height,
                static_cast<AVPixelFormat>(frame->format),
                outW, outH, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, nullptr, nullptr, nullptr);

            if (swsCtx) {
                AVFrame* rgb = av_frame_alloc();
                av_image_alloc(rgb->data, rgb->linesize,
                               outW, outH, AV_PIX_FMT_RGB24, 1);
                rgb->width  = outW;
                rgb->height = outH;
                rgb->format = AV_PIX_FMT_RGB24;
                sws_scale(swsCtx,
                          frame->data, frame->linesize, 0, frame->height,
                          rgb->data, rgb->linesize);
                sws_freeContext(swsCtx);
                result = avFrameToQImage(rgb);
                av_freep(&rgb->data[0]);
                av_frame_free(&rgb);
            }
            break; // un frame basta per il thumbnail
        }
    }

    if (result.isNull()) {
        qWarning() << "[FFmpegThumbnailer] No frame decoded for:" << filePath;
        return {};
    }
    return QPixmap::fromImage(result);
}

// ── FFmpegThumbnailer::readVideoMetadata ─────────────────────────────────────
VideoMetadata FFmpegThumbnailer::readVideoMetadata(const QString& filePath)
{
    VideoMetadata meta;
    const QByteArray path = filePath.toUtf8();

    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.constData(), nullptr, nullptr) < 0)
        return meta;
    auto guard = qScopeGuard([&]{ avformat_close_input(&fmtCtx); });

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
        return meta;

    // Container
    if (fmtCtx->iformat)
        meta.container = QString::fromUtf8(fmtCtx->iformat->long_name);

    // Durata in ms
    if (fmtCtx->duration > 0)
        meta.duration = static_cast<qint64>(fmtCtx->duration / (AV_TIME_BASE / 1000));

    // Bitrate globale
    meta.bitrate = fmtCtx->bit_rate;

    // Stream video principale
    const AVCodec* codec = nullptr;
    const int vidIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO,
                                            -1, -1, &codec, 0);
    if (vidIdx >= 0 && codec) {
        AVStream* st = fmtCtx->streams[vidIdx];
        meta.codec  = QString::fromUtf8(codec->long_name);
        meta.width  = st->codecpar->width;
        meta.height = st->codecpar->height;
        if (st->avg_frame_rate.den > 0)
            meta.fps = av_q2d(st->avg_frame_rate);
    }

    return meta;
}
