
#include "ProcessMonitor.hpp"
#include <QTimer>
#include <QThread>
#include <QTimerEvent>
#include <QDebug>

// --- timerEvent override for debugging QTimer thread issues ---
void ProcessMonitor::timerEvent(QTimerEvent *event) {
    Q_ASSERT(QThread::currentThread() == this->thread());
    qDebug() << "[ProcessMonitor::timerEvent] Thread:" << QThread::currentThread()
             << "Obj thread:" << this->thread();
    QObject::timerEvent(event);
}
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

/**
 * @brief Implementation of ProcessMonitor for cross-platform process stats.
 * Uses best-effort for CPU, memory, and GPU usage.
 */
ProcessMonitor::ProcessMonitor(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    connect(m_timer, &QTimer::timeout, this, &ProcessMonitor::updateStats);

}

ProcessMonitor::~ProcessMonitor() {
    Q_ASSERT(QThread::currentThread() == this->thread());
}

void ProcessMonitor::setPIDs(const QVector<int> &pids) {
    m_pids = pids;
}

void ProcessMonitor::start(int intervalMs) {
    Q_ASSERT(QThread::currentThread() == this->thread());
    m_timer->start(intervalMs);
}

void ProcessMonitor::stop() {
    Q_ASSERT(QThread::currentThread() == this->thread());
    m_timer->stop();
}

void ProcessMonitor::updateStats() {
    QVector<ProcessStats> stats;
    for (int pid : m_pids) {
        stats.append(getStatsForPID(pid));
    }
    emit statsUpdated(stats);
}

ProcessStats ProcessMonitor::getStatsForPID(int pid) {
    ProcessStats stat;
    stat.pid = pid;
    stat.name = QString("PID %1").arg(pid);
    stat.cpuUsage = 0.0;
    stat.memUsage = 0.0;
    stat.gpuUsage = 0.0;
#ifdef Q_OS_LINUX
    // Read /proc/[pid]/stat and /proc/[pid]/status for CPU/mem
    QString statPath = QString("/proc/%1/stat").arg(pid);
    QFile statFile(statPath);
    if (statFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&statFile);
        QString line = in.readLine();
        QStringList parts = line.split(' ');
        if (parts.size() > 22) {
            stat.name = parts[1];
            stat.memUsage = 0.0; // TODO: parse VmRSS from /proc/[pid]/status
        }
    }
    // TODO: Implement CPU and memory usage parsing
#endif
#ifdef Q_OS_WIN
    // Windows: Use GetProcessMemoryInfo, etc.
    // TODO: Implement Windows process stats
#endif
    // GPU usage: Not trivial, best-effort placeholder
    stat.gpuUsage = 0.0;
    return stat;
}
