#include "decodeandsendthread.h"

decodeAndSendThread::decodeAndSendThread(QObject *parent) :
    QThread(parent)
{
    qDebug()<<"decodeThread";
    h264=new h264Class;
    h264->initEncode(NULL);
}
decodeAndSendThread::~decodeAndSendThread()
{

}
void decodeAndSendThread::initData(unsigned char *b,unsigned char *v,int i)
{
    buffer=b;
    cam_yuv=v;
    index=i;
}

void decodeAndSendThread::run()
{
    QTime time;
    time.start();
    int length=h264->encode(cam_yuv,&buffer[1],index);
    qDebug()<<"decode time"<<time.restart()<<length;
    emit finishDecode(buffer,length);

}
