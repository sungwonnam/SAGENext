#ifndef STREAMDRAWTHREAD_H
#define STREAMDRAWTHREAD_H

#include <QThread>
#include <QPainter>
#include <QGraphicsItem>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "imagebuffer.h"

class StreamDrawThread : public QThread
{
public:
    StreamDrawThread(QGraphicsItem* parent, ImageBuffer* buf);
    ~StreamDrawThread();
protected:
    void run();

private:
    ImageBuffer* _sharedBuf;
    QPainter* _paint;
/*signals:
    
public slots:
   */
};

#endif // STREAMDRAWTHREAD_H
