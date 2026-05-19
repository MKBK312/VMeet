QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lWs2_32

INCLUDEPATH += $$PWD/audio $$PWD/video

SOURCES += \
    audio/audio_read.cpp \
    audio/audio_write.cpp \
    video/video_capture.cpp \
    video/screen_capture.cpp \
    Mediator/INetmediator.cpp \
    Mediator/TcpClientMediator.cpp \
    Mediator/UdpNetMediator.cpp \
    chatdialog.cpp \
    ckernel.cpp \
    frienditem.cpp \
    friendlist.cpp \
    logindialog.cpp \
    main.cpp \
    net/TCPClient.cpp \
    net/UDP.cpp \
    room/roomlist.cpp \
    room/roomchat.cpp \
    room/roomcreate.cpp

HEADERS += \
    Mediator/INetmediator.h \
    Mediator/TcpClientMediator.h \
    Mediator/UdpNetMediator.h \
    chatdialog.h \
    ckernel.h \
    frienditem.h \
    friendlist.h \
    logindialog.h \
    net/INet.h \
    net/TCPClient.h \
    net/UDP.h \
    net/def.h \
    room/roomlist.h \
    room/roomchat.h \
    room/roomcreate.h \
    audio/audio_read.h \
    audio/audio_write.h \
    audio/audio_common.h \
    video/video_capture.h \
    video/screen_capture.h

FORMS += \
    chatdialog.ui \
    frienditem.ui \
    friendlist.ui \
    logindialog.ui \
    room/roomlist.ui \
    room/roomchat.ui \
    room/roomcreate.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES += \
    style.qss
