#include "ProcessMonitor.hpp"

#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

ProcessMonitor::ProcessMonitor(QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &ProcessMonitor::updateStats);
}

ProcessMonitor::~ProcessMonitor()
{
    if (m_timer->isActive())
        m_timer->stop();
}

void ProcessMonitor::setPIDs(const QVector<int> &pids)
{
    m_pids = pids;
}

void ProcessMonitor::start(int intervalMs)
{
    if (!m_timer->isActive())
        m_timer->start(intervalMs);
}

void ProcessMonitor::stop()
{
    if (m_timer->isActive())
        m_timer->stop();
}

void ProcessMonitor::updateStats()
{
    QVector<ProcessStats> stats;
    for (int pid : m_pids)
    {
        stats.append(getStatsForPID(pid));
    }
    emit statsUpdated(stats);
}

ProcessStats ProcessMonitor::getStatsForPID(int pid)
{
    ProcessStats stat;
    stat.pid = pid;
    stat.name = QString("PID %1").arg(pid);
    stat.cpuUsage = 0.0;
    stat.memUsage = 0.0;
    stat.gpuUsage = 0.0;

#if defined(Q_OS_MAC)
    // --- Real-time CPU% calculation for macOS ---
    // Uses 'ps' and deltas between samples for %CPU
    static QMap<int, QPair<double, qint64>> lastCpu;
    QString cmd = QString("ps -p %1 -o %cpu=,rss=,comm=").arg(pid);
    QProcess ps;
    ps.start(cmd);
    ps.waitForFinished(500);
    QString output = ps.readAllStandardOutput().trimmed();
    if (!output.isEmpty())
    {
        QStringList parts = output.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 3)
        {
            double currCpu = parts[0].toDouble();
            double currRssMb = parts[1].toDouble() / 1024.0;
            QFileInfo fi(parts[2]);
            stat.name = fi.fileName();
            stat.memUsage = currRssMb;

            // Get time (ms)
            qint64 now = QDateTime::currentMSecsSinceEpoch();

            if (lastCpu.contains(pid))
            {
                double prevCpu = lastCpu[pid].first;
                qint64 prevTime = lastCpu[pid].second;
                double deltaSecs = (now - prevTime) / 1000.0;
                // Since ps gives %CPU averaged over last second, we just use current value.
                // For more smoothing, average prevCpu and currCpu:
                stat.cpuUsage = (currCpu + prevCpu) / 2.0;
            }
            else
            {
                stat.cpuUsage = currCpu;
            }
            lastCpu[pid] = qMakePair(currCpu, now);
        }
    }
#endif

#if defined(Q_OS_WIN)
    // --- Real-time CPU% calculation for Windows ---
    static QMap<int, QPair<ULONGLONG, qint64>> lastCpu; // processTime, timestamp
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL)
    {
        // Memory usage
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
        {
            stat.memUsage = pmc.WorkingSetSize / (1024.0 * 1024.0); // Bytes -> MB
        }

        // CPU usage as % (since process start)
        FILETIME createTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime))
        {
            ULARGE_INTEGER kTime, uTime;
            kTime.LowPart = kernelTime.dwLowDateTime;
            kTime.HighPart = kernelTime.dwHighDateTime;
            uTime.LowPart = userTime.dwLowDateTime;
            uTime.HighPart = userTime.dwHighDateTime;
            ULONGLONG totalProcessTime = (kTime.QuadPart + uTime.QuadPart) / 10; // microseconds

            qint64 now = QDateTime::currentMSecsSinceEpoch();

            if (lastCpu.contains(pid))
            {
                ULONGLONG prevProcessTime = lastCpu[pid].first;
                qint64 prevTime = lastCpu[pid].second;
                qint64 deltaRealMs = now - prevTime;
                ULONGLONG deltaProcUs = totalProcessTime - prevProcessTime;
                if (deltaRealMs > 0)
                {
                    SYSTEM_INFO sysInfo;
                    GetSystemInfo(&sysInfo);
                    int numProcs = sysInfo.dwNumberOfProcessors;
                    double cpuPercent = (deltaProcUs / 1000.0) / (deltaRealMs * numProcs) * 100.0;
                    stat.cpuUsage = cpuPercent;
                }
            }
            lastCpu[pid] = qMakePair(totalProcessTime, now);
        }

        // Process name
        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
        if (GetModuleBaseName(hProcess, NULL, szProcessName, sizeof(szProcessName) / sizeof(TCHAR)))
        {
            stat.name = QString::fromWCharArray(szProcessName);
        }
        CloseHandle(hProcess);
    }
#endif

#if defined(Q_OS_LINUX)
    // Get name and memory from /proc/[pid]/status
    QFile statusFile(QString("/proc/%1/status").arg(pid));
    if (statusFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&statusFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.startsWith("Name:"))
            {
                stat.name = line.section(':', 1).trimmed();
            }
            if (line.startsWith("VmRSS:"))
            {
                QString value = line.section(':', 1).trimmed().section(' ', 0, 0);
                stat.memUsage = value.toDouble() / 1024.0; // KB to MB
            }
        }
    }
    // Real-time CPU% for Linux
    static QMap<int, QPair<long, qint64>> lastStats;
    long total_time = 0;
    QFile statFile(QString("/proc/%1/stat").arg(pid));
    if (statFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&statFile);
        QStringList parts = in.readLine().split(' ');
        if (parts.size() > 21)
        {
            long utime = parts[13].toLong();
            long stime = parts[14].toLong();
            total_time = utime + stime;
        }
    }
    long ticks_per_second = sysconf(_SC_CLK_TCK);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastStats.contains(pid))
    {
        long prev_time = lastStats[pid].first;
        qint64 prev_stamp = lastStats[pid].second;
        long delta_time = total_time - prev_time;
        double delta_secs = (now - prev_stamp) / 1000.0;
        if (delta_secs > 0)
            stat.cpuUsage = 100.0 * (delta_time / double(ticks_per_second)) / delta_secs;
    }
    lastStats[pid] = qMakePair(total_time, now);
#endif

    return stat;
}
