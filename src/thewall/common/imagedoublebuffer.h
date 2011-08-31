#ifndef IMAGEDOUBLEBUFFER_H
#define IMAGEDOUBLEBUFFER_H

#include <QtCore>
#include <QImage>
#include <QPixmap>
#include <pthread.h>

class QPaintDevice;


class DoubleBuffer : public QObject
{
	Q_OBJECT
public:
	explicit DoubleBuffer(QObject *parent = 0);
	virtual ~DoubleBuffer();

	void * getFrontBuffer();
	void * getBackBuffer();
	void releaseBackBuffer();
	void swapBuffer();
	void releaseLocks();


	virtual void initBuffer(int width, int height, QImage::Format fmt) = 0;

protected:
	void **doubleBuffer;

	int bufferIndex;
	int queueLength;

	pthread_mutex_t *mutex;
	pthread_cond_t *notEmpty;
	pthread_cond_t *notFull;

//	QMutex *mutex;
//	QReadWriteLock rwlock;
//	QWaitCondition *notEmpty;
//	QWaitCondition *notFull;
};

class ImageDoubleBuffer : public DoubleBuffer
{
	Q_OBJECT
public:
	void initBuffer(int width, int height, QImage::Format fmt);
	void rgbSwapped();
	~ImageDoubleBuffer();
};

class PixmapDoubleBuffer : public DoubleBuffer
{
	Q_OBJECT
public:
	void initBuffer(int width, int height, QImage::Format fmt);
	~PixmapDoubleBuffer();
};

#endif // IMAGEDOUBLEBUFFER_H
