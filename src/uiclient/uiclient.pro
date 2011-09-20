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
		} else {
			system(mv macCapture ../../)
		}
	}
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








