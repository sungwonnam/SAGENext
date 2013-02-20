#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QGraphicsObject>
#include <QThread>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "imagebuffer.h"

using namespace cv;

class CaptureThread : public QThread
{
    Q_OBJECT
public:
    CaptureThread(ImageBuffer *buffer, QObject* parent, int camera);
    ~CaptureThread();

    bool connectToCamera();
    void disconnectCamera();
    void stopCaptureThread();
    //int getAvgFPS();
    bool isCameraConnected();
    //int getInputSourceWidth();
    //int getInputSourceHeight();

private:
    //void updateFPS(int);
    ImageBuffer* _imageBuffer;
    VideoCapture* _cap;
    Mat _currentFrame;
    //QTime t;
    QMutex _stoppedMutex;
    //int captureTime;
    //int avgFPS;
    //QQueue<int> fps;
    //int sampleNo;
    //int fpsSum;
    int _camid;
    volatile bool _stopped;
protected:
    void run();
};
#endif // CAPTURETHREAD_H
