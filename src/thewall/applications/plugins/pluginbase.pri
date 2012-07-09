
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
../../base/basewidget.cpp \
../../base/appinfo.cpp \
../../base/perfmonitor.cpp \
../../base/sn_priority.cpp \
../../../common/commonitem.cpp


HEADERS  += \
../../base/SN_plugininterface.h \
../../base/basewidget.h \
../../base/appinfo.h \
../../base/perfmonitor.h \
../../base/sn_priority.h \
../../../common/commonitem.h

