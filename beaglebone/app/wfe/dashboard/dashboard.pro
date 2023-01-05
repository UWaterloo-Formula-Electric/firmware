QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dashboard
CONFIG   += console
CONFIG += -g
CONFIG   -= app_bundle

TEMPLATE += app

HEADERS += dashboard.h
SOURCES += dashboard.cpp \
    displays/textdisplay.cpp \
    displays/errordisplay.cpp \
    displays/dial.cpp \
    queuedata.cpp
HEADERS += dashboardUI.h
SOURCES += dashboardUI.cpp
SOURCES += main.cpp

