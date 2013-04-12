#-------------------------------------------------
#
# Project created by QtCreator 2011-09-29T00:05:52
#
#-------------------------------------------------

#
# Upon build, the binary will be placed in ${HOME}/.sagenext/media/plugins/
# Run the SAGENext and launch the plugin by choosing the binary in a file open dialog (Ctrl-O)
#
TARGET = MouseHoverExamplePlugin

# Note that this must not precedes TARGET
# in order for QMAKE_POST_LINK can use TARGET variable
include(../pluginbase.pri)

SOURCES += \
mousehoverexample.cpp

HEADERS  += \
mousehoverexample.h
