/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.3.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QPushButton *startButton;
    QLabel *label;
    QLineEdit *ip;
    QLineEdit *port;
    QLabel *label_2;
    QLabel *raw;
    QLabel *remote;
    QPushButton *stopButton;
    QLabel *count1;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(794, 592);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        startButton = new QPushButton(centralWidget);
        startButton->setObjectName(QStringLiteral("startButton"));
        startButton->setGeometry(QRect(10, 400, 75, 23));
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(10, 310, 54, 12));
        ip = new QLineEdit(centralWidget);
        ip->setObjectName(QStringLiteral("ip"));
        ip->setGeometry(QRect(70, 310, 113, 20));
        port = new QLineEdit(centralWidget);
        port->setObjectName(QStringLiteral("port"));
        port->setGeometry(QRect(70, 340, 113, 20));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(10, 340, 54, 12));
        raw = new QLabel(centralWidget);
        raw->setObjectName(QStringLiteral("raw"));
        raw->setGeometry(QRect(10, 10, 381, 271));
        raw->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        remote = new QLabel(centralWidget);
        remote->setObjectName(QStringLiteral("remote"));
        remote->setGeometry(QRect(400, 10, 381, 271));
        remote->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        stopButton = new QPushButton(centralWidget);
        stopButton->setObjectName(QStringLiteral("stopButton"));
        stopButton->setGeometry(QRect(110, 400, 75, 23));
        count1 = new QLabel(centralWidget);
        count1->setObjectName(QStringLiteral("count1"));
        count1->setGeometry(QRect(330, 290, 54, 12));
        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 794, 23));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        startButton->setText(QApplication::translate("MainWindow", "\345\274\200\345\247\213", 0));
        label->setText(QApplication::translate("MainWindow", "ip:", 0));
        ip->setText(QApplication::translate("MainWindow", "127.0.0.1", 0));
        port->setText(QApplication::translate("MainWindow", "8554", 0));
        label_2->setText(QApplication::translate("MainWindow", "port:", 0));
        raw->setText(QString());
        remote->setText(QString());
        stopButton->setText(QApplication::translate("MainWindow", "\345\201\234\346\255\242", 0));
        count1->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
