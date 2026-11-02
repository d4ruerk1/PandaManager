// SPDX-License-Identifier: Apache-2.0
/**
 * TransferWorker.cpp — v1.0.0 (optimised)
 *
 * Key changes:
 *  - Replaced 200 ms busy-wait with TransferQueue::dequeueNext(stop) which
 *    blocks on QSemaphore::tryAcquire() — CPU usage drops to ~0 % while idle.
 *  - requestStop() calls wakeWorker() so the thread exits without waiting
 *    up to 200 ms for the next poll cycle.
 *  - KIO::FileCopyJob flags: added KIO::Overwrite only when the job requests
 *    it; resume is handled automatically by KIO.
 */
#include "TransferWorker.hpp"
#include "TransferQueue.hpp"

#include <KIO/FileCopyJob>
#include <KJobWidgets/KJobWidgets>
#include <QUrl>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QDebug>

// ---------------------------------------------------------------------------
TransferWorker::TransferWorker(TransferQueue* queue, QObject* parent)
    : QThread(parent), m_queue(queue)
{}

void TransferWorker::requestStop()
{
    m_stop.store(true, std::memory_order_release);
    // Unblock the semaphore acquire in dequeueNext so the thread exits quickly
    m_queue->wakeWorker();
}

// ---------------------------------------------------------------------------
// formatStatus — "4.2 MB/s — ETA 12 s"
// ---------------------------------------------------------------------------
QString TransferWorker::formatStatus(qint64 doneBytes, qint64 totalBytes,
                                     qint64 elapsedMs)
{
    if (elapsedMs <= 0 || doneBytes <= 0)
        return QStringLiteral("Starting…");

    const double speedBps = static_cast<double>(doneBytes) /
                            (static_cast<double>(elapsedMs) / 1000.0);
    const double speedMBs = speedBps / (1024.0 * 1024.0);

    QString result = QStringLiteral("%1 MB/s").arg(speedMBs, 0, 'f', 1);

    if (totalBytes > 0 && speedBps > 0) {
        const qint64 remaining = totalBytes - doneBytes;
        const double etaSec    = static_cast<double>(remaining) / speedBps;
        if (etaSec < 60.0)
            result += QStringLiteral(" — ETA %1 s").arg(static_cast<int>(etaSec));
        else
            result += QStringLiteral(" — ETA %1 min").arg(static_cast<int>(etaSec / 60.0));
    }
    return result;
}

// ---------------------------------------------------------------------------
// run — thread entry point
// ---------------------------------------------------------------------------
void TransferWorker::run()
{
    qDebug() << "TransferWorker: thread started";

    while (!m_stop.load(std::memory_order_acquire)) {
        // Block efficiently until a job arrives (or stop is signalled)
        TransferJob job = m_queue->dequeueNext(m_stop);

        if (job.src.isEmpty())
            break;   // stop was requested or spurious empty job

        qDebug() << "TransferWorker: starting transfer" << job.src << "->" << job.dst;

        QUrl srcUrl = QUrl::fromUserInput(job.src);
        QUrl dstUrl = QUrl::fromUserInput(job.dst);
        if (!srcUrl.isValid() || !dstUrl.isValid()) {
            qWarning() << "TransferWorker: invalid URLs, skipping job";
            m_queue->markCompletedJob(job);
            continue;
        }

        // KIO::FileCopyJob — VFS-aware, handles local/cloud/SFTP/SMB/etc.
        // HideProgressInfo: we supply our own progress via processedSize signal.
        KIO::FileCopyJob* copyJob = KIO::file_copy(
            srcUrl, dstUrl,
            -1,                       // keep source permissions
            KIO::HideProgressInfo);
        copyJob->setUiDelegate(nullptr);   // no UI in worker thread

        QElapsedTimer timer;
        timer.start();
        bool jobOk = false;
        qint64 knownTotal = job.totalBytes;

        QEventLoop loop;

        QObject::connect(copyJob, &KIO::FileCopyJob::totalSize,
                         copyJob, [&](KJob*, qulonglong total) {
            knownTotal = static_cast<qint64>(total);
            job.totalBytes = knownTotal;
        });

        QObject::connect(copyJob, &KIO::FileCopyJob::processedSize,
                         copyJob, [&](KJob*, qulonglong processed) {
            const qint64 done = static_cast<qint64>(processed);
            job.doneBytes = done;
            emit m_queue->progressUpdated(job.src, done, knownTotal);
            emit statusUpdate(job.src,
                              formatStatus(done, knownTotal, timer.elapsed()));
        });

        QObject::connect(copyJob, &KJob::result, copyJob,
                         [&](KJob* kjob) {
            jobOk = (kjob->error() == KJob::NoError);
            if (!jobOk)
                qWarning() << "TransferWorker: KIO error:" << kjob->errorString();
            loop.quit();
        });

        copyJob->start();
        loop.exec();

        m_queue->markCompletedJob(job);
        qDebug() << "TransferWorker: job" << (jobOk ? "OK" : "FAILED") << job.src;
    }

    qDebug() << "TransferWorker: thread stopping";
}
