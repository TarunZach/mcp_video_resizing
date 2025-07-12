#ifndef TIMEREVENTFILTER_HPP
#define TIMEREVENTFILTER_HPP

#include <QObject>
#include <QEvent>
#include <QThread>
#include <QDebug>


class TimerEventFilter : public QObject {
    Q_OBJECT
public:
    explicit TimerEventFilter(QObject *parent = nullptr) : QObject(parent) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Timer) {
            qDebug() << "[TimerEvent] Object:" << obj
                     << "Class:" << obj->metaObject()->className()
                     << "CurrentThread:" << QThread::currentThread()
                     << "ObjThread:" << obj->thread();
        }
        return QObject::eventFilter(obj, event);
    }
};

#endif // TIMEREVENTFILTER_HPP
