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
#include <QScrollArea>
#include <QValueAxis>
#include "TimerEventFilter.hpp"

void MainWindow::setupCharts()
{
    cpuSeries = new QLineSeries(this);
    memSeries = new QLineSeries(this);
    gpuSeries = new QLineSeries(this);

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
    double x = series->count() > 0 ? series->at(series->count() - 1).x() + 1 : 0;
    series->append(x, value);
    if (series->count() > 100)
        series->removePoints(0, series->count() - 100);

    // Move the visible window
    if (series->chart())
    {
        auto axesX = series->chart()->axes(Qt::Horizontal);
        if (!axesX.isEmpty())
        {
            double xMax = x;
            double xMin = xMax - 100;
            if (xMin < 0)
                xMin = 0;
            axesX.first()->setRange(xMin, xMax);
        }

        // Auto-scale Y and set more grid lines
        auto axesY = series->chart()->axes(Qt::Vertical);
        if (!axesY.isEmpty())
        {
            double maxY = value;
            int count = series->count();
            for (int i = qMax(0, count - 100); i < count; ++i)
            {
                maxY = qMax(maxY, series->at(i).y());
            }
            double upper = maxY * 1.1;
            if (upper < 100)
                upper = 100;
            QValueAxis *axisY = qobject_cast<QValueAxis *>(axesY.first());
            if (axisY)
            {
                axisY->setRange(0, upper);
                axisY->setTickCount(8); // More grid lines
            }
        }
        series->chart()->update();
    }
    if (cpuChartView)
        cpuChartView->repaint();
    if (memChartView)
        memChartView->repaint();
    if (gpuChartView)
        gpuChartView->repaint();
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

    // Start ProcessMonitor ONCE
    procMonitor = new ProcessMonitor(this);
    connect(procMonitor, &ProcessMonitor::statsUpdated, this, &MainWindow::updateProcessStats);

    // Always monitor our own process
    QVector<int> pids{int(QCoreApplication::applicationPid())};
    procMonitor->setPIDs(pids);
    procMonitor->start(100);
}

MainWindow::~MainWindow()
{
    if (updateTimer)
        updateTimer->stop();
    // auto-deletes as a QObject child.
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
        QMetaObject::invokeMethod(compressorTask, "deleteLater", Qt::QueuedConnection);
        compressorTask = nullptr;
    }

    // No need to touch procMonitor!

    compressorTask = new VideoCompressorTask(this);
    connect(compressorTask, &VideoCompressorTask::progress, this, &MainWindow::onCompressionProgress);
    connect(compressorTask, &VideoCompressorTask::finished, this, &MainWindow::onCompressionFinished);

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
      // Do NOT stop/delete procMonitor!
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
    // update other UI items at timer intervals
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

    qDebug() << "CPU:" << ps.cpuUsage << "MEM:" << ps.memUsage << "GPU:" << ps.gpuUsage;
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
