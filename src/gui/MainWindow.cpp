// src/gui/MainWindow.cpp

#include "MainWindow.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtConcurrent>
#include "TimerEventFilter.hpp"

#include <QPointer>

void MainWindow::setupCharts()
{
    cpuSeries = new QLineSeries();
    memSeries = new QLineSeries();
    gpuSeries = new QLineSeries();

    auto cpuChart = new QChart();
    cpuChart->addSeries(cpuSeries);
    cpuChart->createDefaultAxes();
    cpuChart->setTitle("CPU Usage (%)");
    cpuChartView->setChart(cpuChart);

    auto memChart = new QChart();
    memChart->addSeries(memSeries);
    memChart->createDefaultAxes();
    memChart->setTitle("Memory Usage (MB)");
    memChartView->setChart(memChart);

    auto gpuChart = new QChart();
    gpuChart->addSeries(gpuSeries);
    gpuChart->createDefaultAxes();
    gpuChart->setTitle("GPU Usage (%)");
    gpuChartView->setChart(gpuChart);
}

void MainWindow::appendChartPoint(QLineSeries *series, double value)
{
    auto points = series->points();
    double x = points.isEmpty() ? 0 : points.last().x() + 1;
    points.append({x, value});
    if (points.size() > 100)
        points.remove(0, points.size() - 100);
    series->replace(points);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupCharts();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateStats);
    updateTimer->start(100);

    timer = new QElapsedTimer();
    procMonitor = new ProcessMonitor(this);
    connect(procMonitor, &ProcessMonitor::statsUpdated, this, &MainWindow::updateProcessStats);
}

MainWindow::~MainWindow()
{
    if (updateTimer)
        updateTimer->stop();
    if (procMonitor)
    {
        procMonitor->stop();
        procMonitor->deleteLater();
    }
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

    // File pickers
    auto *fileLayout = new QHBoxLayout;
    auto *inBtn = new QPushButton("Input File");
    auto *outBtn = new QPushButton("Output File");
    inputLabel = new QLabel("No file selected");
    outputLabel = new QLabel("No file selected");
    fileLayout->addWidget(inBtn);
    fileLayout->addWidget(inputLabel);
    fileLayout->addWidget(outBtn);
    fileLayout->addWidget(outputLabel);
    mainLayout->addLayout(fileLayout);
    connect(inBtn, &QPushButton::clicked, this, &MainWindow::onInputFileBrowse);
    connect(outBtn, &QPushButton::clicked, this, &MainWindow::onOutputFileBrowse);

    // Preset + CRF
    presetBox = new QComboBox;
    presetBox->addItems({"preset-1", "preset-2", "preset-3"});
    mainLayout->addWidget(presetBox);

    auto *crfLayout = new QHBoxLayout;
    crfLayout->addWidget(new QLabel("CRF:"));
    crfSlider = new QSlider(Qt::Horizontal);
    crfSlider->setRange(0, 51);
    crfSlider->setValue(23);
    crfValue = new QLabel("23");
    crfLayout->addWidget(crfSlider);
    crfLayout->addWidget(crfValue);
    mainLayout->addLayout(crfLayout);
    connect(crfSlider, &QSlider::valueChanged, [this](int v)
            { crfValue->setText(QString::number(v)); });

    // Start button
    startBtn = new QPushButton("Start Compression");
    mainLayout->addWidget(startBtn);
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartCompression);

    // Process Stats group
    auto *statsGrp = new QGroupBox("Process Stats");
    auto *statsForm = new QFormLayout;
    cpuLabel = new QLabel("0%");
    gpuLabel = new QLabel("0%");
    memLabel = new QLabel("0 MB");
    procNameLabel = new QLabel("-");
    threadList = new QListWidget;
    statsForm->addRow("CPU Usage:", cpuLabel);
    statsForm->addRow("GPU Usage:", gpuLabel);
    statsForm->addRow("Memory Usage:", memLabel);
    statsForm->addRow("Process Name:", procNameLabel);
    statsForm->addRow("Threads:", threadList);
    statsGrp->setLayout(statsForm);
    mainLayout->addWidget(statsGrp);

    // Timing & Summary
    auto *timeGrp = new QGroupBox("Timing & Summary");
    auto *timeForm = new QFormLayout;
    elapsedLabel = new QLabel("0s");
    etaLabel = new QLabel("0s");
    inputSizeLabel = new QLabel("0 MB");
    outputSizeLabel = new QLabel("0 MB");
    timeForm->addRow("Elapsed Time:", elapsedLabel);
    timeForm->addRow("ETA:", etaLabel);
    timeForm->addRow("Input Size:", inputSizeLabel);
    timeForm->addRow("Output Size:", outputSizeLabel);
    timeGrp->setLayout(timeForm);
    mainLayout->addWidget(timeGrp);

    // Charts
    auto *chartGrp = new QGroupBox("Overall Usage Graphs");
    auto *chartLay = new QVBoxLayout;
    cpuChartView = new QChartView();
    memChartView = new QChartView();
    gpuChartView = new QChartView();
    chartLay->addWidget(new QLabel("CPU"));
    chartLay->addWidget(cpuChartView);
    chartLay->addWidget(new QLabel("Memory"));
    chartLay->addWidget(memChartView);
    chartLay->addWidget(new QLabel("GPU"));
    chartLay->addWidget(gpuChartView);
    chartGrp->setLayout(chartLay);
    mainLayout->addWidget(chartGrp);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(central);
    setCentralWidget(scrollArea);
}

void MainWindow::onInputFileBrowse()
{
    QString f = QFileDialog::getOpenFileName(this, "Select Input Video");
    if (f.isEmpty())
        return;
    inputFilePath = f;
    inputLabel->setText(QFileInfo(f).fileName());
    QFileInfo info(f);
    inputSizeLabel->setText(QString::number(info.size() / (1024.0 * 1024.0), 'f', 2) + " MB");
}

void MainWindow::onOutputFileBrowse()
{
    QString f = QFileDialog::getSaveFileName(this, "Select Output Video", "", "Videos (*.mp4 *.mkv *.avi)");
    if (!f.isEmpty() && QFileInfo(f).suffix().isEmpty())
    {
        f += ".mp4";
    }
    outputFilePath = f;
    outputLabel->setText(QFileInfo(f).fileName());
}

void MainWindow::onStartCompression()
{
    if (inputFilePath.isEmpty() || outputFilePath.isEmpty())
    {
        QMessageBox::warning(this, "Missing File", "Select both input & output");
        return;
    }
    if (compressing)
        return;
    compressing = true;
    startBtn->setEnabled(false);
    timer->start();
    elapsedLabel->setText("0s");
    etaLabel->setText("Calculating...");
    outputSizeLabel->setText("0 MB");
    threadList->clear();

    if (compressorTask)
    {
        qDebug() << "About to call deleteLater on compressorTask, address:" << static_cast<void *>(compressorTask);
        QMetaObject::invokeMethod(compressorTask, "deleteLater", Qt::QueuedConnection);
        compressorTask = nullptr;
    }
    else
    {
        qDebug() << "compressorTask is already null!";
    }

    if (procMonitor)
    {
        procMonitor->stop();
        procMonitor->deleteLater();
        procMonitor = nullptr;
    }

    procMonitor = new ProcessMonitor(this);
    connect(procMonitor, &ProcessMonitor::statsUpdated, this, &MainWindow::updateProcessStats);

    compressorTask = new VideoCompressorTask(this);
    connect(compressorTask, &VideoCompressorTask::progress, this, &MainWindow::onCompressionProgress);
    connect(compressorTask, &VideoCompressorTask::finished, this, &MainWindow::onCompressionFinished);

    QVector<int> pids{int(QCoreApplication::applicationPid())};
    procMonitor->setPIDs(pids);
    procMonitor->start(100);

    auto task = compressorTask;
    QtConcurrent::run([task, this]
                      {
    QString err;
    task->compress(inputFilePath, outputFilePath,
                   crfSlider->value(), presetBox->currentIndex(), &err); });
}

void MainWindow::onCompressionProgress(double pct, double el, double eta)
{
    elapsedLabel->setText(QString::number(el, 'f', 1) + "s");
    etaLabel->setText(QString::number(eta, 'f', 1) + "s");
}

void MainWindow::onCompressionFinished(bool success, const QString &err)
{
    QMetaObject::invokeMethod(this, [=]
                              {
      compressing=false;
      startBtn->setEnabled(true);
      if(procMonitor){ procMonitor->stop(); procMonitor->deleteLater(); }
      if(success){
        etaLabel->setText("Done");
        threadList->addItem("Compression complete.");
        QFileInfo fi(outputFilePath);
        outputSizeLabel->setText(
          QString::number(fi.size()/(1024.0*1024.0),'f',2)+" MB"
        );
      } else {
        QMessageBox::critical(this,"Compression Failed",err);
        threadList->addItem("Compression failed.");
      } }, Qt::QueuedConnection);
}

void MainWindow::onUpdateStats()
{
    if (!compressing)
        return;
    elapsedLabel->setText(
        QString::number(timer->elapsed() / 1000.0, 'f', 1) + "s");
}

void MainWindow::updateProcessStats(const QVector<ProcessStats> &stats)
{
    if (stats.isEmpty())
        return;
    auto const &ps = stats.first();
    cpuLabel->setText(QString::number(ps.cpuUsage, 'f', 1) + "%");
    memLabel->setText(QString::number(ps.memUsage, 'f', 1) + " MB");
    gpuLabel->setText(QString::number(ps.gpuUsage, 'f', 1) + "%");
    procNameLabel->setText(ps.name);
    appendChartPoint(cpuSeries, ps.cpuUsage);
    appendChartPoint(memSeries, ps.memUsage);
    appendChartPoint(gpuSeries, ps.gpuUsage);
    threadList->clear();
    threadList->addItem("Main Thread");
}
