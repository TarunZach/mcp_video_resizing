
#include "MainWindow.hpp"
#include <QFileDialog>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QThread>
#include <QListWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <QCoreApplication>
#include <QVector>
#include <QPointF>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include "ProcessMonitor.hpp"

#include <QtConcurrent> // Added for threaded compression (2025-07-05)


// --- Global Timer Event Filter for Debugging QTimer Thread Issues ---
#include "TimerEventFilter.hpp"

/**
 * @brief Setup the line charts for CPU, memory, and GPU usage.
 */
void MainWindow::setupCharts() {
    cpuSeries = new QtCharts::QLineSeries();
    memSeries = new QtCharts::QLineSeries();
    gpuSeries = new QtCharts::QLineSeries();

    auto cpuChart = new QtCharts::QChart();
    cpuChart->addSeries(cpuSeries);
    cpuChart->createDefaultAxes();
    cpuChart->setTitle("CPU Usage (%)");
    cpuChartView->setChart(cpuChart);

    auto memChart = new QtCharts::QChart();
    memChart->addSeries(memSeries);
    memChart->createDefaultAxes();
    memChart->setTitle("Memory Usage (MB)");
    memChartView->setChart(memChart);

    auto gpuChart = new QtCharts::QChart();
    gpuChart->addSeries(gpuSeries);
    gpuChart->createDefaultAxes();
    gpuChart->setTitle("GPU Usage (%)");
    gpuChartView->setChart(gpuChart);
}

/**
 * @brief Append a new value to a chart series, keeping the last 100 points.
 */
void MainWindow::appendChartPoint(QtCharts::QLineSeries *series, double value) {
    QVector<QPointF> points = series->pointsVector();
    double x = points.isEmpty() ? 0 : points.last().x() + 1;
    points.append(QPointF(x, value));
    if (points.size() > 100) points.remove(0, points.size() - 100);
    series->replace(points);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(nullptr)
{
    setupUi();
    setupCharts();
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateStats);
    updateTimer->start(100); // 100 ms
    timer = new QElapsedTimer();
    procMonitor = new ProcessMonitor(this);
    // Do NOT setParent(nullptr); always let Qt parent/child system manage deletion
    connect(procMonitor, &ProcessMonitor::statsUpdated, this, &MainWindow::updateProcessStats);
}

// --- Install the global event filter in main() ---
// (This code should be placed in main.cpp, but for demonstration, you can copy it there)
//
// int main(int argc, char *argv[]) {
//     QApplication app(argc, argv);
//     TimerEventFilter *filter = new TimerEventFilter;
//     app.installEventFilter(filter);
//     ...existing code...
// }
// --- Optionally, override timerEvent in ProcessMonitor for thread checks ---
#include "ProcessMonitor.hpp"

// Add this method to ProcessMonitor.cpp/.hpp:
// void ProcessMonitor::timerEvent(QTimerEvent *event) {
//     Q_ASSERT(QThread::currentThread() == this->thread());
//     qDebug() << "[ProcessMonitor::timerEvent] Thread:" << QThread::currentThread()
//              << "Obj thread:" << this->thread();
//     QObject::timerEvent(event);
// }

// Ensure ProcessMonitor is deleted in the main thread to avoid QTimer/thread errors
// and prevent QObject::~QObject: Timers cannot be stopped from another thread
MainWindow::~MainWindow() {
    if (updateTimer) {
        updateTimer->stop();
    }
    if (procMonitor) {
        procMonitor->stop();
        procMonitor->deleteLater();
        procMonitor = nullptr;
    }
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // File pickers
    QHBoxLayout *fileLayout = new QHBoxLayout;
    QPushButton *inputBrowse = new QPushButton("Input File");
    QPushButton *outputBrowse = new QPushButton("Output File");
    inputLabel = new QLabel("No file selected");
    outputLabel = new QLabel("No file selected");
    fileLayout->addWidget(inputBrowse);
    fileLayout->addWidget(inputLabel);
    fileLayout->addWidget(outputBrowse);
    fileLayout->addWidget(outputLabel);
    mainLayout->addLayout(fileLayout);
    connect(inputBrowse, &QPushButton::clicked, this, &MainWindow::onInputFileBrowse);
    connect(outputBrowse, &QPushButton::clicked, this, &MainWindow::onOutputFileBrowse);

    // Preset dropdown
    presetBox = new QComboBox;
    presetBox->addItems({"preset-1", "preset-2", "preset-3"});
    mainLayout->addWidget(presetBox);

    // CRF slider
    QHBoxLayout *crfLayout = new QHBoxLayout;
    QLabel *crfLabel = new QLabel("CRF:");
    crfSlider = new QSlider(Qt::Horizontal);
    crfSlider->setRange(0, 51);
    crfSlider->setValue(23);
    crfValue = new QLabel("23");
    crfLayout->addWidget(crfLabel);
    crfLayout->addWidget(crfSlider);
    crfLayout->addWidget(crfValue);
    mainLayout->addLayout(crfLayout);
    connect(crfSlider, &QSlider::valueChanged, [this](int v){ crfValue->setText(QString::number(v)); });

    // Start button
    startBtn = new QPushButton("Start Compression");
    mainLayout->addWidget(startBtn);
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartCompression);

    // Stats group
    QGroupBox *statsGroup = new QGroupBox("Process Stats");
    QFormLayout *statsLayout = new QFormLayout;
    cpuLabel = new QLabel("0%");
    gpuLabel = new QLabel("0%");
    memLabel = new QLabel("0 MB");
    procNameLabel = new QLabel("-");
    threadList = new QListWidget;
    statsLayout->addRow("CPU Usage:", cpuLabel);
    statsLayout->addRow("GPU Usage:", gpuLabel);
    statsLayout->addRow("Memory Usage:", memLabel);
    statsLayout->addRow("Process Name:", procNameLabel);
    statsLayout->addRow("Threads:", threadList);
    statsGroup->setLayout(statsLayout);
    mainLayout->addWidget(statsGroup);

    // Timing/Summary
    QGroupBox *timingGroup = new QGroupBox("Timing & Summary");
    QFormLayout *timingLayout = new QFormLayout;
    elapsedLabel = new QLabel("0s");
    etaLabel = new QLabel("0s");
    inputSizeLabel = new QLabel("0 MB");
    outputSizeLabel = new QLabel("0 MB");
    timingLayout->addRow("Elapsed Time:", elapsedLabel);
    timingLayout->addRow("Estimated Time Left:", etaLabel);
    timingLayout->addRow("Input Size:", inputSizeLabel);
    timingLayout->addRow("Output Size:", outputSizeLabel);
    timingGroup->setLayout(timingLayout);
    mainLayout->addWidget(timingGroup);

    // Charts for overall usage
    QGroupBox *usageGroup = new QGroupBox("Overall Usage Graphs");
    QVBoxLayout *usageLayout = new QVBoxLayout;
    cpuChartView = new QtCharts::QChartView;
    memChartView = new QtCharts::QChartView;
    gpuChartView = new QtCharts::QChartView;
    usageLayout->addWidget(new QLabel("CPU Usage"));
    usageLayout->addWidget(cpuChartView);
    usageLayout->addWidget(new QLabel("Memory Usage"));
    usageLayout->addWidget(memChartView);
    usageLayout->addWidget(new QLabel("GPU Usage"));
    usageLayout->addWidget(gpuChartView);
    usageGroup->setLayout(usageLayout);
    mainLayout->addWidget(usageGroup);

    setCentralWidget(central);
}

void MainWindow::onInputFileBrowse() {
    QString file = QFileDialog::getOpenFileName(this, "Select Input Video");
    if (!file.isEmpty()) {
        inputFilePath = file;
        inputLabel->setText(QFileInfo(file).fileName());
        QFileInfo info(file);
        inputSizeLabel->setText(QString::number(info.size() / (1024.0 * 1024.0), 'f', 2) + " MB");
    }
}

void MainWindow::onOutputFileBrowse() {
    QString file = QFileDialog::getSaveFileName(this, "Select Output Video");
    if (!file.isEmpty()) {
        outputFilePath = file;
        outputLabel->setText(QFileInfo(file).fileName());
    }
}

// Modified 2025-07-05: Connect GUI to backend pipeline using VideoCompressorTask
void MainWindow::onStartCompression() {
    if (inputFilePath.isEmpty() || outputFilePath.isEmpty()) {
        QMessageBox::warning(this, "Missing File", "Please select both input and output files.");
        return;
    }
    if (compressing) return;
    compressing = true;
    startBtn->setEnabled(false);
    timer->start();
    elapsedLabel->setText("0s");
    etaLabel->setText("Calculating...");
    outputSizeLabel->setText("0 MB");
    threadList->clear();
    // Backend integration: launch compression in a thread
    // Always delete compressorTask from the main thread (2025-07-06)
    if (compressorTask) {
        // Always delete compressorTask from the main thread
        if (QThread::currentThread() != this->thread()) {
            QMetaObject::invokeMethod(compressorTask, "deleteLater", Qt::QueuedConnection);
        } else {
            compressorTask->deleteLater();
        }
        compressorTask = nullptr;
    }
    // Always stop and delete ProcessMonitor from the main thread before starting a new compression
    if (procMonitor) {
        procMonitor->stop();
        procMonitor->deleteLater();
        procMonitor = nullptr;
    }
    procMonitor = new ProcessMonitor(this);
    // Do NOT setParent(nullptr); always let Qt parent/child system manage deletion
    connect(procMonitor, &ProcessMonitor::statsUpdated, this, &MainWindow::updateProcessStats);
    compressorTask = new VideoCompressorTask(this);
    connect(compressorTask, &VideoCompressorTask::progress, this, &MainWindow::onCompressionProgress);
    connect(compressorTask, &VideoCompressorTask::finished, this, &MainWindow::onCompressionFinished);
    // Simulate receiving PIDs (for demo, use current process PID)
    trackedPIDs.clear();
    trackedPIDs.append(QCoreApplication::applicationPid());
    procMonitor->setPIDs(trackedPIDs);
    procMonitor->start(100);
    // Launch compression in a thread
    QtConcurrent::run([this]{
        int crf = crfSlider->value();
        int preset = presetBox->currentIndex();
        QString errorMsg;
        compressorTask->compress(inputFilePath, outputFilePath, crf, preset, &errorMsg);
    });
}

// Removed dummy thread. Now handled by VideoCompressorTask (2025-07-05)
// Backend integration: update progress from VideoCompressorTask
void MainWindow::onCompressionProgress(double percent, double elapsed, double eta) {
    elapsedLabel->setText(QString::number(elapsed, 'f', 1) + "s");
    etaLabel->setText(QString::number(eta, 'f', 1) + "s");
    // Optionally update a progress bar here
}

// Backend integration: handle finish signal from VideoCompressorTask
void MainWindow::onCompressionFinished(bool success, const QString& errorMsg) {
    // Ensure all GUI updates and QObject deletions happen in the main thread
    QMetaObject::invokeMethod(this, [this, success, errorMsg]() {
        compressing = false;
        startBtn->setEnabled(true);
        // Always stop and delete ProcessMonitor after compression
        if (procMonitor) {
            procMonitor->stop();
            procMonitor->deleteLater();
            procMonitor = nullptr;
        }
        if (success) {
            etaLabel->setText("Done");
            threadList->addItem("Compression complete.");
            QFileInfo info(outputFilePath);
            outputSizeLabel->setText(QString::number(info.size() / (1024.0 * 1024.0), 'f', 2) + " MB");
        } else {
            QMessageBox::critical(this, "Compression Failed", errorMsg);
            threadList->addItem("Compression failed.");
        }
    }, Qt::QueuedConnection);
}


void MainWindow::onUpdateStats() {
    if (compressing) {
        // Update elapsed time
        elapsedLabel->setText(QString::number(timer->elapsed() / 1000.0, 'f', 1) + "s");
        // TODO: estimate ETA
        // Process stats are updated via ProcessMonitor::statsUpdated
    }
}

/**
 * @brief Slot to update process stats in the GUI.
 * @param stats List of ProcessStats for tracked PIDs.
 */
void MainWindow::updateProcessStats(const QVector<ProcessStats> &stats) {
    if (stats.isEmpty()) return;
    // For now, show the first process (main PID)
    const ProcessStats &ps = stats.first();
    cpuLabel->setText(QString::number(ps.cpuUsage, 'f', 1) + "%");
    memLabel->setText(QString::number(ps.memUsage, 'f', 1) + " MB");
    gpuLabel->setText(QString::number(ps.gpuUsage, 'f', 1) + "%");
    procNameLabel->setText(ps.name);
    // Update charts
    appendChartPoint(cpuSeries, ps.cpuUsage);
    appendChartPoint(memSeries, ps.memUsage);
    appendChartPoint(gpuSeries, ps.gpuUsage);
    // Thread list placeholder
    threadList->clear();
    threadList->addItem("Main Thread");
}