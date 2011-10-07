#-------------------------------------------------
#
# Project created by QtCreator 2011-10-07T13:06:33
#
#-------------------------------------------------

QT       += core gui

TARGET = MouseDragExamplePlugin
TEMPLATE = lib
CONFIG += plugin
DESTDIR = $$(HOME)/.sagenext/plugins

SAGENEXT_DIR = $$(HOME)/SAGENext

INCLUDEPATH += $$SAGENEXT_DIR/src/thewall
#DEPENDPATH += $$SAGENEXT_DIR/src/thewall/BUILD

BUILD_DIR = build
!exists($$BUILD_DIR) {
	system(mkdir build)
}
MOC_DIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR

SOURCES += mousedragexample.cpp \
$$SAGENEXT_DIR/src/thewall/applications/base/basewidget.cpp \
$$SAGENEXT_DIR/src/thewall/applications/base/appinfo.cpp \
$$SAGENEXT_DIR/src/thewall/applications/base/perfmonitor.cpp \
$$SAGENEXT_DIR/src/thewall/common/commonitem.cpp

HEADERS  += mousedragexample.h \
$$SAGENEXT_DIR/src/thewall/applications/base/SN_plugininterface.h \
$$SAGENEXT_DIR/src/thewall/applications/base/basewidget.h \
$$SAGENEXT_DIR/src/thewall/applications/base/appinfo.h \
$$SAGENEXT_DIR/src/thewall/applications/base/perfmonitor.h \
$$SAGENEXT_DIR/src/thewall/common/commonitem.h

