#include "rtpclient.h"
rtpClient::rtpClient(QObject *parent) :
    rtpClass(parent)
{
    //sess=new RTPSession;
    index=0;
    sem.release(maxThreadNum);
    currentIndex=0;
    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(getFrame()));
    timer->start(40);
    for(int i=0;i<maxThreadNum;i++)
    {
        decodeThread[i]=new decodeAndSendThread(this);
        connect(decodeThread[i],SIGNAL(finishDecode(unsigned char*,int)),
                this,SLOT(finishDecode(unsigned char*,int)));
        decodeResult[i]=0;
    }

    f.setFileName("264");
    f.open(QIODevice::Append);

}
void rtpClient::decodeData(int index,unsigned char *buf,unsigned char *yuv)
{
    static int i=0;
    if(!decodeThread[index]->isRunning())
    {
        decodeThread[index]->initData(buf,yuv,i++);
        decodeThread[index]->start();
    }
}
void rtpClient::getFrame()
{
    if(stopped)
        return;
    int offset;
    frame=cvQueryFrame(capture);
    width=frame->width;
    height=frame->height;
    offset=index*width*height*3/2;
    emit display(frame,0);
    if(!sem.tryAcquire(1))
        return;
    InitLookupTable();
    ConvertRGB2YUV(frame->width,frame->height,(unsigned char *)frame->imageData,&cam_yuv[index*width*height*3]);
    decodeData(index,&buffer[offset],&cam_yuv[index*width*height*3]);
    index++;
    if(index>=maxThreadNum)
        index=0;

//        emit display(frame,0);
//        qDebug()<<frame->colorModel;

}
rtpClient::~rtpClient()
{
    timer->stop();
    cvReleaseCapture(&capture);
    Destroy();

}
void rtpClient::rtpInit(QString ip,int port)
{
    int fd_v4l;

    unsigned long destip;

    int portbase = 6000;
    int status,ret,i,j,maxPicketSize;

    // 获得接收端的IP地址和端口号
    QHostAddress add;
    add.setAddress(ip);
   // destip = ntohl(inet_addr(add.toIPv4Address()));;
    destip=add.toIPv4Address();

    if (destip == INADDR_NONE) {
        qDebug()<<"Bad IP address specified";
        return;
    }


   // destport=5000;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0/90000.0);

    sessparams.SetAcceptOwnPackets(true);

    RTPIPv4Address addr(destip,port);

    transparams.SetPortbase(portbase);
    // 创建RTP会话

    status = Create(sessparams,&transparams);
    SetDefaultTimestampIncrement(25/90000);
    checkerror(status,"Create Client");

    // 指定RTP数据接收端

    status = AddDestination(addr);
    SetDefaultPayloadType(96);
    SetDefaultMark(true);
    checkerror(status,"AddDestination");

    capture=(CvCapture *)cvCreateCameraCapture(0);
}
void rtpClient::sendH264Data(unsigned char *buffer,int length)
{
    int maxPicketSize=1024;
    static QTime time;
    static int index=0;
    double timestamp=90000/40;
    int offset=1,i;
    unsigned char payloadType=96;
//    f.write(byte);
//    f.flush();
    if(length-1<=maxPicketSize)
    {
        SendPacket(&buffer[1],length-1,payloadType,false,timestamp);
        return;
    }
    else
    {
        buffer[0] = (buffer[1] & 0xE0) | 28; // FU indicator
        buffer[1] = 0x80 | (buffer[1] & 0x1F); // FU header (with S bit)
        SendPacket(buffer,maxPicketSize,payloadType,false,timestamp);
        offset+=maxPicketSize-1;

    }
    while(length>offset)
    {
        i=length-offset;
        buffer[offset - 2] = buffer[0]; // FU indicator
        buffer[offset - 1] = buffer[1] & ~0x80; // FU header (no S bit)
        if(i>maxPicketSize)
            SendPacket(&buffer[offset-2],maxPicketSize,payloadType,false,timestamp);
        else
        {
            buffer[offset-1]|=0x40;
            SendPacket(&buffer[offset-2],length-offset+2,payloadType,false,timestamp);
        }
        offset+=maxPicketSize-2;
     }
    index++;
    emit heartpack();
}
void  rtpClient::finishDecode(unsigned char *p,int length)
{
    unsigned char *buffer=0;
    static int count=0;
    if (decodeAndSendThread* dec = dynamic_cast<decodeAndSendThread*>(sender()))
    {
        for(int i=0;i<maxThreadNum;i++)
        {
            if(decodeThread[i]==dec)
            {
                decodeResult[i]=length;
                count++;
            }
        }
    }
    if(count==maxThreadNum)
    {
        count=0;
        emit sendData();
    }
}
void rtpClient::sendData()
{

    unsigned char * buffer;
    int length;
    for(int j=0;j<maxThreadNum;j++)
    {
        buffer=&this->buffer[width*height*3/2*j];
        length=decodeResult[j];
        if(length>0)
            emit sendH264Data(buffer,length);
   }


    sem.release(maxThreadNum);

}
void rtpClient::run()
{
    int i,j,offset,length;
    while(!stopped)
    {
    }
}

