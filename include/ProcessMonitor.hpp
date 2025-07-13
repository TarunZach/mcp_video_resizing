#ifndef PROCESSMONITOR_HPP
#define PROCESSMONITOR_HPP

#include <QObject>
#include <QVector>
#include <QString>
#include <QTimer>

#include "ProcessStats.hpp"

/**
 * @brief ProcessMonitor class periodically fetches stats for a set of PIDs.
 * Emits a signal with the latest stats for all tracked processes.
 */
class ProcessMonitor : public QObject
{
    Q_OBJECT

public:
    explicit ProcessMonitor(QObject *parent = nullptr);
    ~ProcessMonitor();

    /**
     * @brief Set the list of PIDs to monitor. Can be called at any time.
     * @param pids List of process IDs.
     */
    void setPIDs(const QVector<int> &pids);

    /**
     * @brief Start periodic monitoring.
     * @param intervalMs Refresh interval in milliseconds.
     */
    void start(int intervalMs = 100);

    /**
     * @brief Stop monitoring.
     */
    void stop();

signals:
    /**
     * @brief Emitted when new stats are available.
     * @param stats List of ProcessStats for all tracked PIDs.
     */
    void statsUpdated(const QVector<ProcessStats> &stats);

private slots:
    /**
     * @brief Called on QTimer timeout to fetch and emit updated stats.
     */
    void updateStats();

    // public slots:
    //     void updateStatsNow();

private:
    ProcessStats getStatsForPID(int pid);
    QTimer *m_timer;
    QVector<int> m_pids;
};

#endif // PROCESSMONITOR_HPP
