#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "rtpclient.h"
#include "rtpserver.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void countPack();
    void display(IplImage *,int index);
private slots:
    void on_startButton_clicked();

    void on_stopButton_clicked();

private:
    Ui::MainWindow *ui;
    rtpClient *client;
    rtpServer *server;
    void closeEvent();
};

#endif // MAINWINDOW_H
