#include "sendthread.h"

SendThread::SendThread(ImageBuffer *buffer, QXmppRtpVideoChannel *channel, QObject *parent)
    :QThread(parent), send_buffer(buffer), send_channel(channel)
{
    send_timer.setInterval(1);
    QObject::connect(&send_timer, SIGNAL(timeout()), this, SLOT(writeFrames()));
}

SendThread::~SendThread(){

}

void SendThread::stopSendThread(){

}

void SendThread::run(){
    while(send_timer.isActive()){
        send_timer.start();
    }
}

void SendThread::writeFrames(){
    if(send_buffer->getSizeOfImageBuffer() != 0){
        send_channel->writeFrame(QImageToQXmppVideoFrame(send_buffer->getFrame()));
    }
}

QXmppVideoFrame SendThread::QImageToQXmppVideoFrame(QImage image){
    QXmppVideoFrame videoFrame(2 * image.width() * image.height(),
                                   image.size(),
                                   2 * image.width(),
                                   QXmppVideoFrame::Format_YUYV);

    const quint8 *iBits = (const quint8 *) image.bits();
    quint8 *oBits = (quint8 *) videoFrame.bits();

    for (qint32 i = 0; i < 3 * image.width() * image.height(); i += 6) {
        quint8 r1 = iBits[i];
        quint8 g1 = iBits[i + 1];
        quint8 b1 = iBits[i + 2];

        quint8 r2 = iBits[i + 3];
        quint8 g2 = iBits[i + 4];
        quint8 b2 = iBits[i + 5];

        // y1
        *oBits++ = this->rgb2y(r1, g1, b1);

        // u
        *oBits++ = this->rgb2u(this->med(r1, r2), this->med(g1, g2), this->med(b1, b2));

        // y2
        *oBits++ = this->rgb2y(r2, g2, b2);

        // v
        *oBits++ = this->rgb2v(this->med(r1, r2), this->med(g1, g2), this->med(b1, b2));
    }

    return videoFrame;
}

// https://en.wikipedia.org/wiki/YUV {
inline quint8 SendThread::clamp(qint32 value)
{
    return (uchar) ((value > 255)? 255: ((value < 0)? 0: value));
}

inline quint8 SendThread::med(quint8 v1, quint8 v2)
{
    return ((v1 + v2) >> 1);
}

inline quint8 SendThread::rgb2y(quint8 r, quint8 g, quint8 b)
{
    return (( 66  * r + 129 * g + 25  * b + 128) >> 8) + 16;
}

inline quint8 SendThread::rgb2u(quint8 r, quint8 g, quint8 b)
{
    return ((-38  * r - 74  * g + 112 * b + 128) >> 8) + 128;
}

inline quint8 SendThread::rgb2v(quint8 r, quint8 g, quint8 b)
{
    return (( 112 * r - 94  * g - 18  * b + 128) >> 8) + 128;
}

inline qint32 SendThread::y2uv(qint32 y, qint32 width)
{
    return (qint32) (((qint32) (y / width) >> 1) * width / 2.0 + (qint32) ((y % width) / 2.0));
}

inline qint32 SendThread::yuv2r(quint8 y, quint8 u, quint8 v)
{
    Q_UNUSED(u)

    return ((298 * (y - 16) + 409 * (v - 128) + 128) >> 8);
}

inline qint32 SendThread::yuv2g(quint8 y, quint8 u, quint8 v)
{
    return ((298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8);
}

inline qint32 SendThread::yuv2b(quint8 y, quint8 u, quint8 v)
{
    Q_UNUSED(v)

    return ((298 * (y - 16) + 516 * (u - 128) + 128) >> 8);
}
