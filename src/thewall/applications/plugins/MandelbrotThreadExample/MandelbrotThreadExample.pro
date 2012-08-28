#-------------------------------------------------
#
# Project created by QtCreator 2011-10-07T13:06:33
#
#-------------------------------------------------

include(../pluginbase.pri)

#
# Upon build, the binary will be placed in ${HOME}/.sagenext/media/plugins/
# Run the SAGENext and launch the plugin by choosing the binary in a file open dialog (Ctrl-O)
#
TARGET = MandelbrotExamplePlugin

SOURCES += \
mandelbrotwidget.cpp \
mandelbrotrenderer.cpp

HEADERS  += \
mandelbrotwidget.h \
mandelbrotrenderer.h


