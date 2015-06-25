#include "rtpclass.h"

void rtpClass::checkerror(int rtperr)
{
    if (rtperr < 0)
    {
        std::cout << "ERROR: " << RTPGetErrorString(rtperr) <<rtperr<< std::endl;
        exit(-1);
    }
}
rtpClass::~rtpClass()
{

}
void rtpClass::delay(unsigned long time){
    int i = 0;
    for(i=0;i<100*time;i++);
}
IplImage* rtpClass::QImageToIplImage(const QImage * qImage)
{
    int width = qImage->width();
    int height = qImage->height();
    CvSize Size;
    Size.height = height;
    Size.width = width;
    IplImage *IplImageBuffer = cvCreateImage(Size, IPL_DEPTH_8U, 3);    //记着释放内存
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRgb rgb = qImage->pixel(x, y);
            cvSet2D(IplImageBuffer, y, x, CV_RGB(qRed(rgb), qGreen(rgb), qBlue(rgb)));
        }
    }
    return IplImageBuffer;
}
void rtpClass::initInstance()
{
    WSADATA wsaData;
    int nRet;
    if((nRet = WSAStartup(MAKEWORD(2,2),&wsaData)) != 0)
    {
        cout<<"WSAStartup failed";
        exit(0);
    }
}

rtpClass::rtpClass(QObject *parent) :
    QThread(parent)
{
     initInstance();
}

void rtpClass::startThread()
{
    if(! isRunning())
    {
        stopped = false;

        start();
    }
}

void rtpClass::stopThread()
{
    if(! isFinished())
    {
        stopped = true;


        this->terminate();
        if(isFinished())
            qDebug()<<"thread quit";
    }
}

