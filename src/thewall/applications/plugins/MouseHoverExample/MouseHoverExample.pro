#-------------------------------------------------
#
# Project created by QtCreator 2011-09-29T00:05:52
#
#-------------------------------------------------

QT       += core gui

TARGET = MouseHoverExamplePlugin
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

SOURCES += mousehoverexample.cpp \
../../base/basewidget.cpp \
../../base/appinfo.cpp \
../../base/perfmonitor.cpp \
../../base/interactionmonitor.cpp \
../../../common/commonitem.cpp

HEADERS  += mousehoverexample.h \
../../base/SN_plugininterface.h \
../../base/basewidget.h \
../../base/appinfo.h \
../../base/perfmonitor.h \
../../base/interactionmonitor.h \
../../../common/commonitem.h

