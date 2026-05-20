QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lWs2_32 -L$$PWD/third_party/libfacedetection/lib -lfacedetection \
        -L$$PWD/third_party/ffmpeg/lib -lavcodec -lavutil -lswscale -lswresample -lavformat

INCLUDEPATH += $$PWD/audio $$PWD/video $$PWD/third_party/libfacedetection/include \
               $$PWD/third_party/ffmpeg/include

SOURCES += \
    audio/audio_read.cpp \
    audio/audio_write.cpp \
    audio/audio_encoder.cpp \
    audio/audio_decoder.cpp \
    video/video_capture.cpp \
    video/screen_capture.cpp \
    video/face_detector.cpp \
    video/video_encoder.cpp \
    video/video_decoder.cpp \
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
    audio/audio_encoder.h \
    audio/audio_decoder.h \
    audio/audio_common.h \
    video/video_capture.h \
    video/screen_capture.h \
    video/face_detector.h \
    video/video_encoder.h \
    video/video_decoder.h

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

# Auto-copy DLLs to output dir (use shell_path for Windows backslash paths)
FACE_DLL = $$shell_path($$PWD/third_party/libfacedetection/bin/libfacedetection.dll)
AVCODEC_DLL = $$shell_path($$PWD/third_party/ffmpeg/bin/avcodec-58.dll)
AVUTIL_DLL = $$shell_path($$PWD/third_party/ffmpeg/bin/avutil-56.dll)
SWSCALE_DLL = $$shell_path($$PWD/third_party/ffmpeg/bin/swscale-5.dll)
SWRESAMPLE_DLL = $$shell_path($$PWD/third_party/ffmpeg/bin/swresample-3.dll)
AVFORMAT_DLL = $$shell_path($$PWD/third_party/ffmpeg/bin/avformat-58.dll)
DEBUG_DIR = $$shell_path($$OUT_PWD/debug)
RELEASE_DIR = $$shell_path($$OUT_PWD/release)
CONFIG(debug, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FACE_DLL) $$quote($$DEBUG_DIR) $$escape_expand(\\n\\t)
CONFIG(debug, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$AVCODEC_DLL) $$quote($$DEBUG_DIR) $$escape_expand(\\n\\t)
CONFIG(debug, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$AVUTIL_DLL) $$quote($$DEBUG_DIR) $$escape_expand(\\n\\t)
CONFIG(debug, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$SWSCALE_DLL) $$quote($$DEBUG_DIR) $$escape_expand(\\n\\t)
CONFIG(debug, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$SWRESAMPLE_DLL) $$quote($$DEBUG_DIR) $$escape_expand(\\n\\t)
CONFIG(debug, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$AVFORMAT_DLL) $$quote($$DEBUG_DIR) $$escape_expand(\\n\\t)
CONFIG(release, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FACE_DLL) $$quote($$RELEASE_DIR) $$escape_expand(\\n\\t)
CONFIG(release, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$AVCODEC_DLL) $$quote($$RELEASE_DIR) $$escape_expand(\\n\\t)
CONFIG(release, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$AVUTIL_DLL) $$quote($$RELEASE_DIR) $$escape_expand(\\n\\t)
CONFIG(release, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$SWSCALE_DLL) $$quote($$RELEASE_DIR) $$escape_expand(\\n\\t)
CONFIG(release, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$SWRESAMPLE_DLL) $$quote($$RELEASE_DIR) $$escape_expand(\\n\\t)
CONFIG(release, debug|release): QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$AVFORMAT_DLL) $$quote($$RELEASE_DIR) $$escape_expand(\\n\\t)

RESOURCES += \
    res.qrc

DISTFILES += \
    style.qss
