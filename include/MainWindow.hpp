// include/MainWindow.hpp
#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <QPointer>

// Qt6: Just include the QtCharts headers
#include <QChartView>
#include <QLineSeries>

#include "ProcessStats.hpp"
#include "ProcessMonitor.hpp"
#include "VideoCompressorTask.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onInputFileBrowse();
    void onOutputFileBrowse();
    void onStartCompression();
    void onCompressionProgress(double percent, double elapsed, double eta);
    void onCompressionFinished(bool success, const QString &errorMsg);
    void updateProcessStats(const QVector<ProcessStats> &stats);
    void onUpdateStats();

private:
    void setupUi();
    void setupCharts();
    void appendChartPoint(QLineSeries *series, double value);

    // File I/O
    QLabel *inputLabel;
    QLabel *outputLabel;
    QComboBox *presetBox;
    QSlider *crfSlider;
    QLabel *crfValue;
    QPushButton *startBtn;

    // Timing & size
    QLabel *elapsedLabel;
    QLabel *etaLabel;
    QLabel *inputSizeLabel;
    QLabel *outputSizeLabel;

    // Process stats
    QLabel *cpuLabel;
    QLabel *memLabel;
    QLabel *gpuLabel;
    QLabel *procNameLabel;
    QListWidget *threadList;

    // Charts (global namespace in Qt6)
    QChartView *cpuChartView;
    QChartView *memChartView;
    QChartView *gpuChartView;
    QLineSeries *cpuSeries;
    QLineSeries *memSeries;
    QLineSeries *gpuSeries;

    // Helpers
    QTimer *updateTimer;
    QElapsedTimer *timer;
    ProcessMonitor *procMonitor;
    QPointer<VideoCompressorTask> compressorTask;

    bool compressing = false;
    QString inputFilePath;
    QString outputFilePath;
};
