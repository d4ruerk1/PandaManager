// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 PandaManager Contributors
#include "GpgVerifier.hpp"
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

namespace PandaManager {

GpgVerifyResult GpgVerifier::verify(const QString &filePath, const QString &sigPath) noexcept
{
    GpgVerifyResult result;
    result.filePath = filePath;
    result.valid    = false;

    const QString sig = sigPath.isEmpty() ? filePath + QStringLiteral(".asc") : sigPath;
    result.sigPath  = sig;

    if (!QFile::exists(filePath)) {
        result.errorDetail = QStringLiteral("File non trovato: ") + filePath;
        return result;
    }
    if (!QFile::exists(sig)) {
        result.errorDetail = QStringLiteral("Firma non trovata: ") + sig;
        return result;
    }

    QProcess gpg;
    gpg.start(QStringLiteral("gpg2"),
              { QStringLiteral("--status-fd"), QStringLiteral("1"),
                QStringLiteral("--verify"), sig, filePath });
    if (!gpg.waitForFinished(30000)) {
        // Process did not finish within the timeout — kill it to avoid zombies
        gpg.kill();
        gpg.waitForFinished(3000);
        result.errorDetail = QStringLiteral("gpg2 timed out");
        qWarning() << "[GPG] gpg2 timed out for:" << filePath;
        return result;
    }

    const QString out = QString::fromUtf8(gpg.readAllStandardOutput());
    const QString err = QString::fromUtf8(gpg.readAllStandardError());

    // Cerca [GNUPG:] GOODSIG
    static const QRegularExpression reSig(
        QStringLiteral(R"(\[GNUPG:\] GOODSIG ([0-9A-F]+) (.+))"));
    static const QRegularExpression reFp(
        QStringLiteral(R"(\[GNUPG:\] VALIDSIG ([0-9A-F]+))"));

    auto mSig = reSig.match(out);
    auto mFp  = reFp.match(out);

    if (out.contains(QStringLiteral("[GNUPG:] GOODSIG")) && mSig.hasMatch()) {
        result.valid      = true;
        result.signerUid  = mSig.captured(2).trimmed();
        if (mFp.hasMatch())
            result.fingerprint = mFp.captured(1);
        qInfo() << "[GPG] Firma valida per:" << filePath << "— Firmatario:" << result.signerUid;
    } else {
        result.errorDetail = err.trimmed();
        qWarning() << "[GPG] Firma NON valida per:" << filePath << err;
    }

    return result;
}

bool GpgVerifier::importKey(const QString &fingerprint) noexcept
{
    QProcess gpg;
    gpg.start(QStringLiteral("gpg2"),
              { QStringLiteral("--keyserver"), QStringLiteral("hkps://keys.openpgp.org"),
                QStringLiteral("--recv-keys"), fingerprint });
    if (!gpg.waitForFinished(30000)) {
        gpg.kill();
        gpg.waitForFinished(3000);
        qWarning() << "[GPG] importKey timed out for fingerprint:" << fingerprint;
        return false;
    }
    const bool ok = gpg.exitCode() == 0;
    if (!ok)
        qWarning() << "[GPG] Importazione chiave fallita:" << fingerprint;
    else
        qInfo() << "[GPG] Chiave importata:" << fingerprint;
    return ok;
}

} // namespace PandaManager
