#include "sn_pboexample.h"
#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include <sys/time.h>


SN_PBOtexture::SN_PBOtexture(const QSize &imagesize, qreal framerate, const QGLFormat &format, const quint64 appid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_BaseWidget(appid, s, parent, wFlags)
    , _pbo(0)
    , _image(0)
    , _glformat(format)
    , _init(false)
    , _rotTri(0.0)
{

	if (!QGLPixelBuffer::hasOpenGLPbuffers()) {
		qWarning() << "pixelbuffer is not supported";
		deleteLater();
		return;
	}
	_image = new QImage(imagesize, QImage::Format_RGB32); // 0xffRRGGBB, 4Byte per pixel
	_image->fill(qRgb(128, 128, 128));

	// supported only on Linux
//	setAttribute(Qt::WA_PaintOutsidePaintEvent);

	resize(imagesize);

	_appInfo->setFrameSize(imagesize.width(), imagesize.height(), _image->depth());
	_perfMon->setExpectedFps(framerate);
	_perfMon->setAdjustedFps(framerate);

	// init() needs to be called after constructor returns and this object is added to the scene
//	QTimer::singleShot(100, this, SLOT(init()));

	_updateTimer = startTimer(1000 * 1.0/framerate);
}

SN_PBOtexture::~SN_PBOtexture() {
	if (glIsTexture(_textureid)) glDeleteTextures(1, &_textureid);
	if (glIsTexture(_dynamictextureid)) glDeleteTextures(1, &_dynamictextureid);

	if (_pbo) {
		_pbo->releaseFromDynamicTexture();
		delete _pbo;
	}

	if (_image) {
		delete _image;
	}
	qDebug() << "~SN_PBOtexture()";
}

void SN_PBOtexture::timerEvent(QTimerEvent *e) {
	if (e->timerId() == _updateTimer) {
		updateTexture();
	}
}

void SN_PBOtexture::init() {

	// assume constructor has finished
	QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());
	if (!context) {
		qCritical() << "null context";
		deleteLater();
		return;
	}
	if (!context->isValid()) {
		qCritical() << "invalid context";
		deleteLater();
		return;
	}

	_pbo = new QGLPixelBuffer(_image->size(), QGLFormat::defaultFormat(), static_cast<QGLWidget *>(context->device()));
//	_pbo = new QGLPixelBuffer(_image->size(), _glformat); // THIS IS NOT GOING TO WORK

	_dynamictextureid = _pbo->generateDynamicTexture();

	_init = true;
}

void SN_PBOtexture::updateTexture() {
	if (!_init) init();

	struct timeval lats, late;

	uint pixelvalue = qrand() % 128;
	_image->fill(qRgb(pixelvalue, pixelvalue, pixelvalue));
	const QImage *constRef = _image;

	gettimeofday(&lats, 0);

	// drawing onto pbo
	QPainter painter(_pbo);
	painter.drawImage(0, 0, *constRef);

	/*
	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, _textureid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, constRef->width(), constRef->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, constRef->bits());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, constRef->width(), constRef->height(), GL_RGB, GL_UNSIGNED_BYTE, constRef->bits());
	glBindTexture(GL_TEXTURE_2D, 0);
*/

	_pbo->updateDynamicTexture(_dynamictextureid);


	gettimeofday(&late, 0);
	qreal delay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
	qDebug() << delay * 1000 << "msec";
}

void SN_PBOtexture::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	if (_perfMon) {
		_perfMon->getDrawTimer().start();
	}

	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {
		painter->beginNativePainting();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _dynamictextureid);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
		glTexCoord2f(1.0, 1.0); glVertex2f(size().width(), 0);
		glTexCoord2f(1.0, 0.0); glVertex2f(size().width(), size().height());
		glTexCoord2f(0.0, 0.0); glVertex2f(0, size().height());
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		qDebug() << "OpenGL is not available";
	}


	SN_BaseWidget::paint(painter,o,w);

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}














SN_FBOtexture::SN_FBOtexture(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_BaseWidget(appid, s, parent, wFlags)
    , _timerid(0)
    , _init(false)
    , _imgsize(imagesize)
    , _fbo(0)
{
	resize(imagesize);

	_image = new QImage(imagesize, QImage::Format_RGB32);

	_timerid = startTimer(1000 * 1.0 / framerate);
}

SN_FBOtexture::~SN_FBOtexture() {
	killTimer(_timerid);

	if (_image) delete _image;

	if (_fbo) {
		_fbo->release();
		delete _fbo;
	}
}

void SN_FBOtexture::timerEvent(QTimerEvent *e) {
	if (e->timerId() == _timerid) {
		updateTexture();
	}
}

void SN_FBOtexture::init() {

	if (QGLFramebufferObject::hasOpenGLFramebufferBlit()) {
		QGLFramebufferObjectFormat fboformat;
		fboformat.setInternalTextureFormat(GL_RGBA);
		fboformat.setSamples(4); // RGB (QImage::Format_RGB888)
		fboformat.setTextureTarget(GL_TEXTURE_2D);
		fboformat.setAttachment(QGLFramebufferObject::CombinedDepthStencil);

		_fbo = new QGLFramebufferObject(_imgsize, fboformat);
	}
	else {
		_fbo = new QGLFramebufferObject(_imgsize);
		qDebug() << "\n\nNo OpenGL FramebufferBlit\n\n";
	}

	_init = true;
}

void SN_FBOtexture::updateTexture() {
	if (!_init) init();

	struct timeval lats, late;

	uint pixelvalue = qrand() % 128;
	_image->fill(qRgb(pixelvalue, pixelvalue, pixelvalue));

	gettimeofday(&lats, 0);

	QPainter painter(_fbo);
//	_fbo->bind();
	painter.drawImage(0, 0, *_image);
	painter.end();


	gettimeofday(&late, 0);
	qreal delay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
	qDebug() << delay * 1000 << "msec";
}

void SN_FBOtexture::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {

		if (!_fbo) return;

		painter->beginNativePainting();

//		_fbo->bind();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _fbo->texture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
		glTexCoord2f(1.0, 1.0); glVertex2f(size().width(), 0);
		glTexCoord2f(1.0, 0.0); glVertex2f(size().width(), size().height());
		glTexCoord2f(0.0, 0.0); glVertex2f(0, size().height());
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		qDebug() << "OpenGL is not available";
	}


	SN_BaseWidget::paint(painter,option,widget);
}

























SN_GLBufferExample::SN_GLBufferExample(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_BaseWidget(appid, s, parent, wFlags)
    , _context(0)
    , _image(0)
    , _glbuffer(0)
    , _doubleBuffer(0)
    , _init(false)
    , _bufferIndex(0)
    , _imgsize(imagesize)
    , _byteperpixel(4)

    , _glPainter(0)
    , _glThread(this)
{
	resize(_imgsize);

	_perfMon->setExpectedFps(framerate);
	_perfMon->setAdjustedFps(framerate);

	if(_perfMon) {
		_perfMon->getRecvTimer().start(); //QTime::start()
	}

//	_timerid = startTimer(1000/framerate);
	QTimer::singleShot(10, this, SLOT(init()));
}

SN_GLBufferExample::~SN_GLBufferExample() {

	_glPainter->stopThread();
	_glPainter->wait();

	if (glIsTexture(_textureid)) {
		glDeleteTextures(1, &_textureid);
	}

	if (_glbuffer) {
		for (int i=0; i<2; i++) {
			_glbuffer[i]->release();
			_glbuffer[i]->destroy();
			delete _glbuffer[i];
		}
		free(_glbuffer);
	}
}

void SN_GLBufferExample::init() {

	//
	// init application buffer (OpenGL client)
	//
	_image = new QImage(_imgsize, QImage::Format_RGB888);
	const QImage *constRef = _image; // to avoid detach()
	_appInfo->setFrameSize(constRef->width(), constRef->height(), constRef->depth());



	qreal test = 0.90;
	qDebug() << test;

	//
	// init texture object
	//
	glGenTextures(1, &_textureid);
	glBindTexture(GL_TEXTURE_2D, _textureid);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, constRef->width(), constRef->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, constRef->bits());
	glBindTexture(GL_TEXTURE_2D, 0);


	/**
	_glbuffer = (QGLBuffer **)malloc(sizeof(QGLBuffer *) * 2);
	for (int i=0; i<2; i++) {
		_glbuffer[i] = new QGLBuffer(QGLBuffer::PixelUnpackBuffer);
		_glbuffer[i]->setUsagePattern(QGLBuffer::StreamDraw);

		//
		// glGenBufferARB
		//
		if (!_glbuffer[i]->create()) {
			qDebug() << "glbuffer->create() failed";
			deleteLater();
			return;
		}

		//
		// glBindBufferARB
		//
		if (!_glbuffer[i]->bind()) {
			qDebug() << "glbuffer->bind() failed";
		}

		//
		// glBufferDataARB
		//
		_glbuffer[i]->allocate(constRef->byteCount());

		_glbuffer[i]->release();
	}
	**/




	//
	// QGLContext is accessible from this widget ONLY after the constructor returns
	//
	QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());
	Q_ASSERT(context);
	Q_ASSERT(context->isValid());

	/*
	_doubleBuffer = new GLDoubleBuffer();

	if (context && context->isValid())
		_doubleBuffer->initBuffer(_imgsize.width(), _imgsize.height(), QImage::Format_RGB888);
	else {
		qCritical() << "\n\n FUCK \n";
		deleteLater();
		return;
	}
	*/

	QGLWidget *viewportWidget = static_cast<QGLWidget *>(context->device());
	Q_ASSERT(viewportWidget);

	_glPainter = new GLPainter(_image->size(), _image->format(), _textureid, viewportWidget);

	if (! QObject::connect(_glPainter, SIGNAL(updated(GLuint)), this, SLOT(updateTextureBind(GLuint))) ) {
		qCritical("%s::%s() : Failed to connect update() signal and updateTextureBind() slot", metaObject()->className(), __FUNCTION__);
		deleteLater();
		return;
	}

	if ( !QObject::connect(&_glThread, SIGNAL(started()), _glPainter, SLOT(start())) ) {
		qCritical("%s::%s() : Failed to connect _glThread::started() signal and _glPainter::start() slot", metaObject()->className(), __FUNCTION__);
		deleteLater();
		return;
	}
//	_glPainter->moveToThread(&_glThread);
//	_glThread.start();

//	QtConcurrent::run(_glPainter, &GLPainter::start);

	_glPainter->start();

	_init = true;
}



void SN_GLBufferExample::updateTextureBind(GLuint tid) {
//	struct timeval lats, late;

//	if (!_init) init();

	_textureid = tid;

	/*
	_bufferIndex = (_bufferIndex + 1) % 2;
	int nextindex = (_bufferIndex + 1) % 2;

	gettimeofday(&lats, 0);


	QGLBuffer *glbuffer = static_cast<QGLBuffer *>(_doubleBuffer->getBackBuffer());
//	QGLBuffer *glbuffer = _glbuffer[_bufferIndex];
//	Q_ASSERT(glbuffer);

	QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());
	Q_ASSERT(context);
	Q_ASSERT(context->isValid());

	//
	// bind the texture and buffer object
	//
	glBindTexture(GL_TEXTURE_2D, _textureid);
	glbuffer->bind();

	//
	// copy the pixel from the glbuffer to texture object
	// glTexImage2D has been called in init()
	//
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _imgsize.width(), _imgsize.height(), GL_RGB, GL_UNSIGNED_BYTE, 0);

	// reset
	glBindTexture(GL_TEXTURE_2D, 0);
	glbuffer->release();


	_doubleBuffer->releaseBackBuffer();
*/




/*
	//
	// bind the other glbuffer to update pixel
	//
	glbuffer = _glbuffer[nextindex];
	glbuffer->bind();

	uint value = qrand() % 128;
	_image->fill(qRgb(value, value, value));
	const QImage *img = _image;


	gettimeofday(&lats, 0);

	//
	// update glbuffer with new image
	//	 DMA write to GPU memory?
	//
	glbuffer->write(0,  img->bits(), img->byteCount());

//	unsigned char *mapped = (unsigned char *)glbuffer->map(QGLBuffer::WriteOnly);
//	if (mapped) {
//		//
//		// modify pixel
//		//
////		QRgb *rgb = (QRgb *)(_image->scanLine(0)); // An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.
//		for (int i=0; i<_image->byteCount(); i++) {
//			mapped[i] = 128 * nextindex; // THIS IS VERY SLOW !
//		}

//		glbuffer->unmap();
//	}
	glbuffer->release();
	*/

//	gettimeofday(&late, 0);
//	qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
//	qDebug() << networkrecvdelay * 1000 << "msec";
}



void SN_GLBufferExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	if (_perfMon) {
		_perfMon->getDrawTimer().start();
	}

	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {

		if (!glIsTexture(_textureid)) return;

		painter->beginNativePainting();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _textureid);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
		glTexCoord2f(1.0, 1.0); glVertex2f(size().width(), 0);
		glTexCoord2f(1.0, 0.0); glVertex2f(size().width(),size().height());
		glTexCoord2f(0.0, 0.0); glVertex2f(0, size().height());
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		qDebug() << "OpenGL is not available";
	}

	SN_BaseWidget::paint(painter,o,w);

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_GLBufferExample::timerEvent(QTimerEvent *e) {
	if (e->timerId() == _timerid) {

		gettimeofday(&lats, 0);

//		updateTextureBind();

		gettimeofday(&late, 0);
//		update();

		if (_perfMon) {
			qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
			_perfMon->updateObservedRecvLatency(_image->byteCount(), networkrecvdelay, ru_start, ru_end);
//			qDebug() << _perfMon->getCurrRecvLatency() * 1000 << _perfMon->getCurrRecvFps();
		}
	}

	SN_BaseWidget::timerEvent(e);
}



































GLPainter::GLPainter(const QSize &imagesize, QImage::Format imageformat, GLuint tid, QGLWidget *glwidget, QObject *parent)
    : QThread(parent)
    , _glwidget(glwidget)
    , _myGlWidget(0)
    , _end(false)
//    , _doublebuffer(buffer)
    , _textureid(tid)
    , _imgsize(imagesize)
    , _imgformat(imageformat)
{
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

GLPainter::~GLPainter() {
	if (_myGlWidget) delete _myGlWidget;
	qDebug() << "~GLPainter()";
}

void GLPainter::stopThread() {
	_end = true;
}

void GLPainter::run() {

	_myGlWidget = new QGLWidget(QGLFormat::defaultFormat(), 0, _glwidget);
	_myGlWidget->makeCurrent();

	//
	// app buffer
	//
	QImage img(_imgsize, _imgformat);
	const QImage &constImgRef = img;


	//
	// init glbuffer (at the GLServer)
	//
	QGLBuffer **_glbuffer = (QGLBuffer **)malloc(sizeof(QGLBuffer *) * 2);
	for (int i=0; i<2; i++) {
		_glbuffer[i] = new QGLBuffer(QGLBuffer::PixelUnpackBuffer);
		_glbuffer[i]->setUsagePattern(QGLBuffer::StreamDraw);

		//
		// glGenBufferARB
		//
		if (!_glbuffer[i]->create()) {
			qDebug() << "glbuffer->create() failed";
			deleteLater();
			return;
		}

		//
		// glBindBufferARB
		//
		if (!_glbuffer[i]->bind()) {
			qDebug() << "glbuffer->bind() failed";
		}

		//
		// glBufferDataARB
		//
		_glbuffer[i]->allocate(constImgRef.byteCount());

		_glbuffer[i]->release();
	}

	unsigned int pixelvalue = 0;

	int _bufferIndex = 0;

	struct timeval lats, late;

	while (!_end) {
		_bufferIndex = (_bufferIndex + 1) % 2;
		int nextindex = (_bufferIndex + 1) % 2;

		QGLBuffer *bufptr = _glbuffer[_bufferIndex];

		//
		// bind the texture and buffer object
		//
		glBindTexture(GL_TEXTURE_2D, _textureid);
		if (!bufptr->bind()) qDebug() << "bind error";

		//
		// copy the pixel from the glbuffer to texture object
		// glTexImage2D has been called in init()
		//
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.size().width(), img.size().height(), GL_RGB, GL_UNSIGNED_BYTE, 0);

		// reset
		glBindTexture(GL_TEXTURE_2D, 0);
		bufptr->release();

//		emit updated(_textureid);

		//
		// receive pixel
		//
		img.fill(qRgb(pixelvalue, pixelvalue, 0));
		pixelvalue++;
		pixelvalue = pixelvalue % 128;

		::usleep(20 * 1000);

		gettimeofday(&lats, 0);





		bufptr = _glbuffer[nextindex];
		if (!bufptr->bind()) qDebug() << "bind error";

		bufptr->allocate(0, img.byteCount());

		unsigned char *mapped = (unsigned char *)bufptr->map(QGLBuffer::WriteOnly);
		if (mapped) {
			::memcpy(mapped, img.bits(), img.byteCount());
			bufptr->unmap();
		}

//		bufptr->write(0, constImgRef.bits(), constImgRef.byteCount());

		bufptr->release();

		gettimeofday(&late, 0);

		qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
		qDebug() << networkrecvdelay * 1000 << "msec";

	}
	qDebug() << "thread exit while loop";


	if (glIsTexture(_textureid)) {
		glDeleteTextures(1, &_textureid);
	}

	if (_glbuffer) {
		for (int i=0; i<2; i++) {
			_glbuffer[i]->release();
			_glbuffer[i]->destroy();
			delete _glbuffer[i];
		}
		free(_glbuffer);
	}

	_myGlWidget->doneCurrent();
}







