// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
/**
 * @file TagLibReader.cpp
 * @brief Implementazione lettura tag audio tramite TagLib.
 *
 * Dipendenze CMake (esempio):
 *   find_package(Taglib REQUIRED)
 *   target_link_libraries(pandamanager PRIVATE Taglib::Taglib)
 *
 * oppure con PkgConfig:
 *   pkg_check_modules(TAGLIB REQUIRED taglib)
 *   target_link_libraries(pandamanager PRIVATE ${TAGLIB_LIBRARIES})
 *   target_include_directories(pandamanager PRIVATE ${TAGLIB_INCLUDE_DIRS})
 */

#include "TagLibReader.hpp"

#include <QDebug>

// ── TagLib headers ────────────────────────────────────────────────────────────
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/tstring.h>

// ID3v2 (MP3, AIFF)
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/textidentificationframe.h>

// FLAC
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>

// OGG/Opus
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>

// MP4/AAC
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>

// ── Helpers ───────────────────────────────────────────────────────────────────
namespace {

inline QString tstr(const TagLib::String& s)
{
    return QString::fromStdString(s.to8Bit(true));
}

void fillCommonTags(AudioTags& out, const TagLib::Tag* tag,
                    const TagLib::AudioProperties* props)
{
    if (tag) {
        out.title   = tstr(tag->title());
        out.artist  = tstr(tag->artist());
        out.album   = tstr(tag->album());
        out.genre   = tstr(tag->genre());
        out.comment = tstr(tag->comment());
        out.year    = static_cast<int>(tag->year());
        out.track   = static_cast<int>(tag->track());
    }
    if (props) {
        out.durationSec = props->lengthInSeconds();
        out.bitrate     = props->bitrate();
        out.sampleRate  = props->sampleRate();
        out.channels    = props->channels();
    }
}

} // namespace

// ── TagLibReader::readTags ────────────────────────────────────────────────────
AudioTags TagLibReader::readTags(const QString& filePath)
{
    AudioTags out;
    const TagLib::FileName tPath(filePath.toUtf8().constData());

    // Usa FileRef come dispatcher generico per tag e proprietà di base
    TagLib::FileRef ref(tPath, /*readAudioProperties=*/true,
                        TagLib::AudioProperties::Fast);
    if (ref.isNull()) {
        qWarning() << "[TagLibReader] Cannot open:" << filePath;
        return out;
    }
    fillCommonTags(out, ref.tag(), ref.audioProperties());

    // ── Raffinamento per tipo file ─────────────────────────────────────────────

    // MP3: leggi TPOS (disco) e ALBUMARTIST (TPE2) da ID3v2
    if (auto* mpegFile = dynamic_cast<TagLib::MPEG::File*>(ref.file())) {
        if (auto* id3v2 = mpegFile->ID3v2Tag()) {
            auto getFrame = [&](const char* id) -> QString {
                const auto& map = id3v2->frameListMap();
                auto it = map.find(id);
                if (it != map.end() && !it->second.isEmpty())
                    return tstr(it->second.front()->toString());
                return {};
            };
            out.albumArtist = getFrame("TPE2");
            out.discNumber  = getFrame("TPOS").section('/', 0, 0).toInt();
        }
    }
    // FLAC: leggi DISCNUMBER / ALBUMARTIST da XiphComment
    else if (auto* flacFile = dynamic_cast<TagLib::FLAC::File*>(ref.file())) {
        if (auto* xiph = flacFile->xiphComment()) {
            auto getField = [&](const char* key) -> QString {
                const auto& map = xiph->fieldListMap();
                auto it = map.find(key);
                if (it != map.end() && !it->second.isEmpty())
                    return tstr(it->second.front());
                return {};
            };
            out.albumArtist = getField("ALBUMARTIST");
            out.discNumber  = getField("DISCNUMBER").toInt();
        }
    }
    // MP4/AAC
    else if (auto* mp4File = dynamic_cast<TagLib::MP4::File*>(ref.file())) {
        if (auto* mp4tag = mp4File->tag()) {
            auto getItem = [&](const char* key) -> QString {
                const auto& map = mp4tag->itemMap();
                auto it = map.find(key);
                if (it != map.end())
                    return tstr(it->second.toStringList().toString(", "));
                return {};
            };
            out.albumArtist = getItem("aART");
            const QString diskStr = getItem("disk");
            out.discNumber = diskStr.section('/', 0, 0).toInt();
        }
    }

    // Cover art (delegato a extractCoverArt per evitare duplicazione)
    out.coverArt = extractCoverArt(filePath);

    return out;
}

// ── TagLibReader::extractCoverArt ─────────────────────────────────────────────
QByteArray TagLibReader::extractCoverArt(const QString& filePath)
{
    const TagLib::FileName tPath(filePath.toUtf8().constData());
    TagLib::FileRef ref(tPath, false);
    if (ref.isNull()) return {};

    // MP3 / ID3v2 → APIC frame
    if (auto* mpegFile = dynamic_cast<TagLib::MPEG::File*>(ref.file())) {
        if (auto* id3v2 = mpegFile->ID3v2Tag()) {
            const auto& frames = id3v2->frameListMap()["APIC"];
            for (auto* f : frames) {
                if (auto* apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(f)) {
                    const auto& data = apic->picture();
                    if (!data.isEmpty())
                        return QByteArray(data.data(), static_cast<qsizetype>(data.size()));
                }
            }
        }
    }
    // FLAC → PICTURE block
    else if (auto* flacFile = dynamic_cast<TagLib::FLAC::File*>(ref.file())) {
        const auto& pics = flacFile->pictureList();
        if (!pics.isEmpty()) {
            const auto& data = pics.front()->data();
            return QByteArray(data.data(), static_cast<qsizetype>(data.size()));
        }
    }
    // MP4 → covr atom
    else if (auto* mp4File = dynamic_cast<TagLib::MP4::File*>(ref.file())) {
        if (auto* mp4tag = mp4File->tag()) {
            const auto& map = mp4tag->itemMap();
            auto it = map.find("covr");
            if (it != map.end()) {
                const auto& covers = it->second.toCoverArtList();
                if (!covers.isEmpty()) {
                    const auto& data = covers.front().data();
                    return QByteArray(data.data(), static_cast<qsizetype>(data.size()));
                }
            }
        }
    }

    return {};
}
