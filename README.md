# GPU Video Compressor

A multi-threaded, cross-platform video compressor with GPU acceleration and a real-time Qt6 GUI.

## Features

- **GPU-based video resizing and compression** using OpenCL and OpenCV.
- **Multi-threaded pipeline** (reader, processor, encoder) for efficient throughput.
- **Qt6 GUI** with real-time display of CPU, memory, and GPU usage graphs.
- **Supports Mac (Apple Silicon), Linux, and Windows.**
- File I/O, compression presets, live progress/ETA, and charts.

---

## Folder Structure

```
.
├── CMakeLists.txt
├── include/
│   ├── MainWindow.hpp
│   ├── ProcessMonitor.hpp
│   ├── ProcessStats.hpp
│   ├── TimerEventFilter.hpp
│   └── VideoCompressorTask.hpp
├── src/
│   ├── encoder.cpp
│   ├── opencl_driver.cpp
│   ├── video_reader.cpp
│   └── gui/
│       ├── MainWindow.cpp
│       ├── ProcessMonitor.cpp
│       ├── TimerEventFilter.cpp
│       ├── VideoCompressorTask.cpp
│       └── main.cpp
├── data/
│   └── [place your test video files here]
```

---

## Dependencies

- **Qt6** (Core, Widgets, Concurrent, Charts)
- **OpenCV**
- **OpenCL**
- **CMake ≥ 3.10**
- **(macOS users)**: Homebrew for easy Qt/OpenCV install
- **(Windows users)**: Visual Studio, Qt6, OpenCV, OpenCL

---

## Building (macOS/Linux)

1. **Install dependencies**:

   - **macOS:**

     ```sh
     brew install qt@6 opencv opencl
     ```

   - **Linux:**

     ```sh
     sudo apt-get install qt6-base-dev qt6-charts-dev qt6-concurrent-dev libopencv-dev ocl-icd-opencl-dev
     ```

2. **Clone the repo and create build folder:**

   ```sh
   git clone <repo-url>
   cd mcp_video_resizing
   mkdir build && cd build
   ```

3. **Configure with CMake:**

   ```sh
   cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt6)/lib/cmake/Qt6" -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build:**

   ```sh
   make -j$(sysctl -n hw.ncpu)   # (macOS)
   # or
   make -j$(nproc)               # (Linux)
   ```

---

## Running

### **Command-Line (CLI) Mode**

```sh
./video_compressor <input_video> <output_video>
```

### **GUI Mode**

```sh
./video_gui
```

- Use the file pickers to select input/output video files.
- Set compression preset and CRF (quality) as desired.
- Click "Start Compression."
- **Watch CPU/memory/GPU usage charts update in real time during compression.**

---

## Notes & Tips

- **Artificial Delay for Debugging Real-Time Stats:**
  By default, compression may finish quickly on small files/fast CPUs, making charts update only for a split second.
  For testing, an artificial delay can be added inside the compression loop in `VideoCompressorTask.cpp`:

  ```cpp
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ```

  Remove or comment out this line for full-speed compression.

- **GPU stats** are shown as zero by default. If you add platform-specific GPU monitoring, this can be displayed live.

- **Process stats** show the full app’s resource usage (all threads), not per-thread.

---

## Troubleshooting

- **No GUI appears or missing Qt libraries?**
  Ensure your `CMAKE_PREFIX_PATH` points to your Qt6 installation.

- **Linker or build errors on macOS?**
  Ensure you’re using Apple Silicon compatible Homebrew Qt/OpenCV, and you’ve exported any required environment variables for OpenCV/Qt.

- **On Windows:**
  Make sure all dependencies (Qt6, OpenCV, OpenCL) are in your Visual Studio environment.
  Use CMake GUI or command line to generate a Visual Studio solution, then build.

---

## License

[MIT](LICENSE)

---

## Credits

- Developed by Tarun Zacharias Akkarakalam, Can Özkan, Wonil Choi, 2025.
- Uses Qt, OpenCV, and OpenCL.
