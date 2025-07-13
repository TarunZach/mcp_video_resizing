#include "ProcessMonitor.hpp"
#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <tchar.h>
#endif

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <QFile>
#include <QTextStream>
#endif

#ifdef Q_OS_MAC
#include <mach/mach.h>
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

void ProcessMonitor::updateStatsNow()
{
    updateStats();
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
    // CPU usage (macOS, per process)
    static quint64 lastTotalTime = 0;
    static qint64 lastTimestamp = 0;
    task_thread_times_info_data_t info;
    mach_msg_type_number_t count = TASK_THREAD_TIMES_INFO_COUNT;
    kern_return_t kr = task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t)&info, &count);
    if (kr == KERN_SUCCESS)
    {
        quint64 totalTime = info.user_time.seconds * 1000000 + info.user_time.microseconds +
                            info.system_time.seconds * 1000000 + info.system_time.microseconds;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        double percent = 0.0;
        if (lastTimestamp > 0)
        {
            double deltaCPU = (totalTime - lastTotalTime) / 1000000.0;
            double deltaTime = (now - lastTimestamp) / 1000.0;
            percent = (deltaCPU / deltaTime) * 100.0;
        }
        stat.cpuUsage = percent;
        lastTotalTime = totalTime;
        lastTimestamp = now;
    }

    // Memory usage (macOS)
    mach_task_basic_info_data_t minfo;
    mach_msg_type_number_t mcount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&minfo, &mcount) == KERN_SUCCESS)
        stat.memUsage = minfo.resident_size / (1024.0 * 1024.0);

    stat.gpuUsage = 0.0; // Not available via public API
    stat.name = QCoreApplication::applicationName();
#endif

#if defined(Q_OS_WIN)
    static QMap<int, QPair<ULONGLONG, qint64>> lastCpu;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL)
    {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
        {
            stat.memUsage = pmc.WorkingSetSize / (1024.0 * 1024.0); // Bytes -> MB
        }

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

        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
        if (GetModuleBaseName(hProcess, NULL, szProcessName, sizeof(szProcessName) / sizeof(TCHAR)))
        {
            stat.name = QString::fromWCharArray(szProcessName);
        }
        CloseHandle(hProcess);
    }
#endif

#if defined(Q_OS_LINUX)
    QFile statusFile(QString("/proc/%1/status").arg(pid));
    if (statusFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&statusFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.startsWith("Name:"))
                stat.name = line.section(':', 1).trimmed();
            if (line.startsWith("VmRSS:"))
            {
                QString value = line.section(':', 1).trimmed().section(' ', 0, 0);
                stat.memUsage = value.toDouble() / 1024.0; // KB to MB
            }
        }
    }
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
