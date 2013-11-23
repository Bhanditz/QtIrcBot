#-------------------------------------------------
#
# Project created by QtCreator 2013-11-01T14:44:52
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = QtIrcBot
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    ircsocket.cpp \
    quotebot.cpp \
    channelusermodequery.cpp

HEADERS += \
    ircsocket.h \
    quotebot.h \
    channelusermodequery.h

QMAKE_CXXFLAGS += -std=c++11
