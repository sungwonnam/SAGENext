#-------------------------------------------------
#
# Project created by QtCreator 2010-08-05T18:16:04
#
#-------------------------------------------------

QT       += core gui network

TARGET = sagenextui
TEMPLATE = app

CONFIG += thread
CONFIG += copy_dir_files

BUILDDIR = build
!exists($$BUILDDIR) {
message("Creating build directory")
system(mkdir build)
}

DESTDIR = ../../
MOC_DIR = MOC
OBJECTS_DIR = $${BUILDDIR}

macx {
	#CONFIG += x86 ppc
    #CONFIG += x86
	#LIBS += -framework Carbon
	#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
	#QMAKE_MAC_SDK=/
}


SOURCES += \
	main.cpp\
	externalguimain.cpp \
	sendthread.cpp \
	messagethread.cpp

HEADERS  += \
	externalguimain.h \
	sendthread.h \
	messagethread.h

FORMS    += \
	externalguimain.ui \
	connectiondialog.ui


