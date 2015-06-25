#ifndef RTPSERVER_H
#define RTPSERVER_H

#include "rtpclass.h"
class rtpServer : public rtpClass
{
    Q_OBJECT
public:
    rtpServer(QObject *parent = 0);
    void rtp_recv_init(int port);
    ~rtpServer();
private:
    void run();


    RTPSession *sess;
    pic_data *picData;
    unsigned char buffer[3276800];
    int prosess_pack(int timestamp,int len,char*buf,int index);
signals:
    void display(IplImage *,int index);
public slots:

};

#endif // RTPSERVER_H
