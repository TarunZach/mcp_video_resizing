
// main.cpp for GUI. No changes needed for backend integration, but comment added for clarity (2025-07-05)
#include <QApplication>
#include "MainWindow.hpp"


#include "TimerEventFilter.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Install global timer event filter for debugging QTimer thread issues
    TimerEventFilter *filter = new TimerEventFilter;
    app.installEventFilter(filter);
    MainWindow w;
    w.show();
    return app.exec();
}
