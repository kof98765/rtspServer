#include "rtspserver.h"
char const* dateHeader() {
  static char buf[200];
#if !defined(_WIN32_WCE)
  time_t tt = time(NULL);
  strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
#else
  // WinCE apparently doesn't have "time()", "strftime()", or "gmtime()",
  // so generate the "Date:" header a different, WinCE-specific way.
  // (Thanks to Pierre l'Hussiez for this code)
  // RSF: But where is the "Date: " string?  This code doesn't look quite right...
  SYSTEMTIME SystemTime;
  GetSystemTime(&SystemTime);
  WCHAR dateFormat[] = L"ddd, MMM dd yyyy";
  WCHAR timeFormat[] = L"HH:mm:ss GMT\r\n";
  WCHAR inBuf[200];
  DWORD locale = LOCALE_NEUTRAL;

  int ret = GetDateFormat(locale, 0, &SystemTime,
                          (LPTSTR)dateFormat, (LPTSTR)inBuf, sizeof inBuf);
  inBuf[ret - 1] = ' ';
  ret = GetTimeFormat(locale, 0, &SystemTime,
                      (LPTSTR)timeFormat,
                      (LPTSTR)inBuf + ret, (sizeof inBuf) - ret);
  wcstombs(buf, inBuf, wcslen(inBuf));
#endif
  return buf;
}
RtspServer::RtspServer(QObject *parent) :
    QTcpServer(parent)
{
    connect(this,SIGNAL(newConnection()),this,SLOT(newConnect()));
    sdpFile=new QFile;

    sdp=new QString;


}
RtspServer::~RtspServer()
{

    delete sdpFile;
    delete sdp;
}
void RtspServer::newConnect()
{
    if(hasPendingConnections())
    {
        QTcpSocket *tcpClient=nextPendingConnection();

        connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)),
               this, SLOT(displayError(QAbstractSocket::SocketError)));

        connect(tcpClient,SIGNAL(readyRead()),this,SLOT(tcpDate()));
        connect(tcpClient,SIGNAL(aboutToClose()),this,SLOT(closeSocket()));
        tcpClientList.append(tcpClient);
    }
}
void RtspServer::closeSocket()
{
    if (QTcpSocket* dec = dynamic_cast<QTcpSocket*>(sender()))
    {
        for(int i=0;i<tcpClientList.size();i++)
        {
            if(tcpClientList.at(i)==dec)
                tcpClientList.removeAt(i);
        }
    }
}
void RtspServer::initSocked(QString ip,int tcpport)
{

    listen(QHostAddress::LocalHost,tcpport);
    connect(this,SIGNAL(acceptError(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
    connect(this,SIGNAL(newConnection()),this,SLOT(newConnect()));

}
void RtspServer::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug("The host was not found. Please check the "
                         "host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug("The connection was refused by the peer. "
                         "Make sure the server is running, "
                         "and check that the host name and port "
                         "settings are correct.");
        break;
    default:
        qDebug("The following error occurred:");

    }
}
void RtspServer::tcpDate()
{

    if (QTcpSocket* dec = dynamic_cast<QTcpSocket*>(sender()))
    {
       qDebug()<<"{";
      QString buf=dec->readAll();
      qDebug()<<buf;
      qDebug()<<"}";
      //qDebug()<<dec->readAll();
      if(buf.contains("OPTIONS"))
      {

        if(buf.mid(buf.indexOf("CSeq: ")+6,1).toInt()!=2)
            return;

        dec->write("RTSP/1.0 200 OK\r\n");
        dec->write("CSeq: 2\r\n");
        dec->write("{\"Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\"}\r\n",61);
        dec->write("\r\n\r\n");
      }

      if(buf.contains("DESCRIBE"))
      {
         sdpFile->setFileName("rtp.sdp");
         sdpFile->open(QIODevice::ReadOnly);
         *sdp=sdpFile->readAll();
         QString describe= "RTSP/1.0 200 OK\r\nCSeq: %1\r\n"
                           "%2"
                           "Content-Base: %3/\r\n"
                           "Content-Type: application/sdp\r\n"
                           "Content-Length: %4\r\n\r\n"
                           "%5";
         dec->write(describe.arg(3)
                    .arg(dateHeader())
                    .arg("rtsp://127.0.0.1:5500/video")
                    .arg(sdp->length()+describe.length())
                    .arg(sdp->toUtf8().data())
                    .toUtf8().data());
         dec->write("\r\n\r\n");
          qDebug()<<"???";

                   //表示回应的是sdp信息
        sdpFile->close();


      }
    }
}
