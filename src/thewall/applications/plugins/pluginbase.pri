QT += core gui

TEMPLATE = lib
CONFIG += plugin


# Upon a successful build, the binary will be placed in DESTDIR
# Run the SAGENext and launch the plugin by choosing the binary in a file dialog (Ctrl-O)
# Or simply use the media browser
DESTDIR = $$(HOME)/.sagenext/media/plugins


# Note that the TARGET variable must be defined already !
# So, put include(../pluginbase.pri) AFTER you define TARGET variable in your project file
PLUGIN_ICON_NAME = lib$${TARGET}.so.picon
exists( $$OUT_PWD/$$PLUGIN_ICON_NAME ) {
    message("An icon for the plugin exists : $$OUT_PWD/$${PLUGIN_ICON_NAME}")
    # copy the plugin icon to the media directory
    QMAKE_POST_LINK = cp $$OUT_PWD/$${PLUGIN_ICON_NAME} $${DESTDIR}
}


# BUILD DIR points to the thewall's BUILD_DIR
# So, the object files (*.o) in that directory will be
# used to compile plugins
BUILD_DIR = ../../../_BUILD
!exists($$BUILD_DIR) {
	system(mkdir $${BUILD_DIR})
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


HEADERS += \
../../base/SN_plugininterface.h \
../../base/sn_basewidget.h \
../../base/sn_appinfo.h \
../../base/sn_perfmonitor.h \
../../base/sn_priority.h \
../../../common/sn_commonitem.h
