INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/BufferReader.h \
    $$PWD/BufferWriter.h \
    $$PWD/Channel.h \
    $$PWD/EventLoop.h \
    $$PWD/SelectTaskScheduler.h \
    $$PWD/SigConnection.h \
    $$PWD/TaskScheduler.h \
    $$PWD/TcpConnection.h \
    $$PWD/TcpSocket.h \
    $$PWD/TimeStamp.h \
    $$PWD/Timer.h

SOURCES += \
    $$PWD/BufferReader.cpp \
    $$PWD/BufferWriter.cpp \
    $$PWD/EventLoop.cpp \
    $$PWD/SelectTaskScheduler.cpp \
    $$PWD/SigConnection.cpp \
    $$PWD/TaskScheduler.cpp \
    $$PWD/TcpConnection.cpp \
    $$PWD/TcpSocket.cpp \
    $$PWD/Timer.cpp
