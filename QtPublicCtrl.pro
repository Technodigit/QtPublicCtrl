#-------------------------------------------------
#
# Project created by QtCreator 2016-03-16T11:22:55
#
#-------------------------------------------------

QT       += widgets

TARGET = QtPublicCtrl
TEMPLATE = lib

DEFINES += QTPUBLICCTRL_LIBRARY

SOURCES += \
    qtcolorpicker.cpp

HEADERS +=\
    qtcolorpicker.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
