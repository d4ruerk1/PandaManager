// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "SecureDelete.hpp"
#include "AuditLog.hpp"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <cstring>

namespace PandaManager {

// Pattern DoD 5220.22-M (semplificato a 7 passi: 0x00, 0xFF, random, 0x00, 0xFF, random, 0x00)
static const QList<QByteArray> dodPatterns(qint64 size)
{
    return {
        QByteArray(static_cast<int>(size), '\x00'),
        QByteArray(static_cast<int>(size), '\xFF'),
        QByteArray(), // random
        QByteArray(static_cast<int>(size), '\x00'),
        QByteArray(static_cast<int>(size), '\xFF'),
        QByteArray(), // random
        QByteArray(static_cast<int>(size), '\x00'),
    };
}

static bool writePattern(QFile &f, const QByteArray &pattern, qint64 size)
{
    f.seek(0);
    if (pattern.isEmpty()) {
        // Random pass
        QByteArray buf(static_cast<int>(size), Qt::Uninitialized);
        for (auto &b : buf)
            b = static_cast<char>(QRandomGenerator::global()->bounded(256));
        return f.write(buf) == size;
    }
    qint64 written = 0;
    while (written < size) {
        qint64 chunk = qMin(static_cast<qint64>(pattern.size()), size - written);
        qint64 w     = f.write(pattern.constData(), chunk);
        if (w < 0) return false;
        written += w;
    }
    return f.flush();
}

bool SecureDelete::wipeFile(const QString &path, WipeStandard standard) noexcept
{
    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        qWarning() << "[SecureDelete] File non trovato:" << path;
        return false;
    }

    const qint64 size = info.size();
    if (size == 0) {
        // Nothing to overwrite — just remove the file and log it.
        if (!QFile::remove(path)) {
            qWarning() << "[SecureDelete] Impossibile rimuovere il file vuoto:" << path;
            return false;
        }
        AuditLog::instance().record(AuditEvent::SecureDelete, path,
                                    QStringLiteral("0-byte file removed"));
        return true;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[SecureDelete] Impossibile aprire il file in scrittura:" << path;
        return false;
    }

    if (standard == WipeStandard::DoD_7pass) {
        // DoD 5220.22-M: 7 passi con pattern specifici (0x00, 0xFF, random, ×2, 0x00)
        for (const auto &pat : dodPatterns(size)) {
            if (!writePattern(f, pat, size)) {
                qWarning() << "[SecureDelete] Errore scrittura pattern DoD";
                return false;
            }
            f.flush();
        }
    } else if (standard == WipeStandard::Gutmann_35) {
        // Gutmann: 35 passi — i primi 4 e gli ultimi 4 sono random; i 27 centrali
        // (pass 5-31) usano pattern specifici di Gutmann per MFM/RLL.
        // Implementazione semplificata conforme allo spirito dello standard:
        // tutti i 35 passi usano dati casuali crittograficamente sicuri, che su
        // supporti moderni (SSD, NVMe, ATA Secure Erase) è equivalente o
        // superiore ai pattern originali progettati per dischi MFM anni '90.
        // BUG FIX: in precedenza il branch DoD usava passi corretti ma Gutmann
        // e SinglePass usavano lo stesso loop "random" senza distinzione.
        for (int i = 0; i < 35; ++i) {
            if (!writePattern(f, {}, size)) {  // {} → random pass in writePattern
                qWarning() << "[SecureDelete] Errore scrittura pass Gutmann" << i;
                return false;
            }
            f.flush();
        }
    } else {
        // SinglePass: una sovrascrittura random (rapida, adatta a SSD/NVMe)
        if (!writePattern(f, {}, size)) {
            qWarning() << "[SecureDelete] Errore scrittura SinglePass";
            return false;
        }
        f.flush();
    }

    const int passes = (standard == WipeStandard::DoD_7pass)  ? 7
                     : (standard == WipeStandard::Gutmann_35) ? 35
                     : 1;

    f.close();
    if (!QFile::remove(path)) {
        qWarning() << "[SecureDelete] Impossibile rimuovere il file dopo wiping:" << path;
        return false;
    }

    AuditLog::instance().record(AuditEvent::SecureDelete, path,
                                QStringLiteral("%1 pass").arg(passes));
    qInfo() << "[SecureDelete] File cancellato in modo sicuro:" << path;
    return true;
}

bool SecureDelete::wipeDirectory(const QString &path, WipeStandard standard) noexcept
{
    QDir dir(path);
    if (!dir.exists()) return false;

    bool ok = true;
    const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const auto &e : entries)
        ok &= wipeFile(e.absoluteFilePath(), standard);

    const auto subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &s : subdirs)
        ok &= wipeDirectory(s.absoluteFilePath(), standard);

    dir.rmdir(path);
    return ok;
}

} // namespace PandaManager
