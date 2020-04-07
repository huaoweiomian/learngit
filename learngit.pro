QT += core network
QT -= gui

CONFIG += c++11

TARGET = learngit
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
INCLUDEPATH += /usr/local/include
LIBS += /usr/local/lib/libmetis.a
SOURCES += \
    gtree_build.cpp \
    gtree_query.cpp \
    main.cpp \
    global.cpp \
    httpserver.cpp

HEADERS += \
    gtree.h \
    httpserver.h

