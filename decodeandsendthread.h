#ifndef DECODEANDSENDTHREAD_H
#define DECODEANDSENDTHREAD_H

#include <QThread>
#include "rtpclass.h"
class decodeAndSendThread : public QThread
{
    Q_OBJECT
public:
    decodeAndSendThread(QObject *parent = 0);
    void initData(unsigned char *b,unsigned char *v,int i);

private:
    ~decodeAndSendThread();
     unsigned char *buffer;
     unsigned char *cam_yuv;
     int width,height;
     void run();
     QMutex lock;
     int index;
     int currentIndex;
     pic_data picData;
     h264Class *h264;
signals:
    void  finishDecode(unsigned char *buffer,int length);

public slots:

};

#endif // DECODEANDSENDTHREAD_H
