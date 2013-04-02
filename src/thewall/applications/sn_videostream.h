/*
 * Class takes an input stream from a video device and displays streaming video
 * as a widget on canvas
 *
 */

#ifndef SN_VIDEOSTREAM_H
#define SN_VIDEOSTREAM_H
#include "sn_displaystreambase.h"
#include "capturethread.h"
#include "sendthread.h"

using namespace cv;

class SN_VideoStream: public SN_DisplayStreamBase
{
    Q_OBJECT
public:
    SN_VideoStream(const quint64 globalAppId, const QSettings* s, SN_ResourceMonitor* rm = 0, QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0, ImageBuffer* buf=0);
    ~SN_VideoStream();
    //void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    CaptureThread* _cThread;

};

#endif // SN_VIDEOSTREAM_H
