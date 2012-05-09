#-------------------------------------------------
#
# Project created by QtCreator 2010-08-05T18:16:04
#
#-------------------------------------------------

QT       += core gui network

TARGET = sagenextpointer
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
#	CONFIG += x86 ppc
#	LIBS += -framework Carbon
	#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
#	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.5.sdk
#LIBS += -framework ApplicationServices
    message("Compiling macCapture")

    !exists(macCapture.c) {
		error(macCapture.c is not available)
	} else {
	    system(gcc -Wall -o macCapture macCapture.c -framework ApplicationServices)
		!exists(macCapture) {
			error(macCapture failed to compiple)
		}
	}
}

macx {
# append & at the end of each command if you want multiple command
QMAKE_POST_LINK = cp macCapture ../../sagenextpointer.app/Contents/MacOS/
}


SOURCES += \
	main.cpp\
    sn_pointerui.cpp
#	sn_pointerui_sendthread.cpp

HEADERS  += \
    sn_pointerui.h
#	sn_pointerui_sendthread.h

FORMS    += \
	sn_pointerui.ui \
	sn_pointerui_conndialog.ui

RESOURCES += \
    resources.qrc








