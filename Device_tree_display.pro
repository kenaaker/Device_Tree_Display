#-------------------------------------------------
#
# Project created by QtCreator 2015-11-17T14:05:12
#
#-------------------------------------------------

QT       += core gui

CONFIG   += c++11

INCLUDEPATH += $(HOME)/src/common/include

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Device_tree_display
TEMPLATE = app


SOURCES += main.cpp\
        device_tree_window.cpp \
    treeitem.cpp \
    fdt_model.cpp

HEADERS  += device_tree_window.h \
    treeitem.h \
    fdt_model.h

FORMS    += device_tree_window.ui

LIBS += -lfdt
