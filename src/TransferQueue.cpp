// SPDX-License-Identifier: Apache-2.0
#include "TransferQueue.hpp"
#include <QMutexLocker>
#include <atomic>

TransferQueue::TransferQueue(QObject* parent) : QObject(parent) {}

// ---------------------------------------------------------------------------
// enqueue — add job and immediately wake one waiting worker
// ---------------------------------------------------------------------------
void TransferQueue::enqueue(const TransferJob& job)
{
    {
        QMutexLocker lock(&m_mutex);
        m_pending.enqueue(job);
    }
    m_semaphore.release();   // signal ONE worker; no busy-wait needed
}

// ---------------------------------------------------------------------------
// dequeueNext — block until a job is ready or stop is requested
// ---------------------------------------------------------------------------
TransferJob TransferQueue::dequeueNext(const std::atomic<bool>& stop)
{
    while (!stop.load(std::memory_order_acquire)) {
        // tryAcquire with timeout so we can re-check the stop flag
        if (m_semaphore.tryAcquire(1, 100 /*ms*/)) {
            QMutexLocker lock(&m_mutex);
            if (!m_pending.isEmpty())
                return m_pending.dequeue();
            // Spurious release (e.g. wakeWorker() for shutdown) — loop again
        }
    }
    return {};   // stop requested
}

// ---------------------------------------------------------------------------
// wakeWorker — unblock a worker so it can check the stop flag
// ---------------------------------------------------------------------------
void TransferQueue::wakeWorker()
{
    m_semaphore.release();
}

// ---------------------------------------------------------------------------
// markCompleted / markCompletedJob
// ---------------------------------------------------------------------------
void TransferQueue::markCompleted(const QString& src)
{
    QMutexLocker lock(&m_mutex);
    for (int i = 0; i < m_pending.size(); ++i) {
        if (m_pending[i].src == src) {
            auto job = m_pending.takeAt(i);
            job.completed = true;
            if (m_completed.size() >= kMaxCompleted)
                m_completed.removeFirst();
            m_completed.append(job);
            emit jobFinished(src);
            return;
        }
    }
    // Job already dequeued — record minimal tombstone
    TransferJob tombstone;
    tombstone.src       = src;
    tombstone.completed = true;
    if (m_completed.size() >= kMaxCompleted)
        m_completed.removeFirst();
    m_completed.append(tombstone);
    emit jobFinished(src);
}

void TransferQueue::markCompletedJob(TransferJob job)
{
    QMutexLocker lock(&m_mutex);
    for (int i = 0; i < m_pending.size(); ++i) {
        if (m_pending[i].src == job.src) {
            m_pending.takeAt(i);
            break;
        }
    }
    job.completed = true;
    if (m_completed.size() >= kMaxCompleted)
        m_completed.removeFirst();
    m_completed.append(job);
    emit jobFinished(job.src);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
int TransferQueue::pendingCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_pending.size();
}

int TransferQueue::completedCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_completed.size();
}
