#include "capturethread.h"

CaptureThread::CaptureThread(ImageBuffer *buffer, QObject* parent, int camera):
    QThread(parent),_imageBuffer(buffer), _camid(camera)
{
    if(connectToCamera()){
        _stopped = false;
    }
}

CaptureThread::~CaptureThread(){

}

void CaptureThread::run(){
    while(_cap->isOpened() && !_stopped){
        (*_cap).operator >>( _currentFrame); // get a new frame from camera
        _imageBuffer->addFrame(_currentFrame);
    }
}

bool CaptureThread::connectToCamera(){
    _cap = new VideoCapture(_camid);
    if(!_cap->isOpened()){

    }
}
void CaptureThread::disconnectCamera() {
    _stopped = true;
    _cap->release();
}

void CaptureThread::stopCaptureThread() {
    disconnectCamera();
}

bool CaptureThread::isCameraConnected() {
    return !_stopped;
}

/*void CaptureThread::updateFPS(int){

}*/
