#include "imagebuffer.h"

ImageBuffer::ImageBuffer(int bufferSize, bool dropFrame)
    :QObject(),
    _bufferSize(bufferSize),
    _dropFrame(dropFrame)
{
    _numFrames = 0;

    _clearBuffer1 = new QSemaphore(1);
    _clearBuffer2 = new QSemaphore(1);
}

QImage ImageBuffer::convertMattoQImage(const Mat& frame){
    return (QImage(frame.data, frame.size().width, frame.size().height, frame.step, QImage::Format_RGB888).rgbSwapped());
}

void ImageBuffer::addFrame(QImage image) {
    qDebug() << "addFrame(): ImageBuffer -> Adding Frame";
    _clearBuffer1->acquire();
    qDebug() << "addFrame(): ImageBuffer -> Aquired clearBuffer1";
    if(_dropFrame) {
        if(_numFrames < _bufferSize){
            // Try and acquire semaphore to add frame
            _imageQueueProtect.lock();
            qDebug() << "addFrame(): ImageBuffer -> Aquired lockedImageQueueProtect";
            _numFrames++;
            _imageQueue.enqueue(image);
            //qDebug() << "addFrame -> emitting haveNewImage signal";
            emit haveNewImage(&image);
            qDebug() << "addFrame(): ImageBuffer -> Added Frame...Num Frames: " << _numFrames;
            _imageQueueProtect.unlock();
            qDebug() << "addFrame(): ImageBuffer -> Unlock lockedImageQueueProtect";
        }
        else {
            _clearBuffer1->release();
            qDebug() << "addFrame(): ImageBuffer -> Release clearbuffer1";
            clearBuffer();
        }
    }
    _clearBuffer1->release();
    qDebug() << "addFrame(): ImageBuffer -> Release clearbuffer1";
}

void ImageBuffer::addFrame(const Mat& frame) {
    qDebug() << "addFrame(): ImageBuffer -> Adding Frame";
    _clearBuffer1->acquire();
    qDebug() << "addFrame(): ImageBuffer -> Aquired clearBuffer1";
    if(_dropFrame) {
        if(_numFrames < _bufferSize){
            // Try and acquire semaphore to add frame
            _imageQueueProtect.lock();
            qDebug() << "addFrame(): ImageBuffer -> Aquired lockedImageQueueProtect";
            _numFrames++;
            QImage tmp = convertMattoQImage(frame);
            _imageQueue.enqueue(tmp);
            //qDebug() << "addFrame -> emitting haveNewImage signal";
            emit haveNewImage(&tmp);
            qDebug() << "addFrame(): ImageBuffer -> Added Frame...Num Frames: " << _numFrames;
            _imageQueueProtect.unlock();
            qDebug() << "addFrame(): ImageBuffer -> Unlock lockedImageQueueProtect";
        }
        else {
            _clearBuffer1->release();
            qDebug() << "addFrame(): ImageBuffer -> Release clearbuffer1";
            clearBuffer();
        }
    }
    _clearBuffer1->release();
    qDebug() << "addFrame(): ImageBuffer -> Release clearbuffer1";
}

QImage ImageBuffer::getFrame(){
    QImage tempFrame;

    // Acquire semaphores
    qDebug() << "getFrame(): ImageBuffer -> Getting Frame";
    _clearBuffer2->acquire();
    qDebug() << "getFrame(): ImageBuffer -> Aquired clearbuffer2";
    _imageQueueProtect.lock();
    qDebug() << "getFrame(): ImageBuffer -> Currently hold imagequeprotect lock";
    tempFrame=_imageQueue.dequeue();
    _imageQueueProtect.unlock();
    qDebug() << "getFrame(): ImageBuffer -> Released imagequeprotect lock";
    _clearBuffer2->release();
    qDebug() << "getFrame(): ImageBuffer -> Released clearbuffer2";
    //qDebug() << "tempFrame";
    return tempFrame;
}

void ImageBuffer::clearBuffer() {
    // Check if buffer is not empty
    if(_imageQueue.size()!=0) {

        // Stop adding frames to buffer
        _clearBuffer1->acquire();
        qDebug() << "clearBuffer(): ImageBuffer -> Aquired clearbuffer1";
        // Stop taking frames from buffer
        _clearBuffer2->acquire();
        qDebug() << "clearBuffer(): ImageBuffer -> Aquired clearbuffer2";
        _imageQueue.clear();
        _clearBuffer2->release();
        qDebug() << "clearBuffer(): ImageBuffer -> Released clearbuffer2";
        _clearBuffer1->release();
        qDebug() << "clearBuffer(): ImageBuffer -> Released clearbuffer1";

    }
}

int ImageBuffer::getSizeOfImageBuffer() {
    _imageQueueProtect.lock();
    int g = _imageQueue.size();
    _imageQueueProtect.unlock();
    qDebug() << "getSizeOfImageBuffer(): ImageBuffer -> Getting Size..Size: " <<  g;
    return g;
}
