#ifndef MAINWINDOW_HPP

#define MAINWINDOW_HPP
#include "VideoCompressorTask.hpp" // Added for backend integration (2025-07-05)

#include <QMainWindow>
#include <QTimer>
#include <QString>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QElapsedTimer>
#include <QListWidget>
#include <QVector>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include "ProcessMonitor.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


// MainWindow: GUI for video compression. Modified 2025-07-05 for backend integration.
class MainWindow : public QMainWindow
{
    Q_OBJECT // Ensure Q_OBJECT is present for Qt meta-object system

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartCompression();
    void onUpdateStats();
    void onInputFileBrowse();
    void onOutputFileBrowse();
    void onCompressionProgress(double percent, double elapsed, double eta); // Added for backend
    void onCompressionFinished(bool success, const QString& errorMsg); // Added for backend

private:
    Ui::MainWindow *ui;
    QTimer *updateTimer;
    // GUI widgets for access in slots
    QLabel *inputLabel;
    QLabel *outputLabel;
    QComboBox *presetBox;
    QSlider *crfSlider;
    QLabel *crfValue;
    QLabel *cpuLabel;
    QLabel *gpuLabel;
    QLabel *memLabel;
    QLabel *procNameLabel;
    QListWidget *threadList;
    QLabel *elapsedLabel;
    QLabel *etaLabel;
    QLabel *inputSizeLabel;
    QLabel *outputSizeLabel;
    QPushButton *startBtn;
    // Charts
    QtCharts::QChartView *cpuChartView;
    QtCharts::QChartView *memChartView;
    QtCharts::QChartView *gpuChartView;
    QtCharts::QLineSeries *cpuSeries;
    QtCharts::QLineSeries *memSeries;
    QtCharts::QLineSeries *gpuSeries;
    // State
    QString inputFilePath;
    QString outputFilePath;
    QElapsedTimer *timer;
    bool compressing = false;
    QVector<int> trackedPIDs;
    ProcessMonitor *procMonitor;
    void setupUi();
    void updateProcessStats(const QVector<ProcessStats> &stats);
    void updateTimingInfo();
    void startCompressionThread();
    void stopCompression();
    void setupCharts();
    void appendChartPoint(QtCharts::QLineSeries *series, double value);
    // Backend integration
    VideoCompressorTask* compressorTask = nullptr; // Fixed pointer type (2025-07-05)
};

#endif // MAINWINDOW_HPP
