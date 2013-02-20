#ifndef SN_GOOGLETALK_H
#define SN_GOOGLETALK_H

#include "base/basewidget.h"
#include "common/sn_layoutwidget.h"

#include "sn_streamdisplay.h"
#include "videostreamclient.h"
#include "sn_videostream.h"
#include "imagebuffer.h"

class SN_GoogleTalk : public SN_BaseWidget
{
Q_OBJECT
public:
    SN_GoogleTalk(const quint64 globalAppId, const QSettings* s, SN_ResourceMonitor* rm = 0, QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);
    ~SN_GoogleTalk();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void haveNewImage(QImage *f);

signals:
    void drawImage(QImage *f);

private:
    QGraphicsLinearLayout* _layout;

    SN_VideoStream* _sendStream;
    ImageBuffer* _cameraBuffer;

    SN_StreamDisplay* _recvStream;
    ImageBuffer* _recvBuffer;
    VideoStreamClient* _client;

};

#endif // SN_GOOGLETALK_H
