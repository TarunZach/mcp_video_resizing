#ifndef PROCESSMONITOR_HPP
#define PROCESSMONITOR_HPP

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include <QTimer>

/**
 * @brief Struct to hold process statistics.
 */
struct ProcessStats {
    int pid;
    QString name;
    double cpuUsage;   // percent
    double memUsage;   // MB
    double gpuUsage;   // percent (best effort)
};

/**
 * @brief ProcessMonitor class periodically fetches stats for a set of PIDs.
 * Emits a signal with the latest stats for all tracked processes.
 */
class ProcessMonitor : public QObject {
    Q_OBJECT // Ensure Q_OBJECT is present for Qt meta-object system
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
    void updateStats();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QVector<int> m_pids;
    QTimer *m_timer;
    // Platform-specific helpers
    ProcessStats getStatsForPID(int pid);
};

#endif // PROCESSMONITOR_HPP
