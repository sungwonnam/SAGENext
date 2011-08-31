

TEMPLATE = lib
TARGET = charmapexample
CONFIG += plugin

INCLUDEPATH += ../../common

HEADERS     = characterwidget.h \
			  mainwindow.h
SOURCES     = characterwidget.cpp \
			  mainwindow.cpp

# install
target.path = ../
#sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS charactermap.pro
#sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/charactermap
INSTALLS += target

#symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
