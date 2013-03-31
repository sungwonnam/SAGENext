#ifndef RECEIVETHREAD_H
#define RECEIVETHREAD_H

#include <QThread>
#include <QXmppRtpChannel.h>
#include "imagebuffer.h"
#include <QTimer>

class ReceiveThread : public QThread
{
    Q_OBJECT
public:
    ReceiveThread(QXmppRtpVideoChannel* chan, ImageBuffer* buf, QObject *parent = 0);
    ~ReceiveThread();

    void stopRecieveThread();
private:
    QXmppRtpVideoChannel *recv_channel;
    QImage QXmppVideoFrameToQImage(QXmppVideoFrame &frame);
    QTimer recv_timer;
    ImageBuffer* recv_buffer;

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

public slots:
    void readFrames();

protected:
    void run();
};

#endif // RECEIVETHREAD_H
