#ifndef TIMEDREAD_H
#define TIMEDREAD_H

#include <QTimer>
#include <QMessagebox>
#include <IXWebSocket.h>
#include <string.h>
#include "gamereroute.h"

class timedRead : public QObject
{
    Q_OBJECT
public:
    timedRead(std::string url, std::string device, int mode, const std::shared_ptr<baseReroute>& bp);
    ~timedRead();
    QTimer* timer;

public slots:
    void timerSlot();

private:
    ix::WebSocket webSocket;
    dsiiReroute *gInstance;

    int vMode;
    std::string devTag;
};

#endif // MYTIMER_H