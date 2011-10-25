#include "vncwidget.h"

#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include "../common/commonitem.h"

#include <QtGui>

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>



rfbBool SN_VNCClientWidget::got_data = FALSE;
QString SN_VNCClientWidget::username = "";
QString SN_VNCClientWidget::vncpasswd = "evl123";

SN_VNCClientWidget::SN_VNCClientWidget(quint64 globalappid, const QString senderIP, int display, const QString username, const QString passwd, int frate, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: SN_RailawareWidget(globalappid, s, parent, wflags)
	, vncclient(0)
	, serverPort(5900)
	, _image(0)
    , _textureid(0)
	, _end(false)
	, _framerate(frate)

{
	_appInfo->setMediaType(SAGENext::MEDIA_TYPE_VNC);
	
	if ( username == "user" )
		SN_VNCClientWidget::username = "";
	else
		SN_VNCClientWidget::username = username;
	SN_VNCClientWidget::vncpasswd = passwd;
	
	_appInfo->setVncUsername(SN_VNCClientWidget::username);
	_appInfo->setVncPassword(SN_VNCClientWidget::vncpasswd);

//	qDebug() << "vnc widget constructor " <<  username << passwd << VNCClientWidget::username << VNCClientWidget::vncpasswd;

	// 8 bit/sample
	// 3 samples/pixel
	// 4 Byte/pixel
	vncclient = rfbGetClient(8, 3, 4);

	vncclient->canHandleNewFBSize = false;
	vncclient->appData.useRemoteCursor = true;
	vncclient->MallocFrameBuffer = SN_VNCClientWidget::resize_func;
	vncclient->GotFrameBufferUpdate = SN_VNCClientWidget::update_func;
	vncclient->HandleCursorPos = SN_VNCClientWidget::position_func;
	vncclient->GetPassword = SN_VNCClientWidget::password_func;

	serverAddr.setAddress(senderIP);
	_appInfo->setSrcAddr(senderIP);
	
	

	if (!SN_VNCClientWidget::username.isEmpty()  &&  !SN_VNCClientWidget::vncpasswd.isEmpty()) {
		vncclient->GetCredential = SN_VNCClientWidget::getCredential;
		const uint32_t authSchemes[] = {rfbARD, rfbVncAuth, rfbNoAuth};
		int numSchemes = 3;
		SetClientAuthSchemes(vncclient, authSchemes, numSchemes);
	}
	else {
		const uint32_t authSchemes[] = {rfbVncAuth, rfbNoAuth};
		int numSchemes = 2;
		SetClientAuthSchemes(vncclient, authSchemes, numSchemes);
	}

	vncclient->FinishedFrameBufferUpdate = SN_VNCClientWidget::frame_func;

	int margc = 2;
	char *margv[2];
	margv[0] = strdup("vnc");
	margv[1] = (char *)malloc(256);
	memset(margv[1], 0, 256);
	sprintf(margv[1], "%s:%d", qPrintable(serverAddr.toString()), display);

	if ( ! rfbInitClient(vncclient, &margc, margv) ) {
		qCritical() << "rfbInitClient() error init";
		deleteLater();
	}

	if (vncclient->serverPort == -1 )
		vncclient->vncRec->doNotSleep = true;

	_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB32);
//	_image = new QImage(vncclient->width, vncclient->height, QImage::Format_ARGB32_Premultiplied);
//	_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB888);
	//qDebug("vnc widget image %d x %d and bytecount %d", vncclient->width, vncclient->height, _image->byteCount());


//	qreal fmargin = _settings->value("gui/framemargin", 0).toInt();

	/**
	  Don't forget to call resize() once you know the size of image you're displaying.
	  Also BaseWidget::resizeEvent() will call setTransformOriginPoint();
     */
//	resize(_image->width() + fmargin*2, _image->height() + fmargin*2);
//	_appInfo->setFrameSize(_image->width() + fmargin*2, _image->height() + fmargin*2, 24);

	resize(_image->size());
	_appInfo->setFrameSize(_image->width(), _image->height(), _image->depth());

//	qDebug() << "VNCClientWidget constructor" << boundingRect() << size();


	setWidgetType(SN_BaseWidget::Widget_RealTime);
	if (_perfMon) {
		_perfMon->setExpectedFps( (qreal)_framerate );
		_perfMon->setAdjustedFps( (qreal)_framerate );
	}

	setAttribute(Qt::WA_PaintOnScreen);

	/**
	  sets the transform origin point to widget's center
	 */
//	setTransformOriginPoint( _image->width() / 2.0 , _image->height() / 2.0 );

	// starting thread.
	future = QtConcurrent::run(this, &SN_VNCClientWidget::receivingThread);
}

SN_VNCClientWidget::~SN_VNCClientWidget() {
	_end = true;
	future.cancel();
	future.waitForFinished();
	if (_image) delete _image;

	if (glIsTexture(_textureid)) {
		glDeleteTextures(1, &_textureid);
	}
}

rfbCredential * SN_VNCClientWidget::getCredential(struct _rfbClient *client, int credentialType) {
	Q_UNUSED(client);
	Q_UNUSED(credentialType);
	rfbCredential *res = (rfbCredential *)malloc(sizeof(rfbCredential));
	if ( ! SN_VNCClientWidget::username.isEmpty())
		res->userCredential.username = strdup(qPrintable(SN_VNCClientWidget::username));
	else
		res->userCredential.username = NULL;
	if (! SN_VNCClientWidget::vncpasswd.isEmpty())
		res->userCredential.password = strdup(qPrintable(SN_VNCClientWidget::vncpasswd));
	else
		res->userCredential.password = NULL;
	return res;
}

void SN_VNCClientWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
	if (_perfMon) {
		_perfMon->getDrawTimer().start();
	}

	SN_BaseWidget::paint(painter, o, w);

	//	painter->setRenderHint(QPainter::Antialiasing);
	//	painter->setRenderHint(QPainter::HighQualityAntialiasing);
	painter->setRenderHint(QPainter::SmoothPixmapTransform); // important -> this will make text smoother

/*
	if (!_pixmap.isNull())
		painter->drawPixmap(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), _pixmap); // Drawing QPixmap is much faster than QImage
		*/


//	if (!_imageForDrawing.isNull()) {
//		// I'm drawing the QImage to avoid conversion delay (just like SageStreamWidget)
//		painter->drawImage(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), _imageForDrawing);
//	}




	if (painter->paintEngine()->type() == QPaintEngine::OpenGL2 /* || painter->paintEngine()->type() == QPaintEngine::OpenGL */) {
		if (glIsTexture(_textureid)) {
			QGLWidget *viewportWidget = (QGLWidget *)w;
			viewportWidget->drawTexture(QPointF(0, 0), _textureid);
		}
	}
	else {
		if (_image && !_image->isNull()) {
			// I'm drawing the QImage to avoid conversion delay (just like SageStreamWidget)
			painter->drawImage(0, 0, *_image);
		}
	}


	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_VNCClientWidget::scheduleUpdate() {
	/*
#if QT_VERSION < 0x040700
    pixmap = QPixmap::fromImage(*_image);
    if (pixmap.isNull()) {
        qDebug("SageStreamWidget::scheduleUpdate() : QPixmap::fromImage() error");
#else
    if (! _pixmap.convertFromImage(*_image, Qt::AutoColor | Qt::OrderedDither) ) {
        qDebug("VNCClientWidget::%s() : pixmap->convertFromImage() error", __FUNCTION__);
#endif
*/
	if (!_image || _image->isNull()) {
    }
	else {
		_perfMon->getConvTimer().start();

		// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
		// QGraphicsView will process the event

//		_imageForDrawing = *_image;





		QGLContext *glContext = const_cast<QGLContext *>(QGLContext::currentContext());
		if(glContext) {

			// to avoid detach(), and QGLContext::InvertedYBindOption
			// In OpenGL 0,0 is bottom left, In Qt 0,0 is top left
			const QImage &constRef = _image->mirrored(false, true);
			//_textureid = glContext->bindTexture(constRef, GL_TEXTURE_2D, QGLContext::InvertedYBindOption);

			if (glIsTexture(_textureid)) {
				glDeleteTextures(1, &_textureid);
			}
			glGenTextures(1, &_textureid);
			glBindTexture(GL_TEXTURE_2D, _textureid);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//			glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, constRef.width(), constRef.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, constRef.bits());
			glBindTexture(GL_TEXTURE_2D, 0);

			GLenum error = glGetError();
			if(error != GL_NO_ERROR) {
				qWarning("texture upload failed. error code 0x%x\n", error);
			}
		}





		_perfMon->updateConvDelay();

		update();
	}
}

void SN_VNCClientWidget::receivingThread() {
/*
	struct timeval lats, late;
	struct rusage ru_start, ru_end;

	if(_perfMon) {
		_perfMon->getRecvTimer().start(); //QTime::start()

#if defined(Q_OS_LINUX)
		getrusage(RUSAGE_THREAD, &ru_start); // that of calling thread. Linux specific
#elif defined(Q_OS_MAC)
		getrusage(RUSAGE_SELF, &ru_start);
#endif
	}
*/

	while (!_end) {

//		if (_perfMon) gettimeofday(&lats, 0);


		// sleep to ensure desired fps
		qint64 now = 0;
        
#if QT_VERSION < 0x040700
        struct timeval tv;
        gettimeofday(&tv, 0);
        now = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#else
        now = QDateTime::currentMSecsSinceEpoch();
#endif
        qint64 time = 0;
		forever {
			if (!vncclient) {
				_end = true;
				break;
			}

            int i = WaitForMessage(vncclient, 100000); // 100 microsecond == 0.1 msec
			if ( i<0 ) {
				rfbClientLog("VNC error. quit\n");
				_end = true;
				break;
			}

			if (i) {
				if(!HandleRFBServerMessage(vncclient)) {
					rfbClientLog("HandleRFBServerMessage quit\n");
					_end = true;
					break;
				}
			}
            
#if QT_VERSION < 0x040700
            struct timeval tv;
            gettimeofday(&tv, 0);
            time = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#else
            time = QDateTime::currentMSecsSinceEpoch();
#endif
            if ( (time-now) >= (1000/_framerate) ) break;
            
		}
		if (_end) break;


		//
		// now copy pixels from LibVNCClient
		//
		unsigned char * vncpixels = (unsigned char *)vncclient->frameBuffer;
//		unsigned char * rgbbuffer = _image->bits();
		QRgb *rgbbuffer = (QRgb *)(_image->scanLine(0)); // An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.

		Q_ASSERT(vncpixels && rgbbuffer);

		/*
		for (int k =0 ; k<vncclient->width * vncclient->height; k++) {
			// QImage::Format_RGB32 format : 0xffRRGGBB
			rgbbuffer[4*k + 3] = 0xff;
			rgbbuffer[4*k + 2] = vncpixels[ 4*k + 0]; // red
			rgbbuffer[4*k + 1] = vncpixels[ 4*k + 1]; // green
			rgbbuffer[4*k + 0] = vncpixels[ 4*k + 2]; // blue

			// QImage::Format_RGB888
//			buffer[3*k + 0] = vncpixels[ 4*k + 0];
//			buffer[3*k + 1] = vncpixels[ 4*k + 1];
//			buffer[3*k + 2] = vncpixels[ 4*k + 2];
		}
		*/

		for (int i=0; i<vncclient->width * vncclient->height; i++) {
			rgbbuffer[i] = qRgb(vncpixels[4*i+2], vncpixels[4*i+1], vncpixels[4*i+0]);
		}

		QMetaObject::invokeMethod(this, "scheduleUpdate", Qt::QueuedConnection);
//		QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

/***
		if (_perfMon) {
			gettimeofday(&late, 0);
#if defined(Q_OS_LINUX)
			getrusage(RUSAGE_THREAD, &ru_end);
#elif defined(Q_OS_MAC)
			getrusage(RUSAGE_SELF, &ru_end);
#endif
			qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001); // second

			// calculate
//			perf->updateRecvLatency(read, ru_start, ru_end); // QTimer::restart()
			_perfMon->updateObservedRecvLatency(vncclient->width * vncclient->height, networkrecvdelay, ru_start, ru_end);

			ru_start = ru_end;
			lats = late;

//			qDebug() << perf->getCpuUsage();
		}
		****/


	} // end of while (_end)

//	qDebug() << "LibVNCClient receiving thread finished";
//	QMetaObject::invokeMethod(this, "fadeOutClose", Qt::QueuedConnection);
	QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}



void SN_VNCClientWidget::signal_handler(int signal)
{
	rfbClientLog("Cleaning up.\n");
}

rfbBool SN_VNCClientWidget::resize_func(rfbClient* client)
{
        static rfbBool first = TRUE;
        if(!first) {
                rfbClientLog("I don't know yet how to change resolutions!\n");
                exit(1);
        }
        signal(SIGINT,signal_handler);

        int width=client->width;
        int height=client->height;
        int depth=client->format.bitsPerPixel;

//	depth = depth / 8; // Byte

        client->updateRect.x = client->updateRect.y = 0;
        client->updateRect.w = width; client->updateRect.h = height;

        client->frameBuffer = (uint8_t*)malloc(width*height*depth );
        memset(client->frameBuffer, 0, width*height*depth);

        rfbClientLog("Allocate %d bytes: %d x %d x %d\n", width*height*depth, width,height,depth);
        return TRUE;
}

void SN_VNCClientWidget::frame_func(rfbClient *)
{
//        rfbClientLog("Received a frame\n");
}

rfbBool SN_VNCClientWidget::position_func(rfbClient *, int x, int y)
{
        Q_UNUSED(x);
        Q_UNUSED(y);

        //rfbClientLog("Received a position for %d,%d\n",x,y);
        return TRUE;
}

char * SN_VNCClientWidget::password_func(rfbClient *)
{
        char *str = (char*)malloc(64);
        memset(str, 0, 64);
        strncpy(str, qPrintable(SN_VNCClientWidget::vncpasswd), 64);
        return str;
}

void SN_VNCClientWidget::update_func(rfbClient* client,int x,int y,int w,int h)
{
        rfbPixelFormat* pf=&client->format;
        int bpp=pf->bitsPerPixel/8;
        int row_stride = client->width*bpp;

        SN_VNCClientWidget::got_data = TRUE;

        //rfbClientLog("Received an update for %d,%d,%d,%d.\n",x,y,w,h);
}

