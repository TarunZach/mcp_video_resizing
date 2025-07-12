#pragma once
#include <QObject>
#include <QString>

class VideoCompressorTask : public QObject
{
  Q_OBJECT
public:
  explicit VideoCompressorTask(QObject *parent = nullptr);
  ~VideoCompressorTask();
  void compress(const QString &in, const QString &out, int crf, int preset, QString *err);

signals:
  void progress(double percent, double elapsed, double eta);
  void finished(bool success, const QString &errorMsg);
};
