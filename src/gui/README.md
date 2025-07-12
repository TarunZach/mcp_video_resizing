# GUI Module for Video Compression Project

This directory contains the Qt-based graphical user interface for the video compression application. It provides a responsive frontend for controlling and monitoring the multi-process video compression backend.

## Structure

- `MainWindow.cpp/.hpp` — Main application window, user controls, and status display.
- `ProcessMonitor.cpp/.hpp` — Monitors and displays real-time process/thread statistics.
- `TimerEventFilter.cpp/.hpp` — Global event filter for debugging and ensuring thread-safe timer usage.
- `main.cpp` — Application entry point.
- `gui_app.pro` — Qt project file for building the GUI (if using qmake).
- `.gitignore` — Excludes build artifacts and temporary files from version control.

## Best Practices

- **Thread Safety:** All QTimer and QObject operations are performed in the main thread. Worker threads use standard C++ synchronization primitives.
- **Memory Management:** All QObject-derived classes are parented appropriately. Manual deletes are replaced with `deleteLater()` where needed.
- **Build Artifacts:** All build artifacts and temporary files are excluded via `.gitignore` and should not be committed.
- **Debugging:** `TimerEventFilter` can be enabled to log timer events and thread affinity for advanced debugging.

## Building

### With CMake (recommended)
1. From the project root:
   ```sh
   mkdir -p build && cd build
   cmake ..
   make
   ```
2. The GUI executable will be built along with the backend.

### With qmake (optional, for GUI only)
1. From `src/gui`:
   ```sh
   qmake gui_app.pro
   make
   ```

## Usage

- Launch the application from the build output directory.
- Use the GUI to select video files, start compression, and monitor progress and system resource usage in real time.

## Troubleshooting

- If you see `QObject::killTimer: Timers cannot be stopped from another thread`, ensure you are using the latest Qt version and that all timers are managed in the main thread.
- Use the `TimerEventFilter` for advanced debugging of timer events and thread affinity.
- For memory or threading issues, run the application with AddressSanitizer or Valgrind.

## License

See the project root for license information.
