#include "sn_streamdisplay.h"
#include "../common/commonitem.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"

SN_StreamDisplay::SN_StreamDisplay(const quint64 globalAppId, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags, ImageBuffer *buf)
    :SN_DisplayStreamBase(globalAppId, s, rm, parent, wFlags, buf)
{
    if (_perfMon) {
        //
        // maybe treat this widget as non-periodic
        //
        _perfMon->setExpectedFps( (qreal) 30 );
        _perfMon->setAdjustedFps( (qreal) 30 );
    }

    while(_sharedBuffer->getSizeOfImageBuffer() == 0);
    _currFrame = _sharedBuffer->getFrame();
}

SN_StreamDisplay::~SN_StreamDisplay(){

}
