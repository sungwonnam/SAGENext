#include "sn_streamdisplay.h"
#include "../common/commonitem.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"

SN_StreamDisplay::SN_StreamDisplay(const quint64 globalAppId, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags, ImageBuffer* buf)
    :SN_DisplayStreamBase(globalAppId, s, rm, parent, wFlags, buf)
{
    _currCall = NULL;
     if (_perfMon) {
        //
        // maybe treat this widget as non-periodic
        //
        _perfMon->setExpectedFps( (qreal) 30 );
        _perfMon->setAdjustedFps( (qreal) 30 );
    }
}

void SN_StreamDisplay::videoRecvd(QIODevice::OpenMode mode){

    // this module only deals with video recv'd...allow some other module to deal with sending video
    if (mode & QIODevice::ReadOnly) {
        QXmppVideoFormat videoFormat;
        videoFormat.setPixelFormat(QXmppVideoFrame::Format_YUYV);
        _currCall->videoChannel()->setEncoderFormat(videoFormat);
        _recvThread->start();
    }
    else {
        _recvThread->exit(0);
    }
}

void SN_StreamDisplay::callRecvd(QXmppCall* call) {
    _currCall = call;
    QObject::connect(call, SIGNAL(connected()),this,SLOT(call_Connected()));
    QObject::connect(_currCall, SIGNAL(videoModeChanged(QIODevice::OpenMode)), this, SLOT(videoRecvd(QIODevice::OpenMode)));
    _recvThread = new ReceiveThread( _currCall->videoChannel(), _sharedBuffer, dynamic_cast<QObject*>(this));
    _currCall->accept();

}

void SN_StreamDisplay::callStd(QXmppCall* call) {

}

void SN_StreamDisplay::call_connected(){

}

SN_StreamDisplay::~SN_StreamDisplay(){
    delete _currCall;
    delete _recvThread;
}
