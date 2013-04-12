#-------------------------------------------------
#
# Project created by QtCreator 2011-10-07T13:06:33
#
#-------------------------------------------------

#
# Upon build, the binary will be placed in ${HOME}/.sagenext/media/plugins/
# Run the SAGENext and launch the plugin by choosing the binary in a file open dialog (Ctrl-O)
#
TARGET = MouseDragExamplePlugin

# Note that this must not precedes TARGET
# in order for QMAKE_POST_LINK can use TARGET variable
include(../pluginbase.pri)

SOURCES += \
mousedragexample.cpp

HEADERS  += \
mousedragexample.h
