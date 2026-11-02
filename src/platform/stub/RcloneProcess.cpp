// SPDX-License-Identifier: Apache-2.0
// Stub: rclone launched as a child process (no D-Bus) — macOS / Windows / bare Linux.
// Credentials are passed via environment variables to avoid leaking them
// in /proc/<pid>/cmdline (visible to all local users via `ps aux`).
#include "CloudManager.hpp"
#include <QProcess>
#include <QProcessEnvironment>
#include <QDebug>

namespace PandaManager {

bool startRcloneRcd(const QString& configPath, quint16 port, const QString& password)
{
    // Pass password via environment variable, never on the command line.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("RCLONE_RC_PASS"), password);
    // Fixed user; per-platform secret backend supplies the actual password.
    env.insert(QStringLiteral("RCLONE_RC_USER"), QStringLiteral("pandamanager"));

    const QStringList args = {
        QStringLiteral("rcd"),
        QStringLiteral("--rc-no-auth=false"),
        QStringLiteral("--config"), configPath,
        QStringLiteral("--rc-addr"), QStringLiteral("127.0.0.1:%1").arg(port),
    };

    // startDetached(program, args, workingDir, pid) does not accept a custom
    // environment, so we start the process normally and then detach it.
    QProcess* proc = new QProcess();
    proc->setProcessEnvironment(env);
    proc->setProgram(QStringLiteral("rclone"));
    proc->setArguments(args);
    proc->start();
    const bool ok = proc->waitForStarted(3000);
    if (ok)
        proc->setParent(nullptr); // detach; process lives on
    else {
        qWarning() << "[RcloneProcess] Failed to start rclone rcd. Is rclone installed?";
        proc->deleteLater();
    }
    return ok;
}

} // namespace PandaManager
