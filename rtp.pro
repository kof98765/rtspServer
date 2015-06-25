#-------------------------------------------------
#
# Project created by QtCreator 2015-06-12T14:20:58
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rtp
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    rtpclient.cpp \
    rtpserver.cpp \
    rtpclass.cpp \
    h264class.cpp \
    h264/convert.c \
    decodeandsendthread.cpp

HEADERS  += mainwindow.h \
    rtpclient.h \
    rtpserver.h \
    rtpclass.h \
    h264class.h \
    h264/convert.h \
    decodeandsendthread.h

FORMS    += mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/rtp/jrtplib-3.9.1/build/lib/ -ljrtplib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/rtp/jrtplib-3.9.1/build/lib/ -ljrtplib_d
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/rtp/jthread-1.3.1/build/lib/ -ljthread
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/rtp/jthread-1.3.1/build/lib/ -ljthread_d
LIBS+= ws2_32.lib Advapi32.lib libx264.lib
INCLUDEPATH += $$PWD/rtp/jrtplib-3.9.1/build/include/jrtplib3
INCLUDEPATH += $$PWD/rtp/jthread-1.3.1/build/include/jthread
DEPENDPATH +=  $$PWD/rtp/jrtplib-3.9.1/build/include/jrtplib3
DEPENDPATH +=  $$PWD/rtp/jthread-1.3.1/build/include/jthread

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/rtp/jrtplib-3.9.1/build/lib/libjrtplib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/rtp/jrtplib-3.9.1/build/lib/libjrtplib_d.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/rtp/jrtplib-3.9.1/build/lib/jrtplib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/rtp/jrtplib-3.9.1/build/lib/jrtplib_d.lib

INCLUDEPATH +="D:/opencv/build/install/include"
INCLUDEPATH+="D:/opencv/build/install/include/opencv"

#INCLUDEPATH+="h264/h264-lib/common"
INCLUDEPATH+="h264"
INCLUDEPATH+="x264/x264/common"
INCLUDEPATH+="h264/x264"
INCLUDEPATH+="../h264/x264/extras"
QMAKE_LIBDIR+="D:/opencv/build/install/lib"
QMAKE_LIBDIR+="../h264"
QMAKE_LIBDIR+="../h264/x264"

DEFINES-=_MSC_VER
DEFINES +=_CRT_SECURE_NO_WARNINGS
LIBS += -lopencv_core231d \
        -lopencv_video231d \
        -lopencv_highgui231d \
        -lopencv_imgproc231d \
        -lopencv_objdetect231d
