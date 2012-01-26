#include "sn_checker.h"

#include "base/appinfo.h"
#include "base/perfmonitor.h"

#include <sys/time.h>

#include <QtGui>
#include <QGLPixelBuffer>

SN_Checker::SN_Checker(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_RailawareWidget(appid, s, parent, wFlags)
    , _end(false)
    , _textureid(0)
    , _init(false)
    , _frate(framerate)
    , _useOpenGL(true)
    , _usePbo(false)
    , __firstFrame(true)
    , _pboBufIdx(0)
    , _pbomutex(0)
    , _pbobufferready(0)
{

	setWidgetType(SN_BaseWidget::Widget_RealTime);


	if (QGLPixelBuffer::hasOpenGLPbuffers()) {
		_usePbo = true;
	}

	_pixelFormat = GL_RGB;
	int bpp = 24;


//	int fmargin = _settings->value("gui/framemargin", 0).toInt();
//	resize(_image->width() + fmargin*2, _image->height() + fmargin * 2 );
	resize(imagesize);

	_appInfo->setFrameSize(imagesize.width(), imagesize.height(), bpp);
	_perfMon->setExpectedFps(framerate);
	_perfMon->setAdjustedFps(framerate);


	qDebug() << "SN_Checker : depth" << _appInfo->bitPerPixel() << "bpp";
	qDebug() << "SN_Checker : w x h" << _appInfo->nativeSize() << imagesize.width() << imagesize.height();
	qDebug() << "SN_Checker : bytecount" << _appInfo->frameSizeInByte() << "Byte";


//	connect(&_timer, SIGNAL(timeout()), this, SLOT(doUpdate()));
//	_timer.start(1000/framerate);

	if(_perfMon) {
		_perfMon->getRecvTimer().start(); //QTime::start()
	}


	if (!QObject::connect(&_fwatcher, SIGNAL(finished()), this, SLOT(close()))) {
		qCritical() << "connect failed";
	}


	// _doInit() needs to be called AFTER constructor finishes and this widget is added to the scene
	// otherwise, this widget won't be seeing valid opengl context
	QTimer::singleShot(200, this, SLOT(_doInit()));
}

SN_Checker::~SN_Checker() {
//	killTimer(_timerid);
//	if (_image) delete _image;
//	if (_glbuffer) {
//		_glbuffer->destroy();
//		delete _glbuffer;
//	}

	_end = true;

	if (glIsTexture(_textureid)) {
		glDeleteTextures(1, &_textureid);
	}

	if (_usePbo) {
		pthread_mutex_unlock(_pbomutex);
		if ( pthread_mutex_destroy(_pbomutex) != 0) {
			qDebug() << "failed to destory pbomutex mutex";
		}

		if ( pthread_cond_destroy(_pbobufferready) != 0 ) {
			qDebug() << "failed to destroy pbobufferready condition variable";
		}

		free(_pbomutex);
		free(_pbobufferready);


		glDeleteBuffersARB(2, _pboIds);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	}

	qDebug() << "~SN_Checker()";
}

void SN_Checker::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
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
		glTexCoord2f(0.0,            size().height()); glVertex2f(0, 0);
		glTexCoord2f(size().width(), size().height()); glVertex2f(_appInfo->nativeSize().width(), 0);
		glTexCoord2f(size().width(), 0.0);             glVertex2f(_appInfo->nativeSize().width(), _appInfo->nativeSize().height());
		glTexCoord2f(0.0,            0.0);             glVertex2f(0, _appInfo->nativeSize().height());
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



void SN_Checker::recvPixel() {
	while(!_end) {
		if (_usePbo) {
			// wait for glMapBufferARB
			pthread_mutex_lock(_pbomutex);
			while(!__bufferMapped)
				pthread_cond_wait(_pbobufferready, _pbomutex);
		}

//		qDebug() << "pboBufIdx" << _pboBufIdx << "thread woken up";

		_doRecvPixel();

		if (_usePbo) {
			__bufferMapped = false; // reset flag

//			qDebug() << "done writing emitting signal";

			emit frameReady(); // trigger schedulePboUpdate
			pthread_mutex_unlock(_pbomutex);
		}

		::usleep(1000000 * 1.0/_frate);
	}
}

bool SN_Checker::_initPboMutex() {
	if (!_usePbo) return false;

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

void SN_Checker::_doInit() {

	if (_useOpenGL) {

		glGenTextures(1, &_textureid);

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

		glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// internal format 2 -> the number of color components in the texture
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, _pixelFormat, size().width(), size().height(), 0, _pixelFormat, GL_UNSIGNED_BYTE, (void *)0 /*static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits()*/);
//		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, _pixelFormat, size().width(), size().height(), 0, _pixelFormat, GL_UNSIGNED_BYTE, (void *)0);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glEnable(GL_TEXTURE_2D);

		if ( _usePbo ) {
			//
			// init mutex
			//
			if ( ! _initPboMutex() ) {
				qDebug() << "Failed to init mutex !";
			}

//			qDebug() << "SN_SageStreamWidget : OpenGL pbuffer extension is present. Using PBO doublebuffering";
			glGenBuffersARB(2, _pboIds);

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[0]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, _appInfo->frameSizeInByte(), 0, GL_STREAM_DRAW_ARB);

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[1]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, _appInfo->frameSizeInByte(), 0, GL_STREAM_DRAW_ARB);

			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

			QObject::connect(this, SIGNAL(frameReady()), this, SLOT(schedulePboUpdate()));
			schedulePboUpdate();
		}
	}
	_init = true;

	_future = QtConcurrent::run(this, &SN_Checker::recvPixel);
	_fwatcher.setFuture(_future);
}


void SN_Checker::schedulePboUpdate() {

	Q_ASSERT(_pbomutex);
	Q_ASSERT(_pbobufferready);
	Q_ASSERT(_appInfo);

	_perfMon->getConvTimer().start();

	//
	// flip array index
	//
	_pboBufIdx = (_pboBufIdx + 1) % 2;
	int nextbufidx = (_pboBufIdx + 1) % 2;

	GLenum error = glGetError();

	//
	// unmap previous buffer
	//
	if (!__firstFrame) {
//		qDebug() << "unmap" << nextbufidx;
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[nextbufidx]);
		if ( ! glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB) ) {
			qDebug() << "schedulePboUpdate() : glUnmapBufferARB() failed";
		}
	}
	else {
		__firstFrame = false;
	}


	//
	// update texture with the pbo buffer
	//
//	qDebug() << "update texture" << nextbufidx;
	glBindTexture(/*GL_TEXTURE_2D*/GL_TEXTURE_RECTANGLE_ARB, _textureid);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[nextbufidx]);
	glTexSubImage2D(/*GL_TEXTURE_2D */GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, _appInfo->nativeSize().width(), _appInfo->nativeSize().height(), _pixelFormat, GL_UNSIGNED_BYTE, 0);

	//
	// schedule paintEvent
	//
	update();




	//
	// map buffer
	//
//	qDebug() << "map" << _pboBufIdx;
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[_pboBufIdx]);
	error = glGetError();
	if(error != GL_NO_ERROR) qCritical("glBindBufferARB() error code 0x%x\n", error);

	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, _appInfo->frameSizeInByte(), 0, GL_STREAM_DRAW_ARB);
	error = glGetError();
	if(error != GL_NO_ERROR) qCritical("glBufferDataARB() error code 0x%x\n", error);

	void *ptr = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	error = glGetError();
	if(error != GL_NO_ERROR) qCritical("glMapBufferARB() error code 0x%x\n", error);

	if (ptr) {
		_pbobufarray[_pboBufIdx] = ptr;

		//
		// signal thread
		//
		pthread_mutex_lock(_pbomutex);

		__bufferMapped = true;
//		_receiverThread->flip(_pboBufIdx);

//		qDebug() << "pboBufIdx" << _pboBufIdx << "signaling thread";

		pthread_cond_signal(_pbobufferready);
	//	qDebug() << QDateTime::currentMSecsSinceEpoch() << "signaled";
		pthread_mutex_unlock(_pbomutex);
	}
	else {
		qCritical() << "glMapBUffer failed()";
	}



	//
	// reset GL state
	//
	glBindTexture(/*GL_TEXTURE_2D*/GL_TEXTURE_RECTANGLE_ARB, 0);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	_perfMon->updateConvDelay();
}



void SN_Checker::_doRecvPixel() {

//	if (!_image) {
//		qDebug() << "doRecvPixel() : image is null. returning";
//		return;
//	}

	unsigned char *bufptr = (unsigned char *)_pbobufarray[_pboBufIdx];
	//unsigned char *buffer = _image->bits();

	//int byteperpixel = _image->depth() / 8;
//	int numpixel = _appInfo->nativeSize().width() * _appInfo->nativeSize().height();
	//int numpixel = _image->byteCount() / (_image->depth() / 8); // image size (in Byte) / Byte per pixel

	//
	// determine new pixel data
	//
	unsigned char red = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
	unsigned char green = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
	unsigned char blue = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));

	//
	// write new pixel data
	//
//	QRgb *rgb = (QRgb *)(_image->scanLine(0)); // An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.
	for (int i=0; i<_appInfo->frameSizeInByte(); i++) {
//		rgb[i] = qRgb(red, green, blue);
		bufptr[i] = red;
	}
}


void SN_Checker::endThread() {
	_end = true;
}
