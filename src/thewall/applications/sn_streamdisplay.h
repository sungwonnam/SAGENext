#ifndef SN_STREAMDISPLAY_H
#define SN_STREAMDISPLAY_H

#include "sn_displaystreambase.h"

class SN_StreamDisplay : public SN_DisplayStreamBase
{
    Q_OBJECT
public:
    SN_StreamDisplay(const quint64 globalAppId, const QSettings* s, SN_ResourceMonitor* rm=0, QGraphicsItem* parent=0, Qt::WindowFlags wFlags=0, ImageBuffer* buf=0);
    ~SN_StreamDisplay();
    
};

#endif // SN_STREAMDISPLAY_H
