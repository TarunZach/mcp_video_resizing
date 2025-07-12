#pragma once

#include <QString>

// Holds one sample of CPU, memory, and GPU usage for a single PID
struct ProcessStats
{
  int pid;
  QString name;
  double cpuUsage; // in percent
  double memUsage; // in megabytes
  double gpuUsage; // in percent
};
