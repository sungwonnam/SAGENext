#-------------------------------------------------
#
# Project created by QtCreator 2012-02-17T13:25:53
#
#-------------------------------------------------

QT       += core gui

TARGET = snClockPlugin
TEMPLATE = lib
CONFIG += plugin
DESTDIR = $$(HOME)/.sagenext/media/plugins

BUILD_DIR = build
!exists($$BUILD_DIR) {
        system(mkdir build)
}
MOC_DIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR

INCLUDEPATH += ../../../

SOURCES += \
snclock.cpp \
../../base/basewidget.cpp \
../../base/appinfo.cpp \
../../base/perfmonitor.cpp \
../../base/sn_priority.cpp \
../../../common/commonitem.cpp


HEADERS  += \
snclock.h \
../../base/SN_plugininterface.h \
../../base/basewidget.h \
../../base/appinfo.h \
../../base/perfmonitor.h \
../../base/sn_priority.h \
../../../common/commonitem.h
