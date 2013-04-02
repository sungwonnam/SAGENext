#include "receivethread.h"

ReceiveThread::ReceiveThread(QXmppRtpVideoChannel *chan, ImageBuffer *buf, QObject *parent)
    :QThread(parent), recv_channel(chan), recv_buffer(buf)
{
    recv_timer.setInterval(1);
    QObject::connect(&recv_timer, SIGNAL(timeout()), this, SLOT(readFrames()));
}

ReceiveThread::~ReceiveThread(){

}

void ReceiveThread::stopReceiveThread(){

}

void ReceiveThread::run(){
    while(recv_timer.isActive()){
        recv_timer.start();
    }
}

void ReceiveThread::readFrames(){
    foreach (QXmppVideoFrame frame, recv_channel->readFrames()){
        if (frame.isValid()){
            recv_buffer->addFrame(QXmppVideoFrameToQImage(frame));
        }
    }
}

// Basically copied this method from a example QXmpp application on GitHub that was
// drawing frames onto a QImage

QImage ReceiveThread::QXmppVideoFrameToQImage(QXmppVideoFrame &videoFrame){
    QImage image(videoFrame.size(), QImage::Format_RGB888);

    qint32 width = videoFrame.size().width();
    qint32 height = videoFrame.size().height();

    const quint8 *iBits = (const quint8 *) videoFrame.bits();
    quint8 *oBits = (quint8 *) image.bits();
    const quint8 *yp, *up, *vp;

    switch (videoFrame.pixelFormat())
    {
    case QXmppVideoFrame::Format_YUYV:
        for (qint32 i = 0; i < 2 * width * height; i += 4)
        {
            quint8 y1 = iBits[i];
            quint8 u  = iBits[i + 1];
            quint8 y2 = iBits[i + 2];
            quint8 v  = iBits[i + 3];

            // r1
            *oBits++ = this->clamp(this->yuv2r(y1, u, v));

            // g1
            *oBits++ = this->clamp(this->yuv2g(y1, u, v));

            // b1
            *oBits++ = this->clamp(this->yuv2b(y1, u, v));

            // r2
            *oBits++ = this->clamp(this->yuv2r(y2, u, v));

            // g2
            *oBits++ = this->clamp(this->yuv2g(y2, u, v));

            // b2
            *oBits++ = this->clamp(this->yuv2b(y2, u, v));
        }
        break;

    case QXmppVideoFrame::Format_YUV420P:
        yp = iBits;
        up = yp + width * height;
        vp = up + width * height / 4;

        for (qint32 i = 0; i < width * height; i++)
        {
            quint8 y = yp[i];
            quint8 u = up[this->y2uv(i, width)];
            quint8 v = vp[this->y2uv(i, width)];

            // r
            *oBits++ = this->clamp(this->yuv2r(y, u, v));

            // g
            *oBits++ = this->clamp(this->yuv2g(y, u, v));

            // b
            *oBits++ = this->clamp(this->yuv2b(y, u, v));
        }
        break;

    default:
        break;
    }

    return image;
}

// https://en.wikipedia.org/wiki/YUV {
inline quint8 ReceiveThread::clamp(qint32 value)
{
    return (uchar) ((value > 255)? 255: ((value < 0)? 0: value));
}

inline quint8 ReceiveThread::med(quint8 v1, quint8 v2)
{
    return ((v1 + v2) >> 1);
}

inline quint8 ReceiveThread::rgb2y(quint8 r, quint8 g, quint8 b)
{
    return (( 66  * r + 129 * g + 25  * b + 128) >> 8) + 16;
}

inline quint8 ReceiveThread::rgb2u(quint8 r, quint8 g, quint8 b)
{
    return ((-38  * r - 74  * g + 112 * b + 128) >> 8) + 128;
}

inline quint8 ReceiveThread::rgb2v(quint8 r, quint8 g, quint8 b)
{
    return (( 112 * r - 94  * g - 18  * b + 128) >> 8) + 128;
}

inline qint32 ReceiveThread::y2uv(qint32 y, qint32 width)
{
    return (qint32) (((qint32) (y / width) >> 1) * width / 2.0 + (qint32) ((y % width) / 2.0));
}

inline qint32 ReceiveThread::yuv2r(quint8 y, quint8 u, quint8 v)
{
    Q_UNUSED(u)

    return ((298 * (y - 16) + 409 * (v - 128) + 128) >> 8);
}

inline qint32 ReceiveThread::yuv2g(quint8 y, quint8 u, quint8 v)
{
    return ((298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8);
}

inline qint32 ReceiveThread::yuv2b(quint8 y, quint8 u, quint8 v)
{
    Q_UNUSED(v)

    return ((298 * (y - 16) + 516 * (u - 128) + 128) >> 8);
}
