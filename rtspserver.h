#ifndef RTSPSERVER_H
#define RTSPSERVER_H

#include <QObject>
#include <QFile>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <string.h>
#include <stdio.h>
#include <ctype.h> // for "isxdigit()
#include <time.h>
char const* dateHeader();
class RtspServer : public QTcpServer
{
    Q_OBJECT
public:
    RtspServer(QObject *parent = 0);
    ~RtspServer();
protected:
private:
    QList<QTcpSocket *> tcpClientList;
    QString *sdp;
    QFile *sdpFile;

signals:

public slots:
    void closeSocket();
    void newConnect();
    void tcpDate();
    void initSocked(QString ip,int tcpport);
    void displayError(QAbstractSocket::SocketError socketError);

};

#endif // RTCPSERVER_H
