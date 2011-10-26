#include "pixmapwidget.h"
#include "base/appinfo.h"
#include "base/perfmonitor.h"
#include "../common/commonitem.h"

//#include <sys/types.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <string.h>
//#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


SN_PixmapWidget::SN_PixmapWidget(QString filename, const quint64 id, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)

    : SN_BaseWidget(id, s, parent, wFlags)

    , dataSrc(SN_PixmapWidget::FROM_DISK_FILE)
    , _imageTemp(new QImage()) /* Because it is unsafe to use pixmap outside of GUI thread */
//    , _pixmap(new QPixmap())
    , serverSock(0)
    , filename(QString(filename))
    , filesize(0)
    , recvAddr(QHostAddress(QHostAddress::Null))
    , recvPort(0)
    , isImageReady(false)
{
	/* indicates that the widget contents are north-west aligned and static. On resize, such a widget will receive paint events only for parts of itself that are newly visible. This flag is set or cleared by the widget's author. */
//	setAttribute(Qt::WA_StaticContents, true); // unsupported under X11 ?

	setWidgetType(SN_BaseWidget::Widget_Image);
	_appInfo->setMediaType(SAGENext::MEDIA_TYPE_IMAGE);
	_appInfo->setFileInfo( filename );
	_appInfo->setSrcAddr("127.0.0.1"); // from my disk
	start();
}

SN_PixmapWidget::SN_PixmapWidget(qint64 filesize, const QString &senderIP, const QString &recvIP, quint16 recvPort, const quint64 id, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)

    : SN_BaseWidget(id, s, parent, wFlags)

    , dataSrc(SN_PixmapWidget::FROM_SOCKET)
    , _imageTemp(new QImage()) /* receive into QImage, convert to QPixmap */
//    , _pixmap(new QPixmap())
    , serverSock(0)
    , filename(QString()) /* null string */
    , filesize(filesize)
    , recvAddr(QHostAddress(recvIP))
    , recvPort(recvPort)
    , isImageReady(false)
{
	setWidgetType(SN_BaseWidget::Widget_Image);
	_appInfo->setMediaType(SAGENext::MEDIA_TYPE_IMAGE);
	_appInfo->setSrcAddr(senderIP);
	start();
}

SN_PixmapWidget::~SN_PixmapWidget() {
	if (_imageTemp) delete _imageTemp;
//	if (_pixmap) delete _pixmap;

	if (glIsTexture(_gltexture)) {
		glDeleteTextures(1, &_gltexture);
	}
	qDebug("%s::%s()",metaObject()->className(), __FUNCTION__);
}

void SN_PixmapWidget::start() {
        /** // QRunnable
        // If true, runnable will be deleted after thread finishes.
        setAutoDelete(false);

        // Note that the thread pool takes ownership of the runnable if runnable->autoDelete() returns true, and the runnable will be deleted automatically by the thread pool after the runnable->run() returns. If runnable->autoDelete() returns false, ownership of runnable remains with the caller. Note that changing the auto-deletion on runnable after calling this functions results in undefined behavior.
        QThreadPool::globalInstance()->start(static_cast<QRunnable *>(this));
        **/

	setCacheMode(QGraphicsItem::NoCache);

	//setAttribute(Qt::WA_PaintOnScreen);


	futureWatcher = new QFutureWatcher<bool>(this);
	connect(futureWatcher, SIGNAL(finished()), this, SLOT(callUpdate()));

	/* start receiving thread */
	QFuture<bool> future = QtConcurrent::run(this, &SN_PixmapWidget::readImage);

	futureWatcher->setFuture(future);
}

void SN_PixmapWidget::callUpdate() {
	if ( futureWatcher->result() ) {

		// at this point, image is not null

		_imgWidth = _imageTemp->width();
		_imgHeight = _imageTemp->height();

//		qreal fmargin = _settings->value("gui/framemargin", 0).toInt();

//		resize(_imgWidth + _bordersize * 2 , _imgHeight + _bordersize * 2);
//		_appInfo->setFrameSize(_imgWidth + _bordersize * 2, _imgHeight + _bordersize*2, _imageTemp->depth());

		resize(_imageTemp->size());
		_appInfo->setFrameSize(_imgWidth, _imgHeight, _imageTemp->depth());



		QGLContext *glContext = const_cast<QGLContext *>(QGLContext::currentContext());
		if(glContext) {

			const QImage &constRef = _imageTemp->mirrored().rgbSwapped(); // to avoid detach()
			//_gltexture = glContext->bindTexture(constRef, GL_TEXTURE_2D, QGLContext::InvertedYBindOption);

			if (glIsTexture(_gltexture)) {
				glDeleteTextures(1, &_gltexture);
			}
			glGenTextures(1, &_gltexture);
			glBindTexture(GL_TEXTURE_2D, _gltexture);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, constRef.width(), constRef.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, constRef.bits());
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else {
			_drawingPixmap = QPixmap::fromImage(*_imageTemp);
		}

		delete _imageTemp;
		_imageTemp = 0;
		isImageReady = true;
		update();
	}
	else {
		qCritical() << "SN_PixmapWidget read thread finished with error";
	}

        /*
        qreal left, top, right, bottom;
        getWindowFrameMargins(&left, &top, &right, &bottom);
        qDebug() << "windowFrameMargin" << left << top << right << bottom;
        */
}

void SN_PixmapWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	Q_UNUSED(o);
	Q_UNUSED(w);

	//	qDebug() << o->rect << o->state << o->type;

	if ( ! isImageReady ) {
//		QGraphicsSimpleTextItem text("Loading Image...", this);
		//		painter->setBrush(Qt::black);
		//		painter->fillRect(boundingRect(), Qt::lightGray);
		//		painter->setPen(Qt::white);
		//		painter->drawStaticText(0, 0, QStaticText("Loading Image"));
		return;
	}

	if (_perfMon)
		_perfMon->getDrawTimer().start();

	SN_BaseWidget::paint(painter, o, w);

//	painter->setRenderHint(QPainter::SmoothPixmapTransform);

	//why not using painter->scale() instead?
	//painter->drawImage(0, 0, _image->scaled(contentSize.toSize(), Qt::KeepAspectRatio)); // shallow copy (Implicitly Shared mode)
	//if ( scaleFactorX != 1.0 || scaleFactorY != 1.0 )
	//painter->scale(scaleFactorX, scaleFactorX);


	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {
		if (glIsTexture(_gltexture)) {

//			painter->beginNativePainting();

//			glEnable(GL_TEXTURE_2D);
//			glBindTexture(GL_TEXTURE_2D, _gltexture);

//			glBegin(GL_QUADS);
//			glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
//			glTexCoord2f(1.0, 1.0); glVertex2f(_image->width(), 0);
//			glTexCoord2f(1.0, 0.0); glVertex2f(_image->width(), _image->height());
//			glTexCoord2f(0.0, 0.0); glVertex2f(0, _image->height());
//			glEnd();

//			painter->endNativePainting();

			QGLWidget *viewportWidget = (QGLWidget *)w;
//			QRectF target = QRect(_bordersize, _bordersize, _imgWidth, _imgHeight);
//			QRectF target = QRect(0, 0, _imgWidth, _imgHeight);
			viewportWidget->drawTexture(QPointF(0,0), _gltexture);
		}
	}
	else {
		//	painter->drawImage(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), *_imageTemp);
		//	painter->drawImage(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), _myImage);
			painter->drawPixmap(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), _drawingPixmap);
		//	painter->drawPixmap(0, 0, _drawingPixmap);
	}

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}


bool SN_PixmapWidget::readImage() {
	Q_ASSERT(_imageTemp);

	switch(dataSrc) {
	case FROM_SOCKET:
	{
		qWarning("%s::%s() : receiving from network. %s:%hd", metaObject()->className(), __FUNCTION__, qPrintable(recvAddr.toString()), recvPort);

		serverSock = ::socket(AF_INET, SOCK_STREAM, 0);
		if (serverSock == -1) {
			qCritical("%s::%s() : can't creat socket", metaObject()->className(), __FUNCTION__);
			perror("socket");
			break;
		}

		sockaddr_in myAddr;
		memset(&myAddr, 0, sizeof(myAddr));
		myAddr.sin_family = AF_INET;
		myAddr.sin_port = htons(recvPort);
		//myAddr.sin_addr.s_addr = recvAddr.toIPv4Address();
		inet_pton(AF_INET, qPrintable(recvAddr.toString()), &myAddr.sin_addr.s_addr);

		if ( ::bind(serverSock, (struct sockaddr *)&myAddr, sizeof(myAddr)) != 0 ) {
			qCritical("%s::%s() : bind error", metaObject()->className(), __FUNCTION__);
			perror("bind");
			break;
		}
		if ( ::listen(serverSock, 10) != 0 ) {
			qCritical("%s::%s() : listen error", metaObject()->className(), __FUNCTION__);
			perror("listen");
			break;
		}

		QByteArray buffer(filesize, 0);
		int socket = ::accept(serverSock, 0, 0);
		if ( socket < 0 ) {
			qCritical("%s::%s() : accept error", metaObject()->className(), __FUNCTION__);
			perror("accept");
			break;
		}
		qDebug("%s::%s() : sender connected. socket %d",metaObject()->className(), __FUNCTION__,socket);

		if ( ::recv(socket, buffer.data(), filesize, MSG_WAITALL) <= 0 ) {
			qCritical("%s::%s() : error while receiving.", metaObject()->className(), __FUNCTION__);
			break;
		}

		if (_imageTemp->loadFromData(buffer)) {
			/*
#if QT_VERSION >= 0x040700
			_pixmap->convertFromImage(*_imageTemp);
#else
			QPixmap pm = QPixmap::fromImage(*image);
			*pixmap  = pm;
#endif
*/
			//isImageReady = true;

			/* save image file to local disk */

			/* update filename in _appInfo */
		}
		else
			qWarning("%s::%s() : QImage::loadFromData() failed", metaObject()->className(), __FUNCTION__);


		/*! close data channel network connection */
		::shutdown(socket, SHUT_RDWR);
		if ( ::close(socket) != 0 ) {
			perror("SN_PixmapWidget::run() : close socket");
		}
		if ( ::close(serverSock) != 0 ) {
			perror("SN_PixmapWidget::run() : close serverSock");
		}

		break;
	}

	case FROM_DISK_FILE:
	{
		/***
		  Below block make sagenext crash with error

		  <unknown>: Fatal IO error 11 (Resource temporarily unavailable) on X server :0.0.

		  Be careful when using native graphics backend (x11) because QPixmap is stored in the server side (X server) and QPixmap isn't thread-safe

		if (_imageTemp->load(filename)) {
#if QT_VERSION >= 0x040700
			_pixmap->convertFromImage(*_imageTemp);
#else
			_pixmap->load(filename);
#endif
			//isImageReady = true;
		}
		****/

		/***
		  Be careful when using native graphics backend (x11) because QPixmap is stored in the server side (X server) and QPixmap isn't thread-safe

		if ( ! _pixmap->load(filename)) {
			qCritical("%s::%s() : QPixmap::load(%s) failed !",metaObject()->className(), __FUNCTION__, qPrintable(filename));
		}
		***/

		if (!_imageTemp->load(filename)) {
			qCritical("%s::%s() : QImage::load(%s) failed !",metaObject()->className(), __FUNCTION__, qPrintable(filename));
		}

		break;
	}
	case FROM_DISK_DIRECTORY:
	{
		break;
	}
	} // end of switch




	if ( ! _imageTemp->isNull() ) {
		return true;
	}
	else {
		qCritical("%s::%s() : _pixmap is null", metaObject()->className(), __FUNCTION__);
	}

	return false;
}
