QT += core gui

greaterThan(QT_MAJOR_VERSION, 4) {
	QT	+= widgets concurrent
	DEFINES += QT5
}

TEMPLATE = lib
CONFIG += plugin
DESTDIR = $$(HOME)/.sagenext/media/plugins

BUILD_DIR = build
!exists($$BUILD_DIR) {
	system(mkdir build)
}

MOC_DIR = $$BUILD_DIR/_MOC
OBJECTS_DIR = $$BUILD_DIR/_OBJ
UI_DIR = $$BUILD_DIR/_UI


INCLUDEPATH += ../../../
DEPENDPATH += ../../../


SOURCES += \
../../base/sn_basewidget.cpp \
../../base/sn_appinfo.cpp \
../../base/sn_perfmonitor.cpp \
../../base/sn_priority.cpp \
../../../common/sn_commonitem.cpp


HEADERS  += \
../../base/SN_plugininterface.h \
../../base/sn_basewidget.h \
../../base/sn_appinfo.h \
../../base/sn_perfmonitor.h \
../../base/sn_priority.h \
../../../common/sn_commonitem.h

