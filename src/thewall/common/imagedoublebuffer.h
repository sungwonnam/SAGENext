#ifndef IMAGEDOUBLEBUFFER_H
#define IMAGEDOUBLEBUFFER_H

#include <QtCore>

#include <QImage>
#include <pthread.h>

//class QPaintDevice;

class DoubleBuffer
{
public:
	DoubleBuffer();
	virtual ~DoubleBuffer();

	void * getFrontBuffer();
	void * getBackBuffer();
	void releaseBackBuffer();
	void swapBuffer();
	void releaseLocks();
	
	virtual void rgbSwapped() {}

	virtual void initBuffer(int width, int height, QImage::Format fmt) = 0;

	inline int imageWidth() const {return _width;}
	inline int imageHeight() const {return _height;}
	inline QImage::Format imageFormat() const {return _format;}
	inline int imageBytecount() const {return _bufsize;}

protected:
	void **_doubleBuffer;

	int _bufferIndex;
	int _queueLength;

	int _bufsize;

	int _numbuff;

	pthread_mutex_t *_mutex;
	pthread_cond_t *_notEmpty;
	pthread_cond_t *_notFull;

//	QMutex *mutex;
//	QReadWriteLock rwlock;
//	QWaitCondition *notEmpty;
//	QWaitCondition *notFull;

	int _width;
	int _height;
	QImage::Format _format;
};

class RawDoubleBuffer : public DoubleBuffer
{
public:
	void initBuffer(int width, int height, QImage::Format fmt);
	void rgbSwapped() {}

	~RawDoubleBuffer();
};

class ImageDoubleBuffer : public DoubleBuffer
{
public:
	void initBuffer(int width, int height, QImage::Format fmt);
	void rgbSwapped();
	~ImageDoubleBuffer();
};

class GLDoubleBuffer : public DoubleBuffer
{
public:
	void initBuffer(int width, int height, QImage::Format fmt);
	~GLDoubleBuffer();
};

//class PixmapDoubleBuffer : public DoubleBuffer
//{
//public:
//	void initBuffer(int width, int height, QImage::Format fmt);
//	~PixmapDoubleBuffer();
//};

#endif // IMAGEDOUBLEBUFFER_H
