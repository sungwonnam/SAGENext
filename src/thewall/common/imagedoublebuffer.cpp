#include "imagedoublebuffer.h"

DoubleBuffer::DoubleBuffer(QObject *parent) :
	QObject(parent)
{
//	mutex = new QMutex(); // non recursive lock
//	mutex->unlock();

//	notFull = new QWaitCondition();
//	notEmpty = new QWaitCondition();

	mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if ( pthread_mutex_init(mutex, 0) != 0 ) {
		perror("pthread_mutex_init");
	}
	if ( pthread_mutex_unlock(mutex) != 0 ) {
		perror("pthread_mutex_unlock");
	}

	notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

	if ( pthread_cond_init(notEmpty, 0) != 0 ) {
		perror("pthread_cond_init");
	}
	if ( pthread_cond_init(notFull, 0) != 0 ) {
		perror("pthread_cond_init");
	}

	bufferIndex = 0;

//	rwlock.unlock();
	doubleBuffer = 0;
	queueLength = 0;
}



DoubleBuffer::~DoubleBuffer() {
}


void DoubleBuffer::releaseLocks() {
//	mutex->lock();
//	queueLength = -1;
//	notEmpty->wakeOne();
//	notFull->wakeOne();
//	mutex->unlock();

	pthread_mutex_lock(mutex);
	queueLength = -1;
	pthread_cond_signal(notEmpty);
	pthread_cond_signal(notFull);
	pthread_mutex_unlock(mutex);

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

	if(!mutex || !doubleBuffer) return 0;

	pthread_mutex_lock(mutex);
//	qDebug("%s() : mutex locked", __FUNCTION__);

	while(queueLength == 0) {
		//std::cerr << "=== waiting for new frames === " << std::endl;
		pthread_cond_wait(notEmpty, mutex);
	}
//	qDebug("%s() : buffer isn't empty. producer finished producing so I can consume", __FUNCTION__);

	return doubleBuffer[1-bufferIndex];
}

// PRODUCER
// app is writing pixel into this
void * DoubleBuffer::getFrontBuffer() {
//	qDebug() << "front buffer " << bufferIndex;
	return doubleBuffer[bufferIndex];
}

void DoubleBuffer::releaseBackBuffer() {
//	queueLength--; // I have consumed one;

//	if (queueLength==0) {// buffer is empty
//		Q_ASSERT(notFull);
//		notFull->wakeOne();
//	}
//	Q_ASSERT(mutex);
//	mutex->unlock();


	queueLength--;
//	qDebug() << "releaseBackBuffer() queuelen " << queueLength;
	pthread_mutex_unlock(mutex);
//	qDebug() << "releaseBackBuffer() mutex unlocked";

	if (queueLength == 0) {
		pthread_cond_signal(notFull);
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


	pthread_mutex_lock(mutex);
//	qDebug("%s() : mutex locked", __FUNCTION__);

	while(queueLength > 0) {
		pthread_cond_wait(notFull, mutex);
	}
//	qDebug("%s() : buffer is not full (sageWidget consumed it)", __FUNCTION__);

	if (queueLength == 0) {
		bufferIndex = 1 - bufferIndex;
		queueLength++;
//		qDebug("%s() : swapped. queuelen is now %d", __FUNCTION__, queueLength);
	}

	pthread_mutex_unlock(mutex);
//	qDebug("%s() : mutex unlocked", __FUNCTION__);
	pthread_cond_signal(notEmpty);
//	qDebug("%s() : signaled notEmpty - producer produced", __FUNCTION__);
}



void ImageDoubleBuffer::initBuffer(int width, int height, QImage::Format fmt) {
	doubleBuffer = (void **)malloc(sizeof(QImage *) * 2);
	doubleBuffer[0] = new QImage(width, height, fmt);
	doubleBuffer[1] = new QImage(width, height, fmt);
}

void ImageDoubleBuffer::rgbSwapped() {
	if ( ! doubleBuffer ) return;
	*(static_cast<QImage *>(doubleBuffer[0])) = static_cast<QImage *>(doubleBuffer[0])->rgbSwapped();
	*(static_cast<QImage *>(doubleBuffer[1])) = static_cast<QImage *>(doubleBuffer[1])->rgbSwapped();
}

ImageDoubleBuffer::~ImageDoubleBuffer() {
	if(mutex) free(mutex);
	if(notEmpty) free(notEmpty);
	if(notFull) free(notFull);

	if ( doubleBuffer && doubleBuffer[0] ) delete static_cast<QImage *>(doubleBuffer[0]);
	if ( doubleBuffer && doubleBuffer[1] ) delete static_cast<QImage *>(doubleBuffer[1]);

	free(doubleBuffer);

	fprintf(stderr, "ImageDoubleBuffer::%s() \n", __FUNCTION__);
}


void PixmapDoubleBuffer::initBuffer(int width, int height, QImage::Format fmt) {
	Q_UNUSED(fmt);
	doubleBuffer = (void **)malloc(sizeof(QPixmap *) * 2);
	doubleBuffer[0] = new QPixmap(width, height);
	doubleBuffer[1] = new QPixmap(width, height);
}

PixmapDoubleBuffer::~PixmapDoubleBuffer() {
	if(mutex) free(mutex);
	if(notEmpty) free(notEmpty);
	if(notFull) free(notFull);

	if ( doubleBuffer && doubleBuffer[0] ) delete static_cast<QPixmap *>(doubleBuffer[0]);
	if ( doubleBuffer && doubleBuffer[1] ) delete static_cast<QPixmap *>(doubleBuffer[1]);

	free(doubleBuffer);

	fprintf(stderr, "PixmapDoubleBuffer::%s() \n", __FUNCTION__);
}
