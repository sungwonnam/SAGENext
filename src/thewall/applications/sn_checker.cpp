#include "applications/sn_checker.h"

#include "applications/base/sn_appinfo.h"
#include "applications/base/sn_perfmonitor.h"
#include "applications/base/sn_priority.h"
#include "system/sn_resourcemonitor.h"

#include <sys/time.h>
#include <unistd.h>

#include <QGLPixelBuffer>

#ifdef QT5
#include <QtConcurrent>
#include <QOpenGLBuffer>
#else
#include <QtGui>
#include <QGLBuffer>
#endif


WorkerThread::WorkerThread(QObject *parent)
    : QThread(parent)
    , _bufptr(0)
    , _appInfo(0)
    , _perfMon(0)
    , _framerate(24)
    , _numpixel(0)
    , _byteperpixel(0)
    , _red(0)
    , _green(0)
    , _blue(0)
    , _threadEnd(false)

    , _pbomutex(0)
    , _pbocond(0)
    , _bufferReadyForWriting(false)
{
	setTerminationEnabled();
}

void WorkerThread::run() {
	//
	// init perf. monitor timer
	//
	if(_perfMon) {
	}

	writeData();
}

void WorkerThread::writeData() {
	while(!_threadEnd) {
		Q_ASSERT(_appInfo);
		Q_ASSERT(_perfMon);
//		Q_ASSERT(_bufptr);

		qreal latency = 0.0;



		//
		// write new pixel data
		//
		//		QRgb *rgb = (QRgb *)(_image->scanLine(0)); // An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.
		//		for (int i=0; i<numpixel; ++i) {
		//			rgb[i] = qRgb(red, green, blue); // based on 4 Byte per pixel
		//		}

		/*
		for (int k =0 ; k<numpixel; k++) {
		 // QImage::Format_RGB32 : 0xffRRGGBB
		 rgbbuffer[4*k + 3] = 0xff;
		 rgbbuffer[4*k + 2] = srcbuffer[ 4*k + 0]; // red
		 rgbbuffer[4*k + 1] = srcbuffer[ 4*k + 1]; // green
		 rgbbuffer[4*k + 0] = srcbuffer[ 4*k + 2]; // blue
		 */

		if (_pbomutex) {
			pthread_mutex_lock(_pbomutex);
//			qDebug() << "thread waiting";
			while(!_bufferReadyForWriting)
				pthread_cond_wait(_pbocond, _pbomutex);

			_bufferReadyForWriting = false;
//			qDebug() << "thread woken" << _bufptr;
		}

		/*!
		  usleep() must be here. This is very important when using _pbocond.

		  If usleep() is outside of the mutex guard,
          then pthread_cond_signal() can be called (from SN_CheckerGLPBO::scheduleUpdate())
		  before this thread waits on the _pbocond.

		  So, I can make sure pthread_cond_wait() can be called immediately after current iteration
		  by sleeping inside the critical region guarded by mutex
		  */
		::usleep(1000000/_framerate - 1000000*latency);

		gettimeofday(&lats, 0);

		//
		// write new pixel data
		//
		for (int i=0; i<_numpixel; ++i) {
			_bufptr[_byteperpixel * i + 0] = _red;
			_bufptr[_byteperpixel * i + 1] = _green;
			_bufptr[_byteperpixel * i + 2] = _blue;
		}

		emit dataReady();

		if (_pbomutex) {
			pthread_mutex_unlock(_pbomutex);
		}

		gettimeofday(&late, 0);

		updateData();

		if (_perfMon) {
            /*
#if defined(Q_OS_LINUX)
			getrusage(RUSAGE_THREAD, &ru_end);
#elif defined(Q_OS_MAC)
			getrusage(RUSAGE_SELF, &ru_end);
#endif

			latency = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001); // in Second

			// calculate recv() delay
			// QTimer::restart() will be called in this function
			// So, QTimer::start() must have been called somewhere
			_perfMon->updateObservedRecvLatency(_appInfo->frameSizeInByte(), latency, ru_start, ru_end);
			ru_start = ru_end;
            */
		}
	}
}

void WorkerThread::updateData() {
	//	unsigned char red = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
	//	unsigned char green = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
	//	unsigned char blue = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));

	if (_red == 255) {
		if (_green == 255) {
			if (_blue == 255) {
				_red = 0; _green = 0; _blue = 0;
			}
			else {
				++_blue;
				--_green;
			}
		}
		else {
			++_green;
		}
	}
	else {
		++_red;
	}
}




/******************************************************************************************************
  SN_Checker abstract class
  *******************************************************************************((((******************/
SN_Checker::SN_Checker(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_RailawareWidget(appid, s, rm, parent, wFlags)
    , _imgsize(imagesize)
    , _framerate(framerate)
    , _workerThread(new WorkerThread)
{
	_workerThread->setAppInfo(_appInfo);
	_workerThread->setPerfMon(_perfMon);
	_workerThread->setFramerate(framerate);

	_perfMon->setAdjustedFps(_framerate);
	_perfMon->setExpectedFps(_framerate);

	// size() will return valid value
	resize(_imgsize);

	if (!QObject::connect(_workerThread, SIGNAL(dataReady()), this, SLOT(scheduleUpdate())) ) {
		qDebug() << "SN_Checker() : signal connection failed";
	}
}

SN_Checker::~SN_Checker() {
	_rMonitor->removeSchedulableWidget(this);

	if (_workerThread && _workerThread->isRunning()) {
		_workerThread->setThreadEnd();
//		_workerThread->quit(); // will stop the eventloop
		_workerThread->terminate();
		_workerThread->wait(); // blocking

		_workerThread->deleteLater();
	}
}



/******************************************************************************************************
  SN_Checker using OpenGL
  *******************************************************************************((((******************/
SN_CheckerGL::SN_CheckerGL(GLenum pixfmt, const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_Checker(imagesize, framerate, appid, s, rm, parent, wFlags)
    , _image(0)
    , _textureid(0)
    , _pixelFormat(pixfmt)
{
	switch (_pixelFormat) {
	case GL_RGB: {
		qDebug() << "24bpp";
		_image = new QImage(imagesize, QImage::Format_RGB888);
		break;
	}
	case GL_RGBA: {
		qDebug() << "32bpp";
		_image = new QImage(imagesize, QImage::Format_RGB32); // 0xffRRGGBB
		break;
	}
	}
	_appInfo->setFrameSize(_image->width(), _image->height(), _image->depth());

	_workerThread->setNumpixel(_image->width() * _image->height());
	_workerThread->setByteperpixel(_image->depth() / 8);

	//
	//
	// OpenGL related stuff need to be called after the widget has added to the Scene that has OpenGL viewport
	// Because opengl functions needs valid OpenGL context
	//
	//
	QTimer::singleShot(10, this, SLOT(doInit()));
}

SN_CheckerGL::~SN_CheckerGL() {
	glDeleteTextures(1, &_textureid);
}

void SN_CheckerGL::doInit() {

	if (!scene()) {
		qCritical() << "SN_CheckerGL::doInit() : I'm not added to the scene yet";
	}

	QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
	if (!ctx || ctx->device()->paintEngine()->type() != QPaintEngine::OpenGL2 ) {
		qCritical() << "\n\n !! SN_CheckerGL::doInit() : no OpenGL context !!";
	}

	//
	// init opengl related stuff
	//
	glGenTextures(1, &_textureid);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// internal format 2 -> the number of color components in the texture
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, _pixelFormat, _imgsize.width(), _imgsize.height(), 0, _pixelFormat, GL_UNSIGNED_BYTE, (void *)0 /*static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits()*/);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glEnable(GL_TEXTURE_2D);

	//
	// init worker
	//
	_workerThread->setBufPtr(_image->bits()); // now worker is ready to work

	// run the thread
	_workerThread->start();

	// writeData() will call itself continuously (as long as the eventloop of the thread is running)
//	if ( ! QMetaObject::invokeMethod(_workerThread, "writeData", Qt::QueuedConnection) ) {
//		qDebug() << "SN_CheckerGL::doInit() : Failed to invoke Worker::writeData()";
//	}

	scheduleUpdate();
}

void SN_CheckerGL::scheduleUpdate() {
	_perfMon->getUpdtTimer().start();

	const QImage &constImageRef = *_image;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

	// upload texture to GPU
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, _image->width(), _image->height(), _pixelFormat, GL_UNSIGNED_BYTE, constImageRef.bits());

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glEnable(GL_TEXTURE_2D);

	// measure texture uploading delay
	_perfMon->updateUpdateDelay();

	update(); // schedule paint()

//	_workerThread->setBufPtr(_image->bits());
//	QMetaObject::invokeMethod(_workerThread, "writeData", Qt::QueuedConnection);
//	QTimer::singleShot()

}

void SN_CheckerGL::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	if (_perfMon) {
		// this is to measure drawing delay
		_perfMon->getDrawTimer().start();
		//perfMon->startPaintEvent();
	}
	SN_BaseWidget::paint(painter,o,w);

	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {

		painter->beginNativePainting();

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0,            _imgsize.height());   glVertex2f(0, 0);
		glTexCoord2f(_imgsize.width(), _imgsize.height()); glVertex2f(_appInfo->nativeSize().width(), 0);
		glTexCoord2f(_imgsize.width(), 0.0);               glVertex2f(_appInfo->nativeSize().width(), _appInfo->nativeSize().height());
		glTexCoord2f(0.0,            0.0);                 glVertex2f(0, _appInfo->nativeSize().height());
		glEnd();

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glEnable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		qDebug() << "No OpenGL";
	}

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}













/******************************************************************************************************
  SN_Checker using OpenGL PBO
  *******************************************************************************((((******************/
SN_CheckerGLPBO::SN_CheckerGLPBO(GLenum pixfmt, const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_Checker(imagesize, framerate, appid, s, rm, parent, wFlags)
    , _textureid(0)
    , _pixelFormat(pixfmt)
    , __firstFrame(true)
    , _pboBufIdx(0)
    , __bufferMapped(false)
    , _pbomutex(0)
    , _pbobufferready(0)
{
	_workerThread->setNumpixel(_imgsize.width() * _imgsize.height());
	if (_pixelFormat == GL_RGB) {
		_appInfo->setFrameSize(_imgsize.width(), _imgsize.height(), 24);
		_workerThread->setByteperpixel(3);
	}
	else if (_pixelFormat == GL_RGBA) {
		_appInfo->setFrameSize(_imgsize.width(), _imgsize.height(), 32);
		_workerThread->setByteperpixel(4);
	}

	//
	//
	// OpenGL related stuff need to be called after the widget has added to the Scene that has OpenGL viewport
	// Because opengl functions needs valid OpenGL context
	//
	//
	QTimer::singleShot(10, this, SLOT(doInit()));
}

SN_CheckerGLPBO::~SN_CheckerGLPBO() {
//	disconnect();

	if (_workerThread) {
		_workerThread->setThreadEnd();
		_workerThread->setBufferReadyForWriting(true);
		if ( pthread_cond_signal(_pbobufferready) != 0 ) {
			perror("pthread_cond_signal");
			qDebug() << "failed to signal condition var.";
		}
	}

	if (glIsTexture(_textureid))
		glDeleteTextures(1, &_textureid);


	pthread_mutex_unlock(_pbomutex);
	if ( pthread_mutex_destroy(_pbomutex) != 0) {
		qDebug() << "failed to destory pbomutex mutex";
	}

	if ( pthread_cond_destroy(_pbobufferready) != 0 ) {
		qDebug() << "failed to destroy pbobufferready condition variable";
	}
	free(_pbomutex);
	free(_pbobufferready);

    for (int i=0; i<2; i++) {
        _pbobuf[i]->destroy();

#ifdef QT5
        QOpenGLBuffer::release(QOpenGLBuffer::PixelUnpackBuffer);
#else
        QGLBuffer::release(QGLBuffer::PixelUnpackBuffer);
#endif
        delete _pbobuf[i];
    }

	qDebug() << "~SN_CheckerGLPBO";
}

bool SN_CheckerGLPBO::_initPboMutex() {
	_pbomutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if ( pthread_mutex_init(_pbomutex, 0) != 0 ) {
		perror("pthread_mutex_init");
		return false;
	}
	if ( pthread_mutex_unlock(_pbomutex) != 0 ) {
		perror("pthread_mutex_unlock");
		return false;
	}

	_pbobufferready = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	if ( pthread_cond_init(_pbobufferready, 0) != 0 ) {
		perror("pthread_cond_init");
		return false;
	}

	return true;
}

void SN_CheckerGLPBO::doInit() {

	if (!scene()) {
		qCritical() << "SN_CheckerGLPBO::doInit() : I'm not added to the scene yet";
	}

	QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
	if (!ctx || ctx->device()->paintEngine()->type() != QPaintEngine::OpenGL2 ) {
		qCritical() << "\n\n !! SN_CheckerGLPBO::doInit() : no OpenGL context !!";
	}


	glGenTextures(1, &_textureid);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// internal format 2 -> the number of color components in the texture
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, _pixelFormat, _imgsize.width(), _imgsize.height(), 0, _pixelFormat, GL_UNSIGNED_BYTE, (void *)0 /*static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits()*/);
	//glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, _pixelFormat, size().width(), size().height(), 0, _pixelFormat, GL_UNSIGNED_BYTE, (void *)0);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
	glEnable(GL_TEXTURE_2D);

	//
	// init mutex
	//
	if ( ! _initPboMutex() ) {
		qDebug() << "Failed to init mutex !";
	}
	_workerThread->setPboMutex(_pbomutex);
	_workerThread->setPboCondition(_pbobufferready);

    for(int i=0; i<2; i++) {
#ifdef QT5
        _pbobuf[i] = new QOpenGLBuffer(QOpenGLBuffer::PixelUnpackBuffer);
        _pbobuf[i]->setUsagePattern(QOpenGLBuffer::StreamDraw);
#else
        _pbobuf[i] = new QGLBuffer(QGLBuffer::PixelUnpackBuffer);
        _pbobuf[i]->setUsagePattern(QGLBuffer::StreamDraw);
#endif
        if (! _pbobuf[i]->create()) {
            qDebug() << "failed to create pbobuf";
        }
        else {
            _pbobuf[i]->bind();
            _pbobuf[i]->allocate(_appInfo->frameSizeInByte());
            
            _pboIds[i] = _pbobuf[i]->bufferId();
        }
    }
#ifdef QT5
    QOpenGLBuffer::release(QOpenGLBuffer::PixelUnpackBuffer);
#else
    QGLBuffer::release(QGLBuffer::PixelUnpackBuffer);
#endif


	//
	// init worker with invalid buf ptr first
	//
//	_workerThread->setBufPtr((quint8 *)_pbobufarray[_pboBufIdx]); // now worker is ready to work


	// run the thread
	_workerThread->start();

	scheduleUpdate();

	// writeData() will call itself continuously (as long as the eventloop of the thread is running)
//	if ( ! QMetaObject::invokeMethod(_workerThread, "writeData", Qt::QueuedConnection) ) {
//		qDebug() << "SN_CheckerGL::doInit() : Failed to invoke Worker::writeData()";
//	}
}

void SN_CheckerGLPBO::scheduleUpdate() {

	Q_ASSERT(_pbomutex);
	Q_ASSERT(_pbobufferready);
	Q_ASSERT(_appInfo);

	_perfMon->getUpdtTimer().start();

	// flip array index
	int nextbufidx = _pboBufIdx;
	_pboBufIdx = 1 - _pboBufIdx; 

	//
	// unmap previous buffer
	//
	if (!__firstFrame) {
        _pbobuf[nextbufidx]->bind();
        if ( ! _pbobuf[nextbufidx]->unmap() ) {
			qDebug() << "SN_Checker::schedulePboUpdate() : glUnmapBuffer() failed";

		}
	}
	else {
		__firstFrame = false;
	}

	//
	// update texture with the current pbo buffer (nextbufidx)
	//
	glBindTexture(/*GL_TEXTURE_2D*/GL_TEXTURE_RECTANGLE_ARB, _textureid);
    _pbobuf[nextbufidx]->bind();
	glTexSubImage2D(/*GL_TEXTURE_2D */GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, _imgsize.width(), _imgsize.height(), _pixelFormat, GL_UNSIGNED_BYTE, 0);

	//
	// schedule paintEvent
	//
	update();


	//
	// map buffer for writing to (_pboBufIdx)
	//
	//	qDebug() << "map" << _pboBufIdx;
    if ( ! _pbobuf[_pboBufIdx]->bind() ) {
        qDebug() << "bind failed";
    }
    else {
        _pbobuf[_pboBufIdx]->allocate(_appInfo->frameSizeInByte());
    }
#ifdef QT5
    void *ptr = _pbobuf[_pboBufIdx]->map(QOpenGLBuffer::WriteOnly);
#else
	void *ptr = _pbobuf[_pboBufIdx]->map(QGLBuffer::WriteOnly);
#endif
	if (ptr) {
		_pbobufarray[_pboBufIdx] = ptr;

		//
		// signal thread
		//
		pthread_mutex_lock(_pbomutex);

		__bufferMapped = true;

		_workerThread->setBufPtr((quint8 *)ptr);
		_workerThread->setBufferReadyForWriting(true);
//		QMetaObject::invokeMethod(_workerThread, "writeData", Qt::QueuedConnection);

//		qDebug() << "pboBufIdx" << _pboBufIdx << "signaling thread" << ptr;

		pthread_cond_signal(_pbobufferready);
		//qDebug() << QDateTime::currentMSecsSinceEpoch() << "signaled";
		pthread_mutex_unlock(_pbomutex);
	}
	else {
		qCritical() << "SN_CheckerGLPBO::scheduleUpdate() : glMapBuffer failed()";
	}

	//
	// reset GL state
	//
	glBindTexture(/*GL_TEXTURE_2D*/GL_TEXTURE_RECTANGLE_ARB, 0);
#ifdef QT5
    QOpenGLBuffer::release(QOpenGLBuffer::PixelUnpackBuffer);
#else
    QGLBuffer::release(QGLBuffer::PixelUnpackBuffer);
#endif

	_perfMon->updateUpdateDelay();
}

void SN_CheckerGLPBO::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	if (_perfMon) {
		_perfMon->getDrawTimer().start();
		//perfMon->startPaintEvent();
	}
	SN_BaseWidget::paint(painter,o,w);

	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {

		painter->beginNativePainting();

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0,            _imgsize.height());   glVertex2f(0, 0);
		glTexCoord2f(_imgsize.width(), _imgsize.height()); glVertex2f(_appInfo->nativeSize().width(), 0);
		glTexCoord2f(_imgsize.width(), 0.0);               glVertex2f(_appInfo->nativeSize().width(), _appInfo->nativeSize().height());
		glTexCoord2f(0.0,            0.0);                 glVertex2f(0, _appInfo->nativeSize().height());
		glEnd();

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glEnable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		qDebug() << "No OpenGL";
	}

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}


