#-------------------------------------------------
#
# Project created by QtCreator 2011-10-07T13:06:33
#
#-------------------------------------------------

include(../pluginbase.pri)

QT       += core gui network

TARGET = FittsLawTestPlugin
#TEMPLATE = lib
#CONFIG += plugin
#DESTDIR = $$(HOME)/.sagenext/media/plugins

#BUILD_DIR = build
#!exists($$BUILD_DIR) {
#	system(mkdir build)
#}
#MOC_DIR = $$BUILD_DIR
#OBJECTS_DIR = $$BUILD_DIR

#INCLUDEPATH += ../../../


SOURCES += \
fittslawtestplugin.cpp \
fittslawteststreamrecvthread.cpp \
../../../system/resourcemonitor.cpp

HEADERS += \
fittslawtestplugin.h \
fittslawteststreamrecvthread.h \
../../../system/resourcemonitor.h

#../../base/basewidget.cpp \
#../../base/appinfo.cpp \
#../../base/perfmonitor.cpp \
#../../base/sn_priority.cpp \
#../../../common/commonitem.cpp \
#../../../system/resourcemonitor.cpp \
#    fittslawteststreamrecvthread.cpp


#HEADERS  += fittslawtestplugin.h \
#../../base/SN_plugininterface.h \
#../../base/basewidget.h \
#../../base/appinfo.h \
#../../base/perfmonitor.h \
#../../base/sn_priority.h \
#../../../common/commonitem.h \
#../../../system/resourcemonitor.h \
#    fittslawteststreamrecvthread.h

RESOURCES += \
    images.qrc
