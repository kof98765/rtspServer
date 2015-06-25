#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    client=new rtpClient();
    server=new rtpServer();

    server->rtp_recv_init(5000);
    //server->startThread();
    connect(client,SIGNAL(display(IplImage*,int)),this,SLOT(display(IplImage*,int)));
    connect(server,SIGNAL(display(IplImage*,int)),this,SLOT(display(IplImage*,int)));
    connect(client,SIGNAL(heartpack()),this,SLOT(countPack()));
}
void MainWindow::closeEvent()
{

}
MainWindow::~MainWindow()
{
    client->stopThread();
    server->stopThread();

    delete client;
    delete server;
    delete ui;
}
void MainWindow::display(IplImage *dst,int index)
{
    QString str;
    CvSize img_cvsize;
    QPalette palette;
    switch(index)
    {
        case 0:

        img_cvsize.width =ui->raw->width();
        img_cvsize.height = ui->raw->height();
    case 1:

        img_cvsize.width =ui->remote->width();
        img_cvsize.height = ui->remote->height();
    }

    IplImage* img = cvCreateImage(img_cvsize, (dst)->depth, (dst)->nChannels);
    cvResize(dst, img, CV_INTER_LINEAR);


    cvSaveImage("jason2.jpg", img);
    // printf("%d man in the capture\n",m);


    QImage qimage = QImage((uchar *)img->imageData,img_cvsize.width,img_cvsize.height, QImage::Format_RGB888);
    //IplImage为BGR格式，QImage为RGB格式，所以要交换B和R通道显示才正常
    //可以用OpenCV的cvConcertImage函数交换，也可以用QImage的rgbSwapped函数交换；
    qimage = qimage.rgbSwapped();

    switch(index)
    {
        case 0:
            ui->raw->setPixmap(QPixmap::fromImage(qimage));
            break;
        case 1:
            ui->remote->setPixmap(QPixmap::fromImage(qimage));
            cvReleaseImage(&dst);
            break;
    }



    //释放图片内存
    cvReleaseImage(&img);
   // cvReleaseImage(&dst);


}
void MainWindow::countPack()
{
    static int i=0;
    ui->count1->setText(QString::number(i++));

}
void MainWindow::on_startButton_clicked()
{
    client->rtpInit(ui->ip->text(),ui->port->text().toInt());
    client->startThread();
}

void MainWindow::on_stopButton_clicked()
{
    client->stopThread();
}
