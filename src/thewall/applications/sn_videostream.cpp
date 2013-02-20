#include "sn_videostream.h"
#include "../common/commonitem.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"

SN_VideoStream::SN_VideoStream(const quint64 globalAppId, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags, ImageBuffer* buf)
    :SN_DisplayStreamBase(globalAppId, s, rm, parent, wFlags, buf)
{
    //_appInfo->setMediaType(SAGENext::MEDIA_TYPE_VNC);
    //What is the widget type for this and how does it affect paint?
    //setWidgetType(SN_BaseWidget::Widget_RealTime);

    // TODO: Gotta change this later...not the right way to do this,
    // have to get this programatically...

    if (_perfMon) {
        //
        // maybe treat this widget as non-periodic
        //
        _perfMon->setExpectedFps( (qreal) 30 );
        _perfMon->setAdjustedFps( (qreal) 30 );
    }

    // THis is necessary to successfully instantiate a thread to capture frames from video device

    _cThread = new CaptureThread(_sharedBuffer, dynamic_cast<QObject*>(this), -1);
    _cThread->start();
    while(_sharedBuffer->getSizeOfImageBuffer() == 0);
    _currFrame = _sharedBuffer->getFrame();
}

/*QRectF SN_VideoStream::boundingRect() const {
    return QRectF(QPointF(0,0), surfaceFormat().sizeHint());
}*/

SN_VideoStream::~SN_VideoStream(){
    // delete object here
}

/*void SN_VideoStream::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
//    Q_UNUSED(option);
//    Q_UNUSED(widget);
    paint(painter, option, widget);
}*/
