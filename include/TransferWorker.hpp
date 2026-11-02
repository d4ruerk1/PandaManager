// SPDX-License-Identifier: Apache-2.0
#pragma once
/**
 * TransferWorker — v1.0.0 (optimised)
 *
 * QThread-based worker for non-blocking I/O using KIO::FileCopyJob.
 * Uses QSemaphore for instant wake-up when a job is enqueued, eliminating
 * the 200 ms busy-wait polling loop.  Multiple TransferWorker instances
 * share the same TransferQueue, giving parallel upload/download lanes.
 *
 * Each worker:
 *   1. Waits on m_queue->semaphore() until a job is available (no CPU waste)
 *   2. Creates a KIO::FileCopyJob (VFS-aware: local↔cloud via KIO slaves)
 *   3. Emits progressUpdated (speed MB/s + ETA) via TransferQueue signal
 *   4. Marks job complete in the queue when KIO reports success or failure
 */
#include <QThread>
#include <QString>
#include <QElapsedTimer>
#include <atomic>
#include "TransferQueue.hpp"

namespace KIO { class FileCopyJob; }

class TransferWorker : public QThread {
    Q_OBJECT
public:
    explicit TransferWorker(TransferQueue* queue, QObject* parent = nullptr);

    /** Request graceful stop after the current job finishes. */
    void requestStop();

signals:
    /** Human-readable speed string, e.g. "4.2 MB/s — ETA 12 s". */
    void statusUpdate(const QString& src, const QString& statusText);

protected:
    void run() override;

private:
    TransferQueue*    m_queue;
    std::atomic<bool> m_stop{false};

    static QString formatStatus(qint64 doneBytes, qint64 totalBytes,
                                qint64 elapsedMs);
};
