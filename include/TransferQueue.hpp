// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QSemaphore>
#include <QString>

struct TransferJob {
    QString src;
    QString dst;
    qint64  totalBytes = 0;
    qint64  doneBytes  = 0;
    bool    completed  = false;
};

/**
 * TransferQueue — thread-safe job queue (v1.0.0 optimised).
 *
 * Key changes vs. previous version:
 *  - QSemaphore replaces the 200 ms busy-wait poll in TransferWorker:
 *    workers block on acquire() and wake immediately when a job arrives.
 *  - m_completed is capped at kMaxCompleted entries (circular ring) to
 *    prevent unbounded memory growth during long sessions.
 *  - All public methods remain guarded by QMutex (multi-worker safe).
 */
class TransferQueue : public QObject {
    Q_OBJECT
public:
    /** Maximum completed-job history retained in memory. */
    static constexpr int kMaxCompleted = 500;

    explicit TransferQueue(QObject* parent = nullptr);

    /**
     * Enqueue a new job and release the semaphore so a waiting worker
     * wakes up immediately.
     */
    void enqueue(const TransferJob& job);

    /**
     * Block until a job is available, then dequeue and return it.
     * Pass the worker's stop-flag so the acquire can be interrupted:
     * returns an empty TransferJob when stop is requested.
     *
     * @param stop  atomic stop flag owned by TransferWorker
     */
    TransferJob dequeueNext(const std::atomic<bool>& stop);

    void markCompleted(const QString& src);

    /**
     * Preferred overload: marks a job complete while preserving all its
     * metadata (dst, totalBytes, doneBytes).
     */
    void markCompletedJob(TransferJob job);

    int pendingCount()   const;
    int completedCount() const;

    /** Release one semaphore token to unblock a waiting worker (used for shutdown). */
    void wakeWorker();

signals:
    /** Emitted from TransferWorker thread; connect with Qt::QueuedConnection. */
    void progressUpdated(const QString& src, qint64 done, qint64 total);
    void jobFinished(const QString& src);

private:
    mutable QMutex      m_mutex;
    QSemaphore          m_semaphore;          // counts available jobs
    QQueue<TransferJob> m_pending;
    QList<TransferJob>  m_completed;          // ring-capped at kMaxCompleted
};
