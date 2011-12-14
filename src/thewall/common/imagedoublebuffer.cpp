#include "imagedoublebuffer.h"

//#include <QGLBuffer>
//#include <QGLContext>

DoubleBuffer::DoubleBuffer()
	: _doubleBuffer(0)
	, _bufferIndex(0)
	, _queueLength(0)
	, _bufsize(0)
	, _numbuff(2)
	, _mutex(0)
	, _notEmpty(0)
	, _notFull(0)
	, _width(0)
	, _height(0)
	, _format(QImage::Format_Invalid)
{
//	mutex = new QMutex(); // non recursive lock
//	mutex->unlock();

//	notFull = new QWaitCondition();
//	notEmpty = new QWaitCondition();

	_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if ( pthread_mutex_init(_mutex, 0) != 0 ) {
		perror("pthread_mutex_init");
	}
	if ( pthread_mutex_unlock(_mutex) != 0 ) {
		perror("pthread_mutex_unlock");
	}

	_notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	_notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

	if ( pthread_cond_init(_notEmpty, 0) != 0 ) {
		perror("pthread_cond_init");
	}
	if ( pthread_cond_init(_notFull, 0) != 0 ) {
		perror("pthread_cond_init");
	}
}



DoubleBuffer::~DoubleBuffer() {
	if(_mutex) free(_mutex);
	if(_notEmpty) free(_notEmpty);
	if(_notFull) free(_notFull);
}


void DoubleBuffer::releaseLocks() {
//	mutex->lock();
//	queueLength = -1;
//	notEmpty->wakeOne();
//	notFull->wakeOne();
//	mutex->unlock();

	pthread_mutex_lock(_mutex);
	_queueLength = -1;
	pthread_cond_signal(_notEmpty);
	pthread_cond_signal(_notFull);
	pthread_mutex_unlock(_mutex);

}



// CONSUMER
// sageWidget convert from this image
void * DoubleBuffer::getBackBuffer() {
//	mutex->lock();
//	if ( queueLength == 0) {// if it's true, nothing in the back buffer
//		Q_ASSERT(notEmpty && mutex);
//		notEmpty->wait(mutex); // wait until back buffer filled with pixels
//		qDebug("%s() here", __FUNCTION__);
//	}

	if(!_mutex || !_doubleBuffer) return 0;

	pthread_mutex_lock(_mutex);
//	qDebug("%s() : mutex locked", __FUNCTION__);

	while(_queueLength == 0) {
		//std::cerr << "=== waiting for new frames === " << std::endl;
		pthread_cond_wait(_notEmpty, _mutex);
	}
//	qDebug("%s() : buffer isn't empty. producer finished producing so I can consume", __FUNCTION__);

	return _doubleBuffer[1-_bufferIndex];
}

// PRODUCER
// app is writing pixel into this
void * DoubleBuffer::getFrontBuffer() {
//	qDebug() << "front buffer " << bufferIndex;
	return _doubleBuffer[_bufferIndex];
}

void DoubleBuffer::releaseBackBuffer() {
//	queueLength--; // I have consumed one;

//	if (queueLength==0) {// buffer is empty
//		Q_ASSERT(notFull);
//		notFull->wakeOne();
//	}
//	Q_ASSERT(mutex);
//	mutex->unlock();


	_queueLength--;
//	qDebug() << "releaseBackBuffer() queuelen " << queueLength;
	pthread_mutex_unlock(_mutex);
//	qDebug() << "releaseBackBuffer() mutex unlocked";

	if (_queueLength == 0) {
		pthread_cond_signal(_notFull);
//		qDebug() << "releaseBackBuffer() signaled notFull";
	}

}

void DoubleBuffer::swapBuffer() {
//	Q_ASSERT(mutex);
//	mutex->lock(); // Calling this function multiple times on the same mutex from the same thread is allowed if this mutex is a recursive mutex. If this mutex is a non-recursive mutex, this function will dead-lock when the mutex is locked recursively.

//	if (queueLength > 0) {
//		Q_ASSERT(notFull && mutex);
//		notFull->wait(mutex); // Wait until consumer finishes consuming.
//		qDebug("%s() here", __FUNCTION__);
//	}

//	if (queueLength == 0) {
//		bufferIndex = 1 - bufferIndex; // swap buffer
//		queueLength++; // I have produced one so there's something to read now.
//	}
//	else if (queueLength < 0 ) {
//		mutex->unlock();
//		return;
//	}

//	Q_ASSERT(notEmpty);
//	notEmpty->wakeOne(); // Now back buffer is not empty. consumer can read
//	Q_ASSERT(mutex);
//	mutex->unlock();


	pthread_mutex_lock(_mutex);
//	qDebug("%s() : mutex locked", __FUNCTION__);

	while(_queueLength > 0) {
		pthread_cond_wait(_notFull, _mutex);
	}
//	qDebug("%s() : buffer is not full (sageWidget consumed it)", __FUNCTION__);

	if (_queueLength == 0) {
		_bufferIndex = 1 - _bufferIndex;
		_queueLength++;
//		qDebug("%s() : swapped. queuelen is now %d", __FUNCTION__, queueLength);
	}

	pthread_mutex_unlock(_mutex);
//	qDebug("%s() : mutex unlocked", __FUNCTION__);
	pthread_cond_signal(_notEmpty);
//	qDebug("%s() : signaled notEmpty - producer produced", __FUNCTION__);
}



void ImageDoubleBuffer::initBuffer(int width, int height, QImage::Format fmt) {
	_width = width;
	_height = height;
	_format = fmt;

	_doubleBuffer = (void **)malloc(sizeof(QImage *) * _numbuff);

	QImage *img = 0;
	for (int i=0; i<_numbuff; i++) {
		img = new QImage(width, height, fmt);
		_doubleBuffer[i] = (void *) img;
	}
	_bufsize = img->byteCount();
}

void ImageDoubleBuffer::rgbSwapped() {
	if ( ! _doubleBuffer ) return;
	for (int i=0; i<_numbuff; i++)
		*(static_cast<QImage *>(_doubleBuffer[i])) = static_cast<QImage *>(_doubleBuffer[i])->rgbSwapped();
}

ImageDoubleBuffer::~ImageDoubleBuffer() {
	for (int i=0; i<_numbuff; i++) {
		if ( _doubleBuffer && _doubleBuffer[i] ) delete static_cast<QImage *>(_doubleBuffer[i]);
	}
	free(_doubleBuffer);
	fprintf(stderr, "ImageDoubleBuffer::%s() \n", __FUNCTION__);
}

/*
void GLDoubleBuffer::initBuffer(int width, int height, QImage::Format fmt) {
	_width = width;
	_height = height;
	_format = fmt;

	_doubleBuffer = (void **)malloc(sizeof(QGLBuffer *) * _numbuff);

	QGLBuffer *glbuff = 0;
	for (int i=0; i<_numbuff; i++) {
		glbuff = new QGLBuffer(QGLBuffer::PixelUnpackBuffer);
		glbuff->setUsagePattern(QGLBuffer::DynamicDraw);
		_doubleBuffer[i] = (void *) glbuff;

		//
		// make sure there's valid current QGLContext
		//
		if (!glbuff->create()) {
			qCritical("%s() : failed to create glbuffer", __FUNCTION__);
			return;
		}
		if (!glbuff->bind()) {
			qCritical("%s() : failed to bind glbuffer", __FUNCTION__);
			return;
		}
		glbuff->allocate(width * height * 4); // 32 bpp
		glbuff->release();
	}
	_bufsize = width * height * 4;
}

GLDoubleBuffer::~GLDoubleBuffer() {
	QGLBuffer *buf = 0;
	for (int i=0; i<_numbuff; i++) {
		if ( _doubleBuffer && _doubleBuffer[i] ) {
			buf = static_cast<QGLBuffer *>(_doubleBuffer[i]);
			buf->destroy();
			delete buf;
		}
	}
	free(_doubleBuffer);
	fprintf(stderr, "GLDoubleBuffer::%s() \n", __FUNCTION__);
}
*/


void RawDoubleBuffer::initBuffer(int width, int height, QImage::Format fmt) {
	QImage temp(width, height, fmt);

	_width = width;
	_height = height;
	_format = fmt;

	_doubleBuffer = (void **)malloc(sizeof(unsigned char *) * _numbuff);

	for (int i=0; i<_numbuff; i++)
		_doubleBuffer[i] = (void *)malloc(sizeof(unsigned char) * temp.byteCount());

	_bufsize = temp.byteCount();
}

RawDoubleBuffer::~RawDoubleBuffer() {
	for (int i=0; i<_numbuff; i++)
		if ( _doubleBuffer && _doubleBuffer[i] ) free(_doubleBuffer[i]);

	free(_doubleBuffer);
	fprintf(stderr, "RawDoubleBuffer::%s() \n", __FUNCTION__);
}

//void PixmapDoubleBuffer::initBuffer(int width, int height, QImage::Format fmt) {
//	Q_UNUSED(fmt);
//	doubleBuffer = (void **)malloc(sizeof(QPixmap *) * 2);
//	doubleBuffer[0] = new QPixmap(width, height);
//	doubleBuffer[1] = new QPixmap(width, height);
//}

//PixmapDoubleBuffer::~PixmapDoubleBuffer() {
//	if(mutex) free(mutex);
//	if(notEmpty) free(notEmpty);
//	if(notFull) free(notFull);

//	if ( doubleBuffer && doubleBuffer[0] ) delete static_cast<QPixmap *>(doubleBuffer[0]);
//	if ( doubleBuffer && doubleBuffer[1] ) delete static_cast<QPixmap *>(doubleBuffer[1]);

//	free(doubleBuffer);

//	fprintf(stderr, "PixmapDoubleBuffer::%s() \n", __FUNCTION__);
//}
