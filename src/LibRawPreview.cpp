// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file LibRawPreview.cpp
 * @brief Implementazione anteprima RAW tramite LibRaw.
 *
 * Dipendenze CMake:
 *   pkg_check_modules(LIBRAW REQUIRED libraw)
 *   target_link_libraries(pandamanager PRIVATE ${LIBRAW_LIBRARIES})
 *   target_include_directories(pandamanager PRIVATE ${LIBRAW_INCLUDE_DIRS})
 */

#include "LibRawPreview.hpp"

#include <QDateTime>
#include <QDebug>
#include <QImage>

#include <libraw/libraw.h>

// ── Helpers ──────────────────────────────────────────────────────────────────
namespace {

/// Apre un file RAW con LibRaw e ritorna il processor (caller deve fare delete).
/// Ritorna nullptr in caso di errore.
LibRaw* openRaw(const QString& filePath)
{
    auto* proc = new LibRaw();
    proc->imgdata.params.use_camera_wb = 1;
    proc->imgdata.params.output_bps    = 8;
    proc->imgdata.params.no_auto_bright = 0;

    const QByteArray path = filePath.toUtf8();
    if (proc->open_file(path.constData()) != LIBRAW_SUCCESS) {
        qWarning() << "[LibRawPreview] Cannot open:" << filePath;
        delete proc;
        return nullptr;
    }
    return proc;
}

} // namespace

// ── LibRawPreview::extractThumbnail ─────────────────────────────────────────
QPixmap LibRawPreview::extractThumbnail(const QString& filePath, int size)
{
    LibRaw* proc = openRaw(filePath);
    if (!proc) return {};

    QImage result;

    // Strategia 1: thumbnail JPEG embedded
    if (proc->unpack_thumb() == LIBRAW_SUCCESS) {
        const libraw_thumbnail_t& thumb = proc->imgdata.thumbnail;
        if (thumb.tformat == LIBRAW_THUMBNAIL_JPEG && thumb.tlength > 0) {
            result.loadFromData(
                reinterpret_cast<const uchar*>(thumb.thumb),
                static_cast<int>(thumb.tlength),
                "JPEG");
        }
    }

    // Strategia 2: decodifica RAW completa
    if (result.isNull()) {
        if (proc->unpack() == LIBRAW_SUCCESS &&
            proc->dcraw_process() == LIBRAW_SUCCESS)
        {
            int errCode = 0;
            libraw_processed_image_t* img = proc->dcraw_make_mem_image(&errCode);
            if (img && errCode == LIBRAW_SUCCESS) {
                result = QImage(
                    reinterpret_cast<const uchar*>(img->data),
                    static_cast<int>(img->width),
                    static_cast<int>(img->height),
                    QImage::Format_RGB888).copy(); // deep copy prima di free
                LibRaw::dcraw_clear_mem(img);
            }
        }
    }

    delete proc;

    if (result.isNull()) {
        qWarning() << "[LibRawPreview] No image decoded for:" << filePath;
        return {};
    }

    if (size > 0)
        result = result.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return QPixmap::fromImage(result);
}

// ── LibRawPreview::readMetadata ──────────────────────────────────────────────
RawMetadata LibRawPreview::readMetadata(const QString& filePath)
{
    RawMetadata meta;
    LibRaw* proc = openRaw(filePath);
    if (!proc) return meta;

    if (proc->adjust_sizes_info_only() == LIBRAW_SUCCESS) {
        const auto& idata  = proc->imgdata.idata;
        const auto& other  = proc->imgdata.other;
        const auto& lensinfo = proc->imgdata.lens;
        const auto& sizes  = proc->imgdata.sizes;

        meta.make        = QString::fromUtf8(idata.make).trimmed();
        meta.model       = QString::fromUtf8(idata.model).trimmed();
        meta.lensModel   = QString::fromUtf8(lensinfo.Lens).trimmed();
        meta.isoSpeed    = static_cast<int>(other.iso_speed);
        meta.aperture    = other.aperture;
        meta.shutterSpeed = other.shutter;
        meta.focalLength = other.focal_len;
        meta.width       = sizes.width;
        meta.height      = sizes.height;

        // Timestamp UNIX → ISO-8601
        if (other.timestamp > 0) {
            meta.timestamp = QDateTime::fromSecsSinceEpoch(
                static_cast<qint64>(other.timestamp)).toString(Qt::ISODate);
        }
    }

    delete proc;
    return meta;
}
