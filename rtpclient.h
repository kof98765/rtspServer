#ifndef RTPCLIENT_H
#define RTPCLIENT_H

#include <QThread>
#include <QUdpSocket>
#include <errno.h>
#include <QSemaphore>
#include "rtpclass.h"
#include <QTimer>
#include "decodeandsendthread.h"
const int maxThreadNum=1;

class rtpClient : public rtpClass
{
    Q_OBJECT
public:
     rtpClient(QObject *parent = 0);
     ~rtpClient();
     void rtpInit(QString ip,int port);
     void decodeData(int index,unsigned char *buf,unsigned char *yuv);
private:
     RTPSession *sess;
     pic_data picData;
     QList<QByteArray*> cacheLink;
     unsigned char buffer[14745600];
     unsigned char cam_yuv[14745600];
    QFile f;
     CvCapture * capture;
     IplImage *frame;
     QTimer *timer;
     int width,height;
     QSemaphore sem;
     void run();
     QMutex lock;
     int index;
     int currentIndex;
     decodeAndSendThread *decodeThread[16];
     int decodeResult[maxThreadNum];
signals:
    void display(IplImage *,int index);
    void heartpack();
public slots:
    void getFrame();
    void sendH264Data(unsigned char *buffer,int length);
    void sendData();
    void  finishDecode(unsigned char *buffer,int length);

};

#endif // RTPCLIENT_H
