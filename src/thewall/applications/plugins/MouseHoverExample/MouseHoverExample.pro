#-------------------------------------------------
#
# Project created by QtCreator 2011-09-29T00:05:52
#
#-------------------------------------------------

QT       += core gui

TARGET = MouseHoverExamplePlugin
TEMPLATE = lib
CONFIG += plugin
DESTDIR = $$(HOME)/.sagenext/plugins

BUILD_DIR = build
!exists($$BUILD_DIR) {
	system(mkdir build)
}
MOC_DIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR

SOURCES += mousehoverexample.cpp \
../../base/basewidget.cpp \
../../base/appinfo.cpp \
../../base/perfmonitor.cpp \
../../../common/commonitem.cpp

HEADERS  += mousehoverexample.h \
../../base/dummyplugininterface.h \
../../base/basewidget.h \
../../base/appinfo.h \
../../base/perfmonitor.h \
../../../common/commonitem.h

