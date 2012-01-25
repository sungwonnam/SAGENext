#-------------------------------------------------
#
# Project created by QtCreator 2011-10-07T13:06:33
#
#-------------------------------------------------

QT       += core gui

TARGET = MouseDragExamplePlugin
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


SOURCES += mousedragexample.cpp \
../../base/basewidget.cpp \
../../base/appinfo.cpp \
../../base/perfmonitor.cpp \
../../base/interactionmonitor.cpp \
../../../common/commonitem.cpp

HEADERS  += mousedragexample.h \
../../base/SN_plugininterface.h \
../../base/basewidget.h \
../../base/appinfo.h \
../../base/perfmonitor.h \
../../base/interactionmonitor.h \
../../../common/commonitem.h



#SOURCES += mousedragexample.cpp \
#$$SAGENEXT_DIR/src/thewall/applications/base/basewidget.cpp \
#$$SAGENEXT_DIR/src/thewall/applications/base/appinfo.cpp \
#$$SAGENEXT_DIR/src/thewall/applications/base/perfmonitor.cpp \
#$$SAGENEXT_DIR/src/thewall/common/commonitem.cpp

#HEADERS  += mousedragexample.h \
#$$SAGENEXT_DIR/src/thewall/applications/base/SN_plugininterface.h \
#$$SAGENEXT_DIR/src/thewall/applications/base/basewidget.h \
#$$SAGENEXT_DIR/src/thewall/applications/base/appinfo.h \
#$$SAGENEXT_DIR/src/thewall/applications/base/perfmonitor.h \
#$$SAGENEXT_DIR/src/thewall/common/commonitem.h

