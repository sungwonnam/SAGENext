#include "streamdrawthread.h"
#include <QDebug>
#include <QGraphicsObject>
#include <QGraphicsItem>

StreamDrawThread::StreamDrawThread(QGraphicsItem *parent, ImageBuffer *buf)
    :QThread(),_sharedBuf(buf)
{
}

void StreamDrawThread::run(){
    while(true){
        if(_sharedBuf->getSizeOfImageBuffer() > 0){
            qDebug() << "Painting...";
            p->drawImage(QPoint(0,0), _sharedBuf->getFrame());
        }
    }
}
