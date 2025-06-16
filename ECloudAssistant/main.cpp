#include "RtmpPushManager.h"
#include "ECloudAssistant.h"
#include <QApplication>
#include "PullerWgt.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ECloudAssistant w;
    w.show();
//    EventLoop loop(2);
//    PullerWgt w(nullptr);
//    w.Open("rtmp://192.168.31.30:1935/live/01");
//    w.show();
//    RtmpPushManager push;
//    push.Open("rtmp://192.168.31.30:1935/live/01");
    return a.exec();
}
