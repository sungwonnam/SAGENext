# -------------------------------------------------
# Project created by QtCreator 2010-04-08T11:36:20
# -------------------------------------------------
QT += network opengl declarative
TARGET = sagenext
TEMPLATE = app

QT += network opengl

# TODO: ADD CHECK FOR OPENCV and QTXMPP HERE

#packagesExist(qxmpp) {
#  message("Linking qxmpp")
  LIBS += -L./qxmpp-src/src -lqxmpp
#}
#else {
#   error("Missing Package : opencv is required")
#}


packagesExist(opencv) {
  message("Linking opencv")
  LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann
}
else {
   error("Missing Package : opencv is required")
}


QTWEBKIT = $$(QTWEBKIT_DIR)
isEmpty(QTWEBKIT) {
    QT += webkit
}
else {
#
# QtWebKit is built separately from WebKit source code using Tools/Scripts/build-webkit --qt --3d-canvas --3d-rendering --accelerated-2d-canvas
#
    message("Using a custom QtWebKit library: $$(QTWEBKIT)")
    QT -= webkit

    message("$$(QTWEBKIT_DIR)/include/QtWebKit")
    INCLUDEPATH += $$(QTWEBKIT_DIR)/include/QtWebKit
    message("-L$$(QTWEBKIT_DIR)/lib -lQtWebKit")
    LIBS += -L$$(QTWEBKIT_DIR)/lib -lQtWebKit
}


#
# NUMA lib
#
linux-g++|linux-g++-64 {
    message("Using Linux libnuma library")
    LIBS += -lnuma
}


# If unix (linux, mac)
# unix includes linux-g++  linux-g++-64    macx  macx-g++  symbian ...
unix {
    INCLUDEPATH += /usr/include

#
# Use pkg-config
#
    CONFIG += link_pkgconfig

#
# LibVNCServer
# install the package in trusted library directory such as /usr/local
# or
# Add LIBVNCSERVER_INSTALL_PATH/lib/pkgconfig in your PKG_CONFIG_PATH environment variable
#
# If you're to compile libvncserver, then don't forget to include GCrypt support
# ./configure --with-gcrypt
    packagesExist(libvncclient) {
        message("Linking LibVNCServer lib")
    	PKGCONFIG += libvncclient
    }
    else {
        error("Package LibVNCServer doesn't exist !")
        LIBVNCSERVER_LIBS = $$system(libvncserver-config --libs)
        isEmpty(LIBVNCSERVER_LIBS) {
            error("Missing Package : LibVCNServer is required")
        }
        else {
            message("libvncserver-config --libs =>" $$LIBVNCSERVER_LIBS)
            LIBS += $${LIBVNCSERVER_LIBS}
        }
    }



#
# Poppler-qt4 for PDFviewer
#
# Add QtSDK/Desktop/Qt/474/gcc/lib/pkgconfig in PKG_CONFIG_PATH environment variable
# configure poppler with --enable-poppler-qt4  (you might need to compile/install openjpeg)
# Add POPPLER_INSTALL_PATH/lib/pkgconfig in PKG_CONFIG_PATH
#
    packagesExist(poppler-qt4) {
        message("Linking poppler-qt4 lib")
    	PKGCONFIG += poppler-qt4
    }
    else {
        error("Missing Package : poppler-qt4 is required")
    }
} # end of unix{}

macx {
#    LIBVNCSERVER = $$(HOME)/Dev/LibVNCServer
#    message("Linking with LibVNC lib $$LIBVNCSERVER")
#    INCLUDEPATH += $$LIBVNCSERVER/include
#    LIBS += -L$$LIBVNCSERVER/lib -lvncclient
}

#
# Qwt
# Use $$(..) to obtain contents of an environment variable
#
QWT_HOME = $$(HOME)/Dev/qwt-6.0.1
exists( $$QWT_HOME/lib/libqwt.so ) {
    message("Package Qwt is available")
    INCLUDEPATH += $${QWT_HOME}/include
	LIBS += -L$${QWT_HOME}/lib -lqwt
    DEFINES += USE_QWT
}
else {
    warning("Missing Package : Qwt is not available, but continue")
}





BUILD_DIR = _BUILD
!exists($$BUILD_DIR) {
    #message("Creating build directory")
    system(mkdir $${BUILD_DIR})
}

MOC_DIR = $$BUILD_DIR/_MOC
OBJECTS_DIR = $$BUILD_DIR/_OBJ
UI_DIR = $$BUILD_DIR/_UI



# Use parenthesis to read from environment variable
SAGENEXT_USER_DIR = $$(HOME)/.sagenext
MEDIA_ROOT_DIR = $$SAGENEXT_USER_DIR/media
message("Your media root directory : $$MEDIA_ROOT_DIR")

IMAGE_DIR = $$MEDIA_ROOT_DIR/image
VIDEO_DIR = $$MEDIA_ROOT_DIR/video
PDF_DIR = $$MEDIA_ROOT_DIR/pdf
PLUGIN_DIR = $$MEDIA_ROOT_DIR/plugins

!exists($$MEDIA_ROOT_DIR) {
    message("Creating media directories")
    system(mkdir -p $$MEDIA_ROOT_DIR)
    system(mkdir $$IMAGE_DIR)
    system(mkdir $$VIDEO_DIR)
    system(mkdir $$PLUGIN_DIR)
}


!exists($$IMAGE_DIR) {
    system(mkdir $$IMAGE_DIR)
}
!exists($$VIDEO_DIR) {
    system(mkdir $$VIDEO_DIR)
}
!exists($$PDF_DIR) {
	system(mkdir $$PDF_DIR)
}
!exists($$PLUGIN_DIR) {
    system(mkdir $$PLUGIN_DIR)
}

SESSIONS_DIR = $$SAGENEXT_USER_DIR/sessions
!exists($$SESSIONS_DIR) {
    system(mkdir $$SESSIONS_DIR)
}

THUMBNAIL_DIR = $$SAGENEXT_USER_DIR/.thumbnails
message("Thumbnail dir is $${THUMBNAIL_DIR}")
!exists($$THUMBNAIL_DIR) {
    system(mkdir $$THUMBNAIL_DIR)
}



# where to put TARGET file
DESTDIR = ../../
#CONFIG(debug, debug|release):DESTDIR += debug
#CONFIG(release, debug|release):DESTDIR += release

FORMS += \
        applications/base/affinitycontroldialog.ui \
        system/resourcemonitorwidget.ui \
    settingstackeddialog.ui \
    generalsettingdialog.ui \
    systemsettingdialog.ui \
    graphicssettingdialog.ui \
    guisettingdialog.ui \
    screenlayoutdialog.ui


RESOURCES += ../resources.qrc


SOURCES += \
main.cpp \
mediastorage.cpp \
sagenextscene.cpp \
sagenextviewport.cpp \
sagenextlauncher.cpp \
settingstackeddialog.cpp \
#
common/commonitem.cpp \
common/imagedoublebuffer.cpp \
common/sn_layoutwidget.cpp \
common/sn_sharedpointer.cpp \
#
uiserver/uiserver.cpp \
uiserver/uimsgthread.cpp \
uiserver/fileserver.cpp \
#
system/resourcemonitor.cpp \
system/sagenextscheduler.cpp \
system/resourcemonitorwidget.cpp \
system/prioritygrid.cpp \
#
sage/fsManager.cpp \
sage/fsmanagermsgthread.cpp \
sage/sageLegacy.cpp \
#
applications/base/perfmonitor.cpp \
applications/base/appinfo.cpp \
applications/base/basewidget.cpp \
applications/base/railawarewidget.cpp \
applications/base/sagestreamwidget.cpp \
applications/base/sagepixelreceiver.cpp \
applications/base/affinityinfo.cpp \
applications/base/affinitycontroldialog.cpp \
applications/base/sn_priority.cpp \
#
applications/sn_checker.cpp \
applications/sn_sagestreammplayer.cpp \
applications/sn_fittslawtest.cpp \
applications/webwidget.cpp \
applications/pixmapwidget.cpp \
applications/sn_mediabrowser.cpp \
applications/vncwidget.cpp \
#applications/pdfviewerwidget.cpp \
applications/sn_pdfvieweropenglwidget.cpp \
    applications/sn_videostream.cpp \
    applications/capturethread.cpp \
    applications/imagebuffer.cpp \
    applications/videostreamclient.cpp \
    applications/sn_googletalk.cpp \
    applications/sn_displaystreambase.cpp \
    applications/sn_streamdisplay.cpp
# applications/streamdrawthread.cpp


HEADERS += \
sagenextscene.h \
sagenextviewport.h \
sagenextlauncher.h \
mediastorage.h \
settingstackeddialog.h \
#
common/commonitem.h \
common/commondefinitions.h \
common/imagedoublebuffer.h \
common/sn_layoutwidget.h \
common/sn_sharedpointer.h \
#
uiserver/uiserver.h \
uiserver/uimsgthread.h \
uiserver/fileserver.h \
#
system/resourcemonitor.h \
system/resourcemonitorwidget.h \
system/sagenextscheduler.h \
system/prioritygrid.h \
#
sage/fsManager.h \
sage/fsmanagermsgthread.h \
sage/sagecommondefinitions.h \
#
applications/base/appinfo.h \
applications/base/perfmonitor.h \
applications/base/affinityinfo.h \
applications/base/affinitycontroldialog.h \
applications/base/basewidget.h \
applications/base/railawarewidget.h \
applications/base/sagestreamwidget.h \
applications/base/sagepixelreceiver.h \
applications/base/sn_priority.h \
#
applications/webwidget.h \
applications/pixmapwidget.h \
applications/vncwidget.h \
applications/sn_mediabrowser.h \
applications/sn_checker.h \
applications/sn_sagestreammplayer.h \
applications/sn_fittslawtest.h \
#applications/pdfviewerwidget.h \
applications/sn_pdfvieweropenglwidget.h \
    applications/sn_videostream.h \
    applications/capturethread.h \
    applications/imagebuffer.h \
    applications/videostreamclient.h \
    applications/sn_googletalk.h \
    applications/sn_displaystreambase.h \
    applications/sn_streamdisplay.h




























