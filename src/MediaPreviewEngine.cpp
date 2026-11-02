// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file MediaPreviewEngine.cpp
 * @brief Motore unificato di anteprima multimediale.
 *
 * Integra:
 *   - FFmpegThumbnailer  → thumbnail video (libavformat/libavcodec/libswscale)
 *   - LibRawPreview      → anteprima RAW fotografici (LibRaw)
 *   - TagLibReader       → tag audio ID3/Vorbis/FLAC/MP4 (TagLib)
 *   - Qt QImageReader    → immagini standard e SVG
 */

#include "MediaPreviewEngine.hpp"
#include "FFmpegThumbnailer.hpp"
#include "LibRawPreview.hpp"
#include "TagLibReader.hpp"

#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QDebug>

// ── Estensioni supportate ─────────────────────────────────────────────────────
static const QStringList IMAGE_EXTS = {
    "jpg","jpeg","png","webp","avif","jxl","heic","qoi","svg","tiff","tif",
    "psd","psb","exr","hdr","dds","tga"
};
static const QStringList RAW_EXTS   = {"cr2","cr3","nef","arw","raf","dng","orf","rw2"};
static const QStringList VIDEO_EXTS = {"mp4","mkv","webm","mov","avi","bik","bk2","mxf","ts","m2ts","vob"};
static const QStringList AUDIO_EXTS = {"mp3","flac","ogg","opus","wav","aiff","aac","alac","ape","dsf","dff","ac3","dts"};
static const QStringList TRACKER_EXTS = {"mod","xm","it","s3m","mid","midi"};

// ── Costruttore ───────────────────────────────────────────────────────────────
MediaPreviewEngine::MediaPreviewEngine(QObject* parent) : QObject(parent) {}

// ── classify ──────────────────────────────────────────────────────────────────
MediaPreviewEngine::MediaType MediaPreviewEngine::classify(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (IMAGE_EXTS.contains(ext))   return MediaType::Image;
    if (RAW_EXTS.contains(ext))     return MediaType::RawPhoto;
    if (VIDEO_EXTS.contains(ext))   return MediaType::Video;
    if (AUDIO_EXTS.contains(ext))   return MediaType::Audio;
    if (TRACKER_EXTS.contains(ext)) return MediaType::Tracker;
    return MediaType::Unknown;
}

// ── thumbnail ─────────────────────────────────────────────────────────────────
QPixmap MediaPreviewEngine::thumbnail(const QString& filePath, int size)
{
    const auto type = classify(filePath);

    switch (type) {

    case MediaType::Image: {
        // Qt QImageReader gestisce JPEG, PNG, WebP, TIFF, SVG rasterizzato, ecc.
        QImageReader reader(filePath);
        reader.setScaledSize({size, size});
        QImage img = reader.read();
        if (!img.isNull())
            return QPixmap::fromImage(img);
        // Fallback su FFmpeg per formati esotici (EXR, DDS, TGA, AVIF, JXL)
        qDebug() << "[MediaPreviewEngine] QImageReader fallback → FFmpeg for" << filePath;
        return FFmpegThumbnailer::extractVideoThumbnail(filePath, size, 0.0);
    }

    case MediaType::RawPhoto:
        // LibRaw: estrae thumbnail JPEG embedded o decodifica RAW
        return LibRawPreview::extractThumbnail(filePath, size);

    case MediaType::Video:
        // FFmpeg: decodifica il frame alla posizione 5 s
        return FFmpegThumbnailer::extractVideoThumbnail(filePath, size, 5.0);

    case MediaType::Audio: {
        // TagLib: restituisce la copertina embedded (APIC/PICTURE/covr)
        const QByteArray coverData = TagLibReader::extractCoverArt(filePath);
        if (!coverData.isEmpty()) {
            QPixmap pm;
            if (pm.loadFromData(coverData))
                return pm.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        // Placeholder musicale
        return makePlaceholder(filePath, size);
    }

    case MediaType::Tracker:
        // Nessuna copertina standard nei tracker; placeholder tipizzato
        return makePlaceholder(filePath, size);

    default:
        return makePlaceholder(filePath, size);
    }
}

// ── metadata ──────────────────────────────────────────────────────────────────
MediaMetadata MediaPreviewEngine::metadata(const QString& filePath)
{
    MediaMetadata m;
    m.container = QFileInfo(filePath).suffix().toUpper();

    const auto type = classify(filePath);

    switch (type) {

    case MediaType::Video: {
        const VideoMetadata vm = FFmpegThumbnailer::readVideoMetadata(filePath);
        m.codec     = vm.codec;
        m.container = vm.container.isEmpty() ? m.container : vm.container;
        m.width     = vm.width;
        m.height    = vm.height;
        m.duration  = vm.duration;
        break;
    }

    case MediaType::RawPhoto: {
        const RawMetadata rm = LibRawPreview::readMetadata(filePath);
        m.width  = rm.width;
        m.height = rm.height;
        // Artist = "Make Model"
        m.artist = (rm.make + ' ' + rm.model).trimmed();
        break;
    }

    case MediaType::Audio: {
        const AudioTags at = TagLibReader::readTags(filePath);
        m.title      = at.title;
        m.artist     = at.artist;
        m.album      = at.album;
        m.duration   = static_cast<qint64>(at.durationSec) * 1000;
        m.sampleRate = at.sampleRate;
        m.channels   = at.channels;
        break;
    }

    default:
        break;
    }

    return m;
}

// ── makePlaceholder (helper privato) ─────────────────────────────────────────
QPixmap MediaPreviewEngine::makePlaceholder(const QString& filePath, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setPen(Qt::gray);
    p.drawText(pm.rect(), Qt::AlignCenter,
               QFileInfo(filePath).suffix().toUpper());
    return pm;
}
