#ifndef IMAGEBUFFER_H
#define IMAGEBUFFER_H

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <QObject>
#include <QMutex>
#include <QSemaphore>
#include <QQueue>
#include <QDebug>
#include <QImage>


/* Is QImage the best tool to do this? */


using namespace cv;

class ImageBuffer : public QObject
{
    Q_OBJECT
public:
    ImageBuffer(int size, bool dropFrame);
    void addFrame(const Mat& frame);
    void addFrame(QImage img);
    void clearBuffer();
    QImage getFrame();
    int getSizeOfImageBuffer();

signals:
    void haveNewImage(QImage* f);

private:
    QImage convertMattoQImage(const Mat &frame);
    QMutex _imageQueueProtect;
    QQueue<QImage> _imageQueue;
    //QSemaphore *freeSlots;
    //QSemaphore *usedSlots;
    QSemaphore *_clearBuffer1;
    QSemaphore *_clearBuffer2;
    int _bufferSize;
    int _numFrames;
    bool _dropFrame;
};

#endif // IMAGEBUFFER_H
