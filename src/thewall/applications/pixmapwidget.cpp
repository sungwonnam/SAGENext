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


PixmapWidget::PixmapWidget(QString filename, const quint64 id, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags) :

                BaseWidget(id, s, parent, wFlags),
                dataSrc(PixmapWidget::FROM_DISK_FILE),
                image(new QImage()), /* Because it is unsafe to use pixmap outside of GUI thread */
                pixmap(new QPixmap()),
                serverSock(0),
                filename(QString(filename)),
                filesize(0),
                recvAddr(QHostAddress(QHostAddress::Null)),
                recvPort(0),
                isImageReady(false)

{
        /* indicates that the widget contents are north-west aligned and static. On resize, such a widget will receive paint events only for parts of itself that are newly visible. This flag is set or cleared by the widget's author. */
//	setAttribute(Qt::WA_StaticContents, true); // unsupported under X11 ?

        setWidgetType(BaseWidget::Widget_Image);
        _appInfo->setMediaType(MEDIA_TYPE_IMAGE);
        _appInfo->setFileInfo( filename );
        _appInfo->setSrcAddr("127.0.0.1"); // from my disk
        start();
}

PixmapWidget::PixmapWidget(qint64 filesize, const QString &senderIP, const QString &recvIP, quint16 recvPort, const quint64 id, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags) :

                BaseWidget(id, s, parent, wFlags),
                dataSrc(PixmapWidget::FROM_SOCKET),
                image(new QImage()), /* receive into QImage, convert to QPixmap */
                pixmap(new QPixmap()),
                serverSock(0),
                filename(QString()), /* null string */
                filesize(filesize),
                recvAddr(QHostAddress(recvIP)),
                recvPort(recvPort),
                isImageReady(false)

{
	setWidgetType(BaseWidget::Widget_Image);
	_appInfo->setMediaType(MEDIA_TYPE_IMAGE);
	_appInfo->setSrcAddr(senderIP);
	start();
}

PixmapWidget::~PixmapWidget() {
	if (image) delete image;
	if (pixmap) delete pixmap;
	qDebug("PixmapWidget::%s()", __FUNCTION__);
}

void PixmapWidget::start() {
        /** // QRunnable
        // If true, runnable will be deleted after thread finishes.
        setAutoDelete(false);

        // Note that the thread pool takes ownership of the runnable if runnable->autoDelete() returns true, and the runnable will be deleted automatically by the thread pool after the runnable->run() returns. If runnable->autoDelete() returns false, ownership of runnable remains with the caller. Note that changing the auto-deletion on runnable after calling this functions results in undefined behavior.
        QThreadPool::globalInstance()->start(static_cast<QRunnable *>(this));
        **/

	futureWatcher = new QFutureWatcher<bool>(this);
	connect(futureWatcher, SIGNAL(finished()), this, SLOT(callUpdate()));

	/* start receiving thread */
	QFuture<bool> future = QtConcurrent::run(this, &PixmapWidget::readImage);

	futureWatcher->setFuture(future);
}

void PixmapWidget::callUpdate() {
	if ( futureWatcher->result() ) {

		qreal left = settings->value("gui/framemarginleft", 0).toInt();
		qreal right = settings->value("gui/framemarginright", 0).toInt();
		qreal top = settings->value("gui/framemargintop", 0).toInt();
		qreal bottom = settings->value("gui/framemarginbottom", 0).toInt();

		resize(image->width() + left + right , image->height() + top + bottom);
//		qDebug() << "boundingRect" << boundingRect() << "windowFrameRect" << windowFrameRect();
		_appInfo->setFrameSize(image->width() + left + right, image->height() + top + bottom, image->depth());

		/**
		  set transform origin point to widget's center
		  this is handled in BaseWidget::resizeEvent()
		  */
//		setTransformOriginPoint( image->width() / 2.0 , image->height() / 2.0 );

		delete image;
		image = 0;

		isImageReady = true;
		update();
	}
	else {
		qCritical() << "PixmapWidget read thread finished with error";
	}

        /*
        qreal left, top, right, bottom;
        getWindowFrameMargins(&left, &top, &right, &bottom);
        qDebug() << "windowFrameMargin" << left << top << right << bottom;
        */
}

void PixmapWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
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

//	painter->setRenderHint(QPainter::SmoothPixmapTransform);

	//why not using painter->scale() instead?
	//painter->drawImage(0, 0, _image->scaled(contentSize.toSize(), Qt::KeepAspectRatio)); // shallow copy (Implicitly Shared mode)
	//if ( scaleFactorX != 1.0 || scaleFactorY != 1.0 )
	//painter->scale(scaleFactorX, scaleFactorX);
	//painter->drawImage(QPointF(0,0), *image);
	painter->drawPixmap(settings->value("gui/framemarginleft", 0).toInt(), settings->value("gui/framemargintop", 0).toInt(), *pixmap);

	BaseWidget::paint(painter, o, w);

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}


bool PixmapWidget::readImage() {
        Q_ASSERT(image);

        switch(dataSrc) {
        case FROM_SOCKET: {
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
//			myAddr.sin_addr.s_addr = recvAddr.toIPv4Address();
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

                        if (image->loadFromData(buffer)) {
#if QT_VERSION >= 0x040700
                            pixmap->convertFromImage(*image);
#else
                            QPixmap pm = QPixmap::fromImage(*image);
                            *pixmap  = pm;
#endif
//				isImageReady = true;

                                /* save image file to local disk */

                                /* update filename in _appInfo */
                        }
                        else
                            qWarning("%s::%s() : QImage::loadFromData() failed", metaObject()->className(), __FUNCTION__);


                        /*! close data channel network connection */
                        ::shutdown(socket, SHUT_RDWR);
                        if ( ::close(socket) != 0 ) {
                                perror("PixmapWidget::run() : close socket");
                        }
                        if ( ::close(serverSock) != 0 ) {
                                perror("PixmapWidget::run() : close serverSock");
                        }

                        break;
                }

        case FROM_DISK_FILE: {
                        if (image->load(filename)) {
#if QT_VERSION >= 0x040700
                            pixmap->convertFromImage(*image);
#else
                            pixmap->load(filename);
#endif
//				isImageReady = true;
                        }
                        else qWarning("%s::%s() : QImage::load(%s) failed !",metaObject()->className(), __FUNCTION__, qPrintable(filename));
                        break;
                }
        case FROM_DISK_DIRECTORY: {
                        break;
                }
        }

        if ( ! pixmap->isNull() ) {
                return true;
        }
        else {
                qCritical("%s::%s() : pixmap is null", metaObject()->className(), __FUNCTION__);
        }

        return false;
}
