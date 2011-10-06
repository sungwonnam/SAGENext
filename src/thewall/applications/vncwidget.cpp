#include "vncwidget.h"
#include <signal.h>

#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include "../common/commonitem.h"

#include <QtGui>

rfbBool SN_VNCClientWidget::got_data = FALSE;
QString SN_VNCClientWidget::username = "";
QString SN_VNCClientWidget::vncpasswd = "evl123";

SN_VNCClientWidget::SN_VNCClientWidget(quint64 globalappid, const QString senderIP, int display, const QString username, const QString passwd, int frate, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: SN_RailawareWidget(globalappid, s, parent, wflags)
	, vncclient(0)
	, serverPort(5900)
	, _image(0)
	, _end(false)
	, framerate(frate)

{
	if ( username == "user" )
		SN_VNCClientWidget::username = "";
	else
		SN_VNCClientWidget::username = username;
	SN_VNCClientWidget::vncpasswd = passwd;

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

	//_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB32);
	_image = new QImage(vncclient->width, vncclient->height, QImage::Format_RGB888);
	//qDebug("vnc widget image %d x %d and bytecount %d", vncclient->width, vncclient->height, _image->byteCount());


	qreal fmargin = _settings->value("gui/framemargin", 0).toInt();

	/**
	  Don't forget to call resize() once you know the size of image you're displaying.
	  Also BaseWidget::resizeEvent() will call setTransformOriginPoint();
     */
	resize(_image->width() + fmargin*2, _image->height() + fmargin*2);
	_appInfo->setFrameSize(_image->width() + fmargin*2, _image->height() + fmargin*2, 24);

	qDebug() << "VNCClientWidget constructor" << boundingRect() << size();


	setWidgetType(SN_BaseWidget::Widget_RealTime);
	if (_perfMon) {
		_perfMon->setExpectedFps( (qreal)framerate );
		_perfMon->setAdjustedFps( (qreal)framerate );
	}


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


	if (!pixmap.isNull())
		painter->drawPixmap(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), pixmap); // Drawing QPixmap is much faster than QImage


	// if (_image && !_image->isNull()) {
	//  painter->drawImage(0, 0, *_image);
	//}


	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_VNCClientWidget::scheduleUpdate() {
#if QT_VERSION < 0x040700
    pixmap = QPixmap::fromImage(*_image);
    if (pixmap.isNull()) {
        qDebug("SageStreamWidget::scheduleUpdate() : QPixmap::fromImage() error");
#else
    if (! pixmap.convertFromImage(*_image, Qt::AutoColor | Qt::OrderedDither) ) {
        qDebug("VNCClientWidget::%s() : pixmap->convertFromImage() error", __FUNCTION__);
#endif
    }
	else {
		// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
		// QGraphicsView will process the event
		update();
	}
}

void SN_VNCClientWidget::receivingThread() {

	while (!_end) {
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

            int i = WaitForMessage(vncclient, 100000); // 100 microsecond
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
            if ( (time-now) >= (1000/framerate) ) break;
            
		}
		if (_end) break;

		// now copy pixels
		unsigned char * vncpixels = (unsigned char *)vncclient->frameBuffer;
		unsigned char * buffer = _image->bits();

		//		_image->loadFromData(vncpixels, vncclient->width * vncclient->height);

		Q_ASSERT(vncpixels && buffer);


		for (int k =0 ; k<vncclient->width * vncclient->height; k++) {
			// QImage::Format_RGB32 format : 0xffRRGGBB
			/*
                    buffer[4*k + 3] = 0xff;
                        buffer[4*k + 2] = vncpixels[ 4*k + 0];
                        buffer[4*k + 1] = vncpixels[ 4*k + 1];
                        buffer[4*k + 0] = vncpixels[ 4*k + 2];
                        */

			// QImage::Format_RGB888
			buffer[3*k + 0] = vncpixels[ 4*k + 0];
			buffer[3*k + 1] = vncpixels[ 4*k + 1];
			buffer[3*k + 2] = vncpixels[ 4*k + 2];
		}

		QMetaObject::invokeMethod(this, "scheduleUpdate", Qt::QueuedConnection);
		// why I can't invoke update() ???
	}

	qDebug() << "LibVNCClient receiving thread finished";
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

