#include "sn_checker.h"

#include "base/appinfo.h"
#include "base/perfmonitor.h"

#include <sys/time.h>

#include <QtGui>

SN_Checker::SN_Checker(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_BaseWidget(appid, s, parent, wFlags)
    , _image(0)
    , _gltexture(0)
    , _init(false)
//    , _glbuffer(0)
//    , _pbuffer(0)
    , _frate(framerate)
{
	_image = new QImage(imagesize , QImage::Format_RGB32);
	qDebug() << "SN_Checker : bytecount" << _image->byteCount() << "Byte";
	qDebug() << "SN_Checker : depth" << _image->depth() << "bpp";
	qDebug() << "SN_Checker : w x h" << _image->width() * _image->height();

	int fmargin = _settings->value("gui/framemargin", 0).toInt();
	resize(_image->width() + fmargin*2, _image->height() + fmargin * 2 );

	_appInfo->setFrameSize(_image->width(), _image->height(), _image->depth());
	_perfMon->setExpectedFps(framerate);
	_perfMon->setAdjustedFps(framerate);

	_timerid = startTimer(1000/framerate);
//	qDebug() << _timerid;

//	connect(&_timer, SIGNAL(timeout()), this, SLOT(doUpdate()));
//	_timer.start(1000/framerate);

	if(_perfMon) {
		_perfMon->getRecvTimer().start(); //QTime::start()
	}


//	if (!QObject::connect(&_fwatcher, SIGNAL(finished()), this, SLOT(close()))) {
//		qCritical() << "connect failed";
//	}
//	QObject::connect(this, SIGNAL(destroyed()), &_fwatcher, SLOT(cancel()));
//	_future = QtConcurrent::run(this, &SN_Checker::recvPixel);
//	_fwatcher.setFuture(_future);
}

SN_Checker::~SN_Checker() {
//	killTimer(_timerid);
	if (_image) delete _image;
//	if (_glbuffer) {
//		_glbuffer->destroy();
//		delete _glbuffer;
//	}

	if (glIsTexture(_gltexture)) {
		glDeleteTextures(1, &_gltexture);
	}
}

void SN_Checker::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {

	if (_perfMon) {
		_perfMon->getDrawTimer().start();
		//perfMon->startPaintEvent();
	}

	SN_BaseWidget::paint(painter,o,w);

	
	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {
		if (glIsTexture(_gltexture)) {

//			painter->beginNativePainting();

//			glBindTexture(GL_TEXTURE_2D, _gltexture);

//			glBegin(GL_QUADS);
//			glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
//			glTexCoord2f(1.0, 1.0); glVertex2f(_image->width(), 0);
//			glTexCoord2f(1.0, 0.0); glVertex2f(_image->width(), _image->height());
//			glTexCoord2f(0.0, 0.0); glVertex2f(0, _image->height());
//			glEnd();

//			painter->endNativePainting();

			QGLWidget *viewportWidget = (QGLWidget *)w;
			QRectF target = QRect(0, 0, _image->width(), _image->height());
			viewportWidget->drawTexture(target, _gltexture);
		}

//		if(_glbuffer && _glbuffer->bufferId()) {
//			QGLWidget *viewportWidget = (QGLWidget *)w;
//			viewportWidget->drawTexture(QPointF(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt()), _glbuffer->bufferId());
//		}

//		if (_pbuffer) {
//			_pbuffer->drawTexture(QPointF(0,0), _dynamic);
//		}
	}
	else {
		if (_image && !_image->isNull()) {
			painter->drawImage(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), *_image);
		}
	}
	

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_Checker::timerEvent(QTimerEvent *e) {

	if (e->timerId() == _timerid) {

		if (!_init) {
			_doInit();
		}

		gettimeofday(&lats, 0);

		_doRecvPixel();

		gettimeofday(&late, 0);
		update();


		if (_perfMon) {
			qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
			_perfMon->updateObservedRecvLatency(_image->byteCount(), networkrecvdelay, ru_start, ru_end);
		}
	}

	SN_BaseWidget::timerEvent(e);
}


void SN_Checker::recvPixel() {
	forever {

		_doRecvPixel();

		update();

		::usleep(1000000 * 1.0/_frate);
	}
}


void SN_Checker::_doInit() {

	Q_ASSERT(_image);

	const QImage &constRef = *_image; // to avoid detach()
	//_gltexture = glContext->bindTexture(constRef, GL_TEXTURE_2D, QGLContext::InvertedYBindOption);

	if (glIsTexture(_gltexture)) {
		glDeleteTextures(1, &_gltexture);
	}
	glGenTextures(1, &_gltexture);
	glBindTexture(GL_TEXTURE_2D, _gltexture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _image->width(), _image->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, constRef.bits());

	glBindTexture(GL_TEXTURE_2D, 0);

	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		qWarning("texture upload failed. error code 0x%x\n", error);
	}

	// check if pbo supported
	bool pboSupported = true;

	if (pboSupported) {
//		glGenBuffersARB(2, pboIds);
	}

	_init = true;
}

void SN_Checker::_doRecvPixel() {

	if (!_image) {
		return;
	}

	//		if (!_glbuffer) {
	//			_glbuffer = new QGLBuffer();
	//			if ( ! _glbuffer->create() ) {
	//				qCritical() << "SN_Checker : couldn't create QGLBuffer";
	//				deleteLater();
	//			}
	//			else {
	//				_glbuffer->allocate(_image->byteCount());
	////				if (!_glbuffer->bind()) {
	////					qCritical() << "SN_Checker::timerEvent() : glbuffer->bind() failed";
	////				}
	//			}
	//		}


	//unsigned char *buffer = _image->bits();

	//int byteperpixel = _image->depth() / 8;
	int numpixel = _image->width() * _image->height();
	//int numpixel = _image->byteCount() / (_image->depth() / 8); // image size (in Byte) / Byte per pixel

	//
	// determine new pixel data
	//
	unsigned char red = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
	unsigned char green = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
	unsigned char blue = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));

	//
	// for each pixel. This loop is expensive because of the internal detach()
	// so use QRgb * instead of unsigned char *
	//
	//			for (int i=0; i<numpixel; i++) {
	//				buffer[byteperpixel * i + 3] = 0xff; // alpha
	//				buffer[byteperpixel * i + 2] = red; // red
	//				buffer[byteperpixel * i + 1] = green; // green
	//				buffer[byteperpixel * i + 0] = blue; // blue
	//			}


	//
	// write new pixel data
	//
	QRgb *rgb = (QRgb *)(_image->scanLine(0)); // An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.
	for (int i=0; i<numpixel; i++) {
		rgb[i] = qRgb(red, green, blue);
	}

	//
	// now _image is complete
	//


	QGLContext *glContext = const_cast<QGLContext *>(QGLContext::currentContext());
	if(glContext) {

		const QImage &constRef = *_image; // to avoid detach()
		//_gltexture = glContext->bindTexture(constRef, GL_TEXTURE_2D, QGLContext::InvertedYBindOption);

		if (glIsTexture(_gltexture)) {
			glDeleteTextures(1, &_gltexture);
		}
		glGenTextures(1, &_gltexture);
		glBindTexture(GL_TEXTURE_2D, _gltexture);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _image->width(), _image->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, constRef.bits());
		glBindTexture(GL_TEXTURE_2D, 0);

		GLenum error = glGetError();
		if(error != GL_NO_ERROR) {
			qWarning("texture upload failed. error code 0x%x\n", error);
		}
	}

	//Q_ASSERT(_glbuffer);
	//_glbuffer->write(0, constRef.bits(), constRef.byteCount());


	/*
 _pbuffer->makeCurrent();
 _gltexture = _pbuffer->bindTexture(*_image);
 _dynamic = _pbuffer->generateDynamicTexture();

 QGLContext *glContext = const_cast<QGLContext *>(QGLContext::currentContext());
 glContext->makeCurrent();
 */
}
