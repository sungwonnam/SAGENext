#-------------------------------------------------
#
# Project created by QtCreator 2010-11-09T17:35:01
#
#-------------------------------------------------

QT       += core gui network opengl

TARGET = ImageWidgetPlugin
TEMPLATE = lib
CONFIG += plugin
DESTDIR = $$(HOME)/.sagenext/plugins

BUILD_DIR = build
!exists($$BUILD_DIR) {
	system(mkdir build)
}
MOC_DIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR


SOURCES += \
imagewidgetplugin.cpp \
../../base/basewidget.cpp \
../../base/appinfo.cpp \
../../base/perfmonitor.cpp \
#../../base/affinityinfo.cpp \
#../../../system/resourcemonitor.cpp \
#../../../system/sagenextscheduler.cpp \
#../../../system/affinitycontroldialog.cpp \
../../../common/commonitem.cpp


HEADERS  += \
imagewidgetplugin.h \
../../base/dummyplugininterface.h \
../../base/basewidget.h \
../../base/appinfo.h \
../../base/perfmonitor.h \
#../../base/affinityinfo.h \
#../../../system/resourcemonitor.h \
#../../../system/sagenextscheduler.h \
#../../../system/affinitycontroldialog.h \
../../../common/commonitem.h
