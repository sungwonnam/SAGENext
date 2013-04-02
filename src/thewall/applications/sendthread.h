#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QThread>
#include <QTimer>
#include "imagebuffer.h"
#include <qxmpp/QXmppRtpChannel.h>

class SendThread : public QThread
{
    Q_OBJECT
public:
    SendThread(ImageBuffer* buffer, QXmppRtpVideoChannel* channel, QObject *parent = 0);
    ~SendThread();

    void stopSendThread();
    
public slots:
    void writeFrames();

private:
    QXmppRtpVideoChannel* send_channel;
    QXmppVideoFrame QImageToQXmppVideoFrame(QImage img);
    QTimer send_timer;
    ImageBuffer* send_buffer;

    // TODO: Consolidate these methods that are used in the receive thread into something that we don't
    // have to repeat
    // Methods for converting between RGB <=> YUV.
    // Taken from GitHub project about a QXmpp sample app
    // HypersayanX VOIP Test

    quint8 clamp(qint32 value);
    quint8 med(quint8 v1, quint8 v2);
    quint8 rgb2y(quint8 r, quint8 g, quint8 b);
    quint8 rgb2u(quint8 r, quint8 g, quint8 b);
    quint8 rgb2v(quint8 r, quint8 g, quint8 b);
    qint32 y2uv(qint32 y, qint32 width);
    qint32 yuv2r(quint8 y, quint8 u, quint8 v);
    qint32 yuv2g(quint8 y, quint8 u, quint8 v);
    qint32 yuv2b(quint8 y, quint8 u, quint8 v);

 protected:
    void run();
};

#endif // SENDTHREAD_H
