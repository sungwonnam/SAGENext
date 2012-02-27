# -------------------------------------------------
# Project created by QtCreator 2010-04-08T11:36:20
# -------------------------------------------------
QT += network opengl webkit
TARGET = sagenext
TEMPLATE = app


#CONFIG += thread
#CONFIG += copy_dir_files

#
# QtWebKit is built separately from WebKit source code using Tools/Scripts/build-webkit --qt --3d-canvas --3d-rendering --accelerated-2d-canvas
#
#WEBKITPATH = $$(HOME)/Downloads/WebKit_SVN/WebKitBuild/Release
#INCLUDEPATH += $$WEBKITPATH/include/QtWebKit
#LIBS += -L$$WEBKITPATH/lib -lQtWebKit

LIBS += -lGL -lGLU

# If unix (linux, mac)
# use pkg-config
unix {
    CONFIG += link_pkgconfig
}


#
# LibVNCServer
#
# Add LIBVNCSERVER_INSTALL_PATH/lib/pkgconfig in PKG_CONFIG_PATH environment variable
#
unix {
# unix includes linux-g++  linux-g++-64  macx   macx-g++   symbian ...
    message("Linking LibVNCServer lib")
    PKGCONFIG += libvncclient


}
#macx {
#    LIBVNCSERVER = ${HOME}/Downloads/LibVNCServer
#    message("Linking with LibVNC lib $$LIBVNCSERVER")
#    INCLUDEPATH += $$LIBVNCSERVER/include
#    LIBS += -L$$LIBVNCSERVER/lib -lvncclient
#}


#
# Poppler-qt4 for PDFviewer
#
# Add QtSDK/Desktop/Qt/474/gcc/lib/pkgconfig in PKG_CONFIG_PATH environment variable
# configure poppler with --enable-poppler-qt4  (you might need to compile/install openjpeg)
# Add POPPLER_INSTALL_PATH/lib/pkgconfig in PKG_CONFIG_PATH
#
unix {
    message("Linking Poppler-qt4")
    PKGCONFIG += poppler-qt4
}

#
# Qwt
#
#QWT_HOME = /home/evl/snam5/Downloads/qwt-5.2
#INCLUDEPATH += $${QWT_HOME}/src
#LIBS += -L$${QWT_HOME}/lib -lqwt


#
# NUMA lib
#
linux-g++|linux-g++-64 {
        message("Linking Linux libnuma library")
        #LIBS += -L/home/evl/snam5/Downloads/numactl-2.0.6 -lnuma
        LIBS += -lnuma
}
macx {
        message("Excluding -lnuma")
        LIBS -= -lnuma
}



BUILD_DIR = BUILD
!exists($$BUILD_DIR) {
        #message("Creating build directory")
        system(mkdir $${BUILD_DIR})
}


MOC_DIR = MOC
OBJECTS_DIR = $${BUILD_DIR}


# Use parenthesis to read from environment variable
MEDIA_ROOT_DIR = $$(HOME)/.sagenext/media
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

SESSIONS_DIR = $$(HOME)/.sagenext/sessions
!exists($$SESSIONS_DIR) {
    system(mkdir $$SESSIONS_DIR)
}


# where to put TARGET file
DESTDIR = ../../
#CONFIG(debug, debug|release):DESTDIR += debug
#CONFIG(release, debug|release):DESTDIR += release

FORMS += \
#        settingdialog.ui \
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
#        settingdialog.cpp \
#	graphicsviewmainwindow.cpp \
        mediastorage.cpp \
        common/commonitem.cpp \
#        common/thumbnailthread.cpp \
        common/imagedoublebuffer.cpp \
        uiserver/uiserver.cpp \
        uiserver/uimsgthread.cpp \
		uiserver/fileserver.cpp \
        system/resourcemonitor.cpp \
        system/sagenextscheduler.cpp \
        system/resourcemonitorwidget.cpp \
        sage/fsManager.cpp \
        sage/fsmanagermsgthread.cpp \
        sage/sageLegacy.cpp \
        applications/webwidget.cpp \
        applications/pixmapwidget.cpp \
        applications/mediabrowser.cpp \
        applications/vncwidget.cpp \
		applications/pdfviewerwidget.cpp \
        applications/base/perfmonitor.cpp \
        applications/base/appinfo.cpp \
        applications/base/basewidget.cpp \
        applications/base/railawarewidget.cpp \
applications/base/sagestreamwidget.cpp \
applications/base/sagepixelreceiver.cpp \
        applications/base/affinityinfo.cpp \
        applications/base/affinitycontroldialog.cpp \
        sagenextscene.cpp \
        sagenextviewport.cpp \
        sagenextlauncher.cpp \
        settingstackeddialog.cpp \
    common/sn_layoutwidget.cpp \
    applications/sn_checker.cpp \
    common/sn_sharedpointer.cpp \
    applications/base/sn_priority.cpp \
    system/prioritygrid.cpp \
    applications/sn_sagestreammplayer.cpp
#    common/sn_drawingwidget.cpp
#    applications/sn_pboexample.cpp

HEADERS += \
#        settingdialog.h \
#	graphicsviewmainwindow.h \
        common/commonitem.h \
        common/commondefinitions.h \
        common/imagedoublebuffer.h \
#        common/thumbnailthread.h \
    common/sn_layoutwidget.h \
    common/sn_sharedpointer.h \
        uiserver/uiserver.h \
        uiserver/uimsgthread.h \
		uiserver/fileserver.h \
        system/resourcemonitor.h \
        system/resourcemonitorwidget.h \
        system/sagenextscheduler.h \
system/prioritygrid.h \
        sage/fsManager.h \
        sage/fsmanagermsgthread.h \
        sage/sagecommondefinitions.h \
        applications/webwidget.h \
        applications/pixmapwidget.h \
        applications/vncwidget.h \
		applications/pdfviewerwidget.h \
applications/mediabrowser.h \
applications/sn_checker.h \
        applications/base/appinfo.h \
        applications/base/perfmonitor.h \
        applications/base/affinityinfo.h \
        applications/base/affinitycontroldialog.h \
        applications/base/basewidget.h \
        applications/base/railawarewidget.h \
applications/base/sagestreamwidget.h \
applications/base/sagepixelreceiver.h \
applications/base/sn_priority.h \
        sagenextscene.h \
        sagenextviewport.h \
        sagenextlauncher.h \
        mediastorage.h \
    settingstackeddialog.h \
    applications/sn_sagestreammplayer.h


#    common/sn_drawingwidget.h \
#    applications/sn_pboexample.h


#    common/filereceivingrunnable.h


























