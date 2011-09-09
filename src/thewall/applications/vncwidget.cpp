#include "vncwidget.h"
#include <signal.h>

#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include "../common/commonitem.h"

#include <QtGui>

rfbBool VNCClientWidget::got_data = FALSE;
QString VNCClientWidget::vncpasswd = "evl123";

VNCClientWidget::VNCClientWidget(quint64 globalappid, const QString senderIP, int display, const QString passwd, int frate, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: RailawareWidget(globalappid, s, parent, wflags)
	, vncclient(0)
	, serverPort(5900)
	, _image(0)
	, _end(false)
	, framerate(frate)

{
	VNCClientWidget::vncpasswd = passwd;

	// 8 bit/sample
	// 3 samples/pixel
	// 4 Byte/pixel
	vncclient = rfbGetClient(8, 3, 4);

	vncclient->canHandleNewFBSize = false;
	vncclient->appData.useRemoteCursor = true;
	vncclient->MallocFrameBuffer = VNCClientWidget::resize_func;
	vncclient->GotFrameBufferUpdate = VNCClientWidget::update_func;
	vncclient->HandleCursorPos = VNCClientWidget::position_func;
	vncclient->GetPassword = VNCClientWidget::password_func;

	serverAddr.setAddress(senderIP);

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

	/**
	  This is important
     */
	resize(_image->size());

	/**
	  sets the transform origin point to widget's center
	 */
	setTransformOriginPoint( _image->width() / 2.0 , _image->height() / 2.0 );

	// starting thread.
	future = QtConcurrent::run(this, &VNCClientWidget::receivingThread);
}

VNCClientWidget::~VNCClientWidget() {
	_end = true;
	future.cancel();
	future.waitForFinished();
	if (_image) delete _image;
}

void VNCClientWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
        if (_perfMon) {
                _perfMon->getDrawTimer().start();
        }

//	painter->setRenderHint(QPainter::Antialiasing);
//	painter->setRenderHint(QPainter::HighQualityAntialiasing);
        painter->setRenderHint(QPainter::SmoothPixmapTransform); // important -> this will make text smoother


        if (isSelected()) {
                // setBrush hurts performance badly !!
//		painter->setBrush( QBrush(Qt::lightGray, Qt::Dense2Pattern) ); // very bad

//		painter->drawRect( windowFrameRect() ); // will add 0.5~1 msec when 4K
//		painter->fillRect(windowFrameRect(), QBrush(Qt::lightGray, Qt::Dense6Pattern)); // bad
//		painter->fillRect(windowFrameRect(), Qt::lightGray); // will adds 2~3 msec when 4K res
//		painter->fillRect(windowFrameRect(), Qt::Dense6Pattern); // bad

//		shadow->setEnabled(true);
        }
        else {
//		shadow->setEnabled(false);
        }



        if (!pixmap.isNull())
                painter->drawPixmap(0, 0, pixmap); // Drawing QPixmap is much faster than QImage



//        if (_image && !_image->isNull()) {
//                painter->drawImage(0, 0, *_image);
//        }





        if ( showInfo  &&  !infoTextItem->isVisible() ) {
#if defined(Q_OS_LINUX)
                _appInfo->setDrawingThreadCpu(sched_getcpu());
#endif
                infoTextItem->show();
        }
        else if (!showInfo && infoTextItem->isVisible()){
                infoTextItem->hide();
        }
        if (_perfMon)
			_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void VNCClientWidget::scheduleUpdate() {
	if (! pixmap.convertFromImage(*_image, Qt::AutoColor | Qt::OrderedDither) ) {
		qDebug("VNCClientWidget::%s() : pixmap->convertFromImage() error", __FUNCTION__);
	}
	else {
		// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
		// QGraphicsView will process the event
		update();
	}
}

void VNCClientWidget::receivingThread() {

	while (!_end) {
		// sleep to ensure desired fps
		qint64 now = QDateTime::currentMSecsSinceEpoch();
		while ( (QDateTime::currentMSecsSinceEpoch() - now) < (1000 / framerate)) {

			if (!vncclient) {
				_end = true;
				break;
			}

			int i = WaitForMessage(vncclient, 100000);
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
	QMetaObject::invokeMethod(this, "fadeOutClose", Qt::QueuedConnection);
}



void VNCClientWidget::signal_handler(int signal)
{
	rfbClientLog("Cleaning up.\n");
}

rfbBool VNCClientWidget::resize_func(rfbClient* client)
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

void VNCClientWidget::frame_func(rfbClient *)
{
        rfbClientLog("Received a frame\n");
}

rfbBool VNCClientWidget::position_func(rfbClient *, int x, int y)
{
        Q_UNUSED(x);
        Q_UNUSED(y);

        //rfbClientLog("Received a position for %d,%d\n",x,y);
        return TRUE;
}

char * VNCClientWidget::password_func(rfbClient *)
{
        char *str = (char*)malloc(64);
        memset(str, 0, 64);
        strncpy(str, qPrintable(VNCClientWidget::vncpasswd), 64);
        return str;
}

void VNCClientWidget::update_func(rfbClient* client,int x,int y,int w,int h)
{
        rfbPixelFormat* pf=&client->format;
        int bpp=pf->bitsPerPixel/8;
        int row_stride = client->width*bpp;

        VNCClientWidget::got_data = TRUE;

        //rfbClientLog("Received an update for %d,%d,%d,%d.\n",x,y,w,h);
}

