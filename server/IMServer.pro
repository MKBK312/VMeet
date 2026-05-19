QT += core
QT -= gui

CONFIG += c++11

TARGET = im_server
DESTDIR = ./bin

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    source/kernel/ckernel.cpp \
    source/net/tcpserver.cpp \
    source/net/threadpool.cpp \
    source/net/task_handler.cpp \
    source/mediator/tcpservermediator.cpp \
    source/mysql/cmysql.cpp

HEADERS += \
    include/net/def.h \
    include/net/inetmediator.h \
    include/net/tcpserver.h \
    include/net/threadpool.h \
    include/net/task_handler.h \
    include/net/task.h \
    include/mediator/tcpservermediator.h \
    include/mysql/cmysql.h \
    include/kernel/ckernel.h

INCLUDEPATH += ./include
INCLUDEPATH += ./include/net
INCLUDEPATH += ./include/mediator
INCLUDEPATH += ./include/mysql
INCLUDEPATH += ./include/kernel
INCLUDEPATH += /usr/include/mysql

linux {
    LIBS += -lpthread
    LIBS += -lmysqlclient
    LIBS += -lssl
    LIBS += -lcrypto
}

QMAKE_CXXFLAGS += -Wall -Wextra
