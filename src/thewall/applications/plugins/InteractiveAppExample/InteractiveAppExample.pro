#-------------------------------------------------
#
# Project created by QtCreator 2010-11-08T12:51:11
#
#-------------------------------------------------

QT       += core gui
TARGET = IAppExample
TEMPLATE = lib
CONFIG += plugin debug
DESTDIR = $$(HOME)/.sagenext/plugins

BUILD_DIR = build
!exists($$BUILD_DIR) {
	system(mkdir build)
}
MOC_DIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR


SOURCES += \
interactiveappexample.cpp \
#../../base/basewidget.cpp \
#../../base/appinfo.cpp \
#../../../common/perfmonitor.cpp \
#../../../common/commonitem.cpp


HEADERS  += \
interactiveappexample.h \
../../base/dummyplugininterface.h \
#../../base/basewidget.h \
#../../base/appinfo.h \
#../../../common/perfmonitor.h \
#../../../common/commonitem.h

FORMS    += interactiveappexample.ui
