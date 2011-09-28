#include "sagestreamwidget.h"
#include "sagepixelreceiver.h"

#include "../sage/fsmanagermsgthread.h"
#include "../sage/sagecommondefinitions.h"

#include "../common/commonitem.h"
#include "../common/imagedoublebuffer.h"

#include "base/appinfo.h"
#include "base/affinityinfo.h"
#include "base/perfmonitor.h"

#include "../system/resourcemonitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <QProcess>

//#include <sys/time.h>
//#include <sys/resource.h>


//SageStreamWidget::SageStreamWidget(quint64 sageAppId, QString appName, int protocol, int port, const QRect initRect, const quint64 globalAppId, const QSettings *s, ResourceMonitor *rm, QGraphicsItem *parent/*0*/, Qt::WindowFlags wFlags/*0*/) :

//	RailawareWidget(globalAppId, s, rm, parent, wFlags), // QSettings *s is const

//	_fsmMsgThread(0),
//	_sageAppId(sageAppId),
//	receiverThread(0),
//	image(0),
//	doubleBuffer(new ImageDoubleBuffer),
//	serversocket(0),
//	streamsocket(0),
//	imageSize(0),
//	frameCounter(0)
//{
////	Q_UNUSED(appName);

//	if ( port > 65535 ) {
//		qCritical("SageStreamWidget::%s() : Invalid port number %d", __FUNCTION__, port);
//		deleteLater();
//	}

//	/* receive regMsg from sail and create buffer */

//	if ( initialize(protocol, port) != 0 ) {
//		qCritical("SageStreamWidget::%s() : failed to initialize", __FUNCTION__);
//		deleteLater();
//	}
//	else {
//		// AffinityInfo is created in RailawareWidget
//		/* when there's no resource monitor, affinityInfo isn't created */
//		if(_affInfo)
//			_affInfo->setWidgetID(sageAppId);

//		Q_ASSERT(_appInfo);
//		_appInfo->setFrameSize(image->width(), image->height(), image->depth()); // == _image->byteCount()

////		qDebug("SageStreamWidget::%s() : sageappid %llu, groupsize %d, frameSize(SAIL) %d, frameSize(QImage) %d, expectedFps %.2f", __FUNCTION__, sageAppId, _appInfo->getNetworkUserBufferLength(), imageSize, _appInfo->getFrameBytecount(), _perfMon->getExpetctedFps());

//		_appInfo->setExecutableName( appName );
//		if ( appName == "imageviewer" ) {
//			_appInfo->setMediaType(MEDIA_TYPE_IMAGE);
//		}
//		else {
//			_appInfo->setMediaType(MEDIA_TYPE_VIDEO);
//		}



//		/* starting receiving thread */

//		// image->bits() will do deep copy (detach)
//		receiverThread = new SagePixelReceiver(protocol, streamsocket, /*image*/ doubleBuffer, _appInfo, _perfMon, _affInfo, /*this, mutex, wc,*/ settings);
////		qDebug("SageStreamWidget::%s() : SagePixelReceiver thread has begun",  __FUNCTION__);

//		connect(receiverThread, SIGNAL(finished()), this, SLOT(fadeOutClose())); // WA_Delete_on_close is defined

//		// don't do below.
////		connect(receiverThread, SIGNAL(finished()), receiverThread, SLOT(deleteLater()));


////		if (!scheduler) {
//			// This is queued connection because receiverThread reside outside of the main thread
//			if ( ! connect(receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate())) ) {
//				qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleUpdate() slot", metaObject()->className(), __FUNCTION__);
//			}
//			else {
////				qDebug("%s::%s() : frameReceived() -> scheduleUpdate() are connected", metaObject()->className(), __FUNCTION__);
//			}
////		}
//		receiverThread->start();


//		/*
//		QFuture<void> future = QtConcurrent::run(this, &SageStreamWidget::pixelRecvThread);
//		futureWatcher.setFuture(future);
//		connect(&futureWatcher, SIGNAL(finished()), this, SLOT(close()));
//		connect(this, SIGNAL(pauseThread()), &futureWatcher, SLOT(pause()));
//		connect(this, SIGNAL(resumeThread()), &futureWatcher, SLOT(resume()));
//		*/
//		setPos(initRect.x(), initRect.y());
//	}
//}

//void SageStreamWidget::stopPixelReceiver() {
//	if ( ::shutdown(socket, SHUT_RDWR) != 0 )
//		qDebug("SageStreamWidget::%s() : error while shutdown recv socket", __FUNCTION__);

//	if (receiverThread && receiverThread->isRunning()) {
//		QMetaObject::invokeMethod(receiverThread, "endReceiver",  Qt::QueuedConnection);
//		qApp->sendPostedEvents();
////		receiverThread->wait();
//	}
//}


SageStreamWidget::SageStreamWidget(QString filename, const quint64 globalappid, const QSettings *s, QString senderIP, ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : RailawareWidget(globalappid, s, parent, wFlags)
    , _fsmMsgThread(0)
    , _sailAppProc(0)
    , _sageAppId(0)
    , receiverThread(0)
    , image(0)
    , doubleBuffer(0)
    //, _pixmap(0)
    , serversocket(0)
    , streamsocket(0)
    , imageSize(0)
    , frameCounter(0)
{
    // this is defined in BaseWidget
    setRMonitor(rm);

	_appInfo->setFileInfo(filename);
	_appInfo->setSrcAddr(senderIP);
}



SageStreamWidget::~SageStreamWidget()
{
//    qDebug() << _globalAppId << "begin destructor" << QTime::currentTime().toString("hh:mm:ss.zzz");

    if (_affInfo)  {
        if ( !  _affInfo->disconnect() ) {
            qDebug() << "affInfo disconnect() failed";
        }
    }

    if (_rMonitor) {
        //_affInfo->disconnect();
        _rMonitor->removeSchedulableWidget(this); // remove this from ResourceMonitor::widgetMultiMap
        _rMonitor->removeApp(this); // will emit appRemoved(int) which is connected to Scheduler::loadBalance()
        //qDebug() << "affInfo removed from resourceMonitor";
    }

    disconnect(this);


    /**
      1. close fsm message channel
    **/
    _fsmMsgThread->sendSailShutdownMsg();
    _fsmMsgThread->wait();


    if (receiverThread) {
        disconnect(receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()));

        /**
          2. pixel receiving thread must exit from run()
          **/
        receiverThread->endReceiver();
    }


    /**
      3. The fsm thread can be deleted.
      **/
    delete _fsmMsgThread;



    /**
      4. Before calling receiverThread->wait(), let's release locks first so that receiverThread can exit from run() safely
      **/
    if (doubleBuffer) {
        doubleBuffer->releaseBackBuffer();
        doubleBuffer->releaseLocks();
    }


    /**
      5. make sure receiverThread finished safely
      **/
    receiverThread->wait();

    /**
      6. Then schedule deletion
      **/
    receiverThread->deleteLater();



    /**
      7. Now the double buffer can be deleted
      **/
    if (doubleBuffer) delete doubleBuffer;
    doubleBuffer = 0;





//    qDebug() << _globalAppId << "end destructor" << QTime::currentTime().toString("hh:mm:ss.zzz");
    qDebug("SageStreamWidget::%s() ",  __FUNCTION__);
}

void SageStreamWidget::setFsmMsgThread(fsManagerMsgThread *thread) {
        _fsmMsgThread = thread;
        _fsmMsgThread->start();
}


void SageStreamWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {

	if (_perfMon) {
		_perfMon->getDrawTimer().start();
		//perfMon->startPaintEvent();
	}


	//	if ( currentScale != 1.0 ) painter->scale(currentScale, currentScale);

	//	Q_ASSERT(image && !image->isNull());
	//	painter->drawImage(0, 0, *image2); // Implicit Sharing

	// slower than painter->scale()
	//	painter->drawImage(QPointF(0, 0), _image->scaled(cs.toSize(), Qt::KeepAspectRatio));

	//	  This is bettern than
	//	  pixmap->convertFromImage(*image) followed by painter->drawPixmap(*pixmap)
	//	  in terms of the total latency for a frame.
	//	  HOWEVER, paint() can be called whenever move, resize
	//	  So, it is wise to have paint() function fast
	//	painter->drawPixmap(0, 0, QPixmap::fromImage(*_image));


	if( !_pixmap.isNull()  &&  isVisible()  ) {
		painter->drawPixmap(0, 0, _pixmap); // the best so far
	}

	// if (!image2.isNull()  &&  isVisible())
	// painter->drawImage(0, 0, image2);


	BaseWidget::paint(painter,o,w);


	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
/***
        if ( ! image2.isNull() ) {
                painter->beginNativePainting();

                glEnable(GL_TEXTURE_2D);

                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                //	glOrtho(0, 1600, 1200, 0, 0, 100);
                glMatrixMode(GL_MODELVIEW);

                glGenTextures(1, &texhandle);
                glBindTexture(GL_TEXTURE_2D, texhandle);
                glTexImage2D(GL_TEXTURE_2D, 0, 3, image2.width(), image2.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image2.bits());


                glClearColor(1, 1, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glBegin(GL_QUADS);
                // top left
                glTexCoord2f(0, 0);
                glVertex2f(0, 0);

                // top right (0,0)
                glTexCoord2f(0, image2.width());
                glVertex2f(0, image2.width());

                // bottom right
                glTexCoord2f(image2.height(), image2.width());
                glVertex2f(image2.height(), image2.width());

                // bottom left
                glTexCoord2f(image2.height(), 0);
                glVertex2f(image2.height(), 0);

                glEnd();
                glFinish();
                //	glDisable(GL_TEXTURE_2D);

                painter->endNativePainting();
        }
        ****/

}



//void SageStreamWidget::resizeEvent(QGraphicsSceneResizeEvent *event) {
//	glViewport(0, 0, event->newSize().width(), event->newSize().height());
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	glMatrixMode(GL_MODELVIEW);

//	BaseGraphicsWidget::resizeEvent(event);
//}




void SageStreamWidget::scheduleReceive() {
//	qDebug() << "widget wakeOne";
//	if(wc) wc->wakeOne();
        receiverThread->receivePixel();
}

/**
  * this slot connected to the signal PixelReceiver::frameReceived()
  */
void SageStreamWidget::scheduleUpdate() {
    //	struct timeval s,e;
    //	gettimeofday(&s, 0);
    QImage *imgPtr = 0;

    if ( /*!image*/  !doubleBuffer || !receiverThread || receiverThread->isFinished() || !isVisible()  )
        return;

    else {
        //qDebug() << _globalAppId << "scheduleUpdate" << QTime::currentTime().toString("hh:mm:ss.zzz");

        imgPtr = static_cast<QImage *>(doubleBuffer->getBackBuffer());

        if (imgPtr && !imgPtr->isNull() ) {

            _perfMon->getConvTimer().start();

//            qDebug() << _globalAppId << "scheduleUpdate" << QTime::currentTime().toString("hh:mm:ss.zzz");

            // converts to QPixmap if you're gonna paint same QImage more than twice.
//            qDebug() << _globalAppId << imgPtr->byteCount();
#if QT_VERSION >= 0x040700
            if (! _pixmap.convertFromImage(*imgPtr) ) {
                qDebug("SageStreamWidget::scheduleUpdate() : pixmap->convertFromImage() error");
#else
            _pixmap = QPixmap::fromImage(*imgPtr);
            if (_pixmap.isNull()) {
                qDebug("SageStreamWidget::scheduleUpdate() : QPixmap::fromImage() error");
#endif
            }

//            image2 = imgPtr->convertToFormat(QImage::Format_RGB32);
//            if (image2.isNull()) {
//                qDebug() << "image2 is null";
//            }


            //image2 = imgPtr->convertToFormat(QImage::Format_RGB32);
            //image2 = QGLWidget::convertToGLFormat(*imgPtr);
            //if ( image2.isNull() )  {
            //	qDebug("Sibal");
            //}


            else {

                setScheduled(false); // reset scheduling flag for SMART scheduler

                ++frameCounter;
                //qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget : " << frameCounter << " has converted";

                _perfMon->updateConvDelay();

                /*
                  Maybe I should schedule update() and releaseBackBuffer in the scheduler
                */
                doubleBuffer->releaseBackBuffer();
                imgPtr = 0;

                //_perfMon->getEqTimer().start();
                //QDateTime::currentMSecsSinceEpoch();
                //qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget is about to call update() for frame " << frameCounter;

                // Schedules a redraw. This is not an immediate paint. This actually is postEvent()
                // QGraphicsView will process the event
                update(); // post paint event to myself
                //qApp->sendPostedEvents(this, QEvent::MetaCall);
                //qApp->flush();
                //qApp->processEvents();

                //this->scene()->views().front()->update( mapRectToScene(boundingRect()).toRect() );
            }
        }
        else {
            qCritical("SageStreamWidget::%s() : globalAppId %llu, sageAppId %llu : imgPtr is null. Failed to retrieve back buffer from double buffer", __FUNCTION__, globalAppId(), _sageAppId);
        }
        //doubleBuffer->releaseBackBuffer();
        //imgPtr = 0;
    }

}


int SageStreamWidget::createImageBuffer(int resX, int resY, sagePixFmt pixfmt) {
    int bytePerPixel = getPixelSize(pixfmt);
    int memwidth = resX * bytePerPixel; //Byte (single row of frame)

    imageSize = memwidth * resY; // Byte (a frame)

    qDebug("SageStreamWidget::%s() : recved regMsg. size %d x %d, pixfmt %d, pixelSize %d, memwidth %d, imageSize %d", __FUNCTION__, resX, resY, pixfmt, bytePerPixel, memwidth, imageSize);

    if (!doubleBuffer) doubleBuffer = new ImageDoubleBuffer;

    /*
         Do not draw ARGB32 images into the raster engine.
         ARGB32_premultiplied and RGB32 are the best ! (they are pixel wise compatible)
         http://labs.qt.nokia.com/2009/12/18/qt-graphics-and-performance-the-raster-engine/
      */
    switch(pixfmt) {
    case PIXFMT_888 : { // GL_RGB
        doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
        //image = new QImage(resX, resY, QImage::Format_RGB32); // x0ffRRGGBB
        break;
    }
    case PIXFMT_888_INV : { // GL_BGR
        doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
        doubleBuffer->rgbSwapped();
        break;
    }
    case PIXFMT_8888 : { // GL_RGBA
        doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB32);
        break;
    }
    case PIXFMT_8888_INV : { // GL_BGRA
        doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB32);
        doubleBuffer->rgbSwapped();
        break;
    }
    case PIXFMT_555 : { // GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1
        doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB555);
        break;
    }
    default: {
        doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
        break;
    }
    }

    //	if ( ! image || image->isNull() ) {
    //		return -1;
    //	}
    image = static_cast<QImage *>(doubleBuffer->getFrontBuffer());

//	glGenTextures(1, &texhandle);
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//	glEnable(GL_TEXTURE_2D);
//	glBindTexture(GL_TEXTURE_2D, texhandle);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width(), image->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image->bits());

    if (image && !image->isNull()) {
        //_pixmap = new QPixmap(resX, resY);
        //_pixmap->fill(QColor(Qt::black));
        return 0;
    }
    else {
        return -1;
    }
}



int SageStreamWidget::initialize(quint64 sageappid, QString appname, QRect initrect, int protocol, int port) {

        _sageAppId = sageappid;


        /* accept connection from sageStreamer */
        serversocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if ( serversocket == -1 ) {
                qCritical("SageStreamWidget::%s() : couldn't create socket", __FUNCTION__);
                return -1;
        }

        // setsockopt
        int optval = 1;
        if ( setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
                qWarning("SageStreamWidget::%s() : setsockopt SO_REUSEADDR failed",  __FUNCTION__);
        }

        // bind to port
        struct sockaddr_in localAddr, clientAddr;
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        localAddr.sin_port = htons(protocol + port);

        // bind
        if( bind(serversocket, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
                qCritical("SageStreamWidget::%s() : bind error",  __FUNCTION__);
                return -1;
        }

        // put in listen mode
        listen(serversocket, 15);

        // accept
        /** accept will BLOCK **/
//	qDebug("SageStreamWidget::%s() : Blocking waiting for sender to connect to TCP port %d", __FUNCTION__,protocol+port);
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(struct sockaddr_in);
        if ((streamsocket = accept(serversocket, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1) {
                qCritical("SageStreamWidget::%s() : accept error", __FUNCTION__);
                perror("accept");
                return -1;
        }

//	struct hostent *he = gethostbyaddr( (void *)&clientAddr, addrLen, AF_INET);
//	Q_ASSERT(he);
//	qDebug("SageStreamWidget::%s() : %s", __FUNCTION__, he->h_name);

        // read regMsg 1024Byte
        /*char regMsg[REG_MSG_SIZE];
                  sprintf(regMsg, "%d %d %d %d %d %d %d %d %d %d %d",
                                  config.streamType, // HARD_SYNC
                                  config.frameRate,
                                  winID,
                                  config.groupSize,
                                  blockSize,
                                  config.nodeNum,
                                  (int)config.pixFmt,
                                  config.blockX,
                                  config.blockY,
                                  config.totalWidth,
                                  config.totalHeight);


                                   [103 60 1 131072 12416 1 5 64 64 400 400]
                  */


        QByteArray regMsg(OldSage::REG_MSG_SIZE, '\0');
        int read = recv(streamsocket, (void *)regMsg.data(), regMsg.size(), MSG_WAITALL);
        if ( read == -1 ) {
                qCritical("SageStreamWidget::%s() : error while reading regMsg. %s",__FUNCTION__, "");
                return -1;
        }
        else if ( read == 0 ) {
                qCritical("SageStreamWidget::%s() : sender disconnected, while reading 1KB regMsg",__FUNCTION__);
                return -1;
        }

        QString regMsgStr(regMsg);
        QStringList regMsgStrList = regMsgStr.split(" ", QString::SkipEmptyParts);
        qDebug("SageStreamWidget::%s() : recved regMsg from sageStreamer::connectToRcv() [%s]",  __FUNCTION__, regMsg.constData());
        int framerate = regMsgStrList.at(1).toInt();
        int groupsize = regMsgStrList.at(3).toInt(); // this is going to be the network user buffer size
        _appInfo->setNetworkUserBufferLength(groupsize);
        int pixfmt = regMsgStrList.at(6).toInt();
        int resX = regMsgStrList.at(9).toInt();
        int resY = regMsgStrList.at(10).toInt();
        Q_ASSERT(resX > 0 && resY > 0);

//	qDebug() << "sd;fkljasdf;lkasjdf;    " << framerate << "\n";
        _perfMon->setExpectedFps( (qreal)framerate );
        _perfMon->setAdjustedFps( (qreal)framerate );

        resize(resX, resY); // BaseWidget::ResizeEvent will call setTransforOriginPoint

        /**
          set transform origin point to widget's center

          **/
//        setTransformOriginPoint( resX / 2.0 , resY / 2.0 );


        /* create double buffer */
        if ( createImageBuffer(resX, resY, (sagePixFmt)pixfmt) != 0 ) {
                qCritical("%s::%s() : imagedoublebuffer is not valid", metaObject()->className(), __FUNCTION__);
                ::shutdown(streamsocket, SHUT_RDWR);
                QMetaObject::invokeMethod(_fsmMsgThread, "sendSailShutdownMsg", Qt::QueuedConnection);
                deleteLater();
                return -1;
        }



        if(_affInfo)
                _affInfo->setWidgetID(_sageAppId);

        Q_ASSERT(_appInfo);
        _appInfo->setFrameSize(image->width(), image->height(), image->depth()); // == _image->byteCount()

//		qDebug("SageStreamWidget::%s() : sageappid %llu, groupsize %d, frameSize(SAIL) %d, frameSize(QImage) %d, expectedFps %.2f", __FUNCTION__, sageAppId, _appInfo->getNetworkUserBufferLength(), imageSize, _appInfo->getFrameBytecount(), _perfMon->getExpetctedFps());

        _appInfo->setExecutableName( appname );
        if ( appname == "imageviewer" ) {
                _appInfo->setMediaType(MEDIA_TYPE_IMAGE);
        }
        else {
                _appInfo->setMediaType(MEDIA_TYPE_VIDEO);
        }



        /* starting receiving thread */

        // image->bits() will do deep copy (detach)
        receiverThread = new SagePixelReceiver(protocol, streamsocket, /*image*/ doubleBuffer, _appInfo, _perfMon, _affInfo, /*this, mutex, wc,*/ settings);
//		qDebug("SageStreamWidget::%s() : SagePixelReceiver thread has begun",  __FUNCTION__);

        Q_ASSERT(receiverThread);

        connect(receiverThread, SIGNAL(finished()), this, SLOT(close())); // WA_Delete_on_close is defined

        // don't do below.
//		connect(receiverThread, SIGNAL(finished()), receiverThread, SLOT(deleteLater()));


//		if (!scheduler) {
                // This is queued connection because receiverThread reside outside of the main thread
                if ( ! connect(receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate())) ) {
                        qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleUpdate() slot", metaObject()->className(), __FUNCTION__);
                        return -1;
                }
                else {
//				qDebug("%s::%s() : frameReceived() -> scheduleUpdate() are connected", metaObject()->className(), __FUNCTION__);
                }
//		}
        receiverThread->start();


        /*
        QFuture<void> future = QtConcurrent::run(this, &SageStreamWidget::pixelRecvThread);
        futureWatcher.setFuture(future);
        connect(&futureWatcher, SIGNAL(finished()), this, SLOT(close()));
        connect(this, SIGNAL(pauseThread()), &futureWatcher, SLOT(pause()));
        connect(this, SIGNAL(resumeThread()), &futureWatcher, SLOT(resume()));
        */
        setPos(initrect.x(), initrect.y());

        return 0;
}


int SageStreamWidget::getPixelSize(sagePixFmt type)
{
   int bytesPerPixel = 0;
   switch(type) {
          case PIXFMT_555:
          case PIXFMT_555_INV:
          case PIXFMT_565:
          case PIXFMT_565_INV:
          case PIXFMT_YUV:
                 bytesPerPixel = 2;
                 break;
          case PIXFMT_888:
          case PIXFMT_888_INV:
                 bytesPerPixel = 3;
                 break;

          case PIXFMT_8888:
          case PIXFMT_8888_INV:
                 bytesPerPixel = 4;
                 break;

          case PIXFMT_DXT:
                 bytesPerPixel = 8;
                 break;

          default:
                 bytesPerPixel = 3;
                 break;
   }
   return bytesPerPixel;
}














//void SageStreamWidget::pixelRecvThread() {

//	/*
//	 * Initially store current affinity settings of this thread using NUMA API
//	 */
//	if (_affInfo)
//		_affInfo->figureOutCurrentAffinity();


//	/*
//	QThread *thread = this->thread();
//	// The operating system will schedule the thread according to the priority parameter. The effect of the priority parameter is dependent on the operating system's scheduling policy. In particular, the priority will be ignored on systems that do not support thread priorities (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler for more details).
//	qDebug("SagePixelReceiver::%s() : priority %d", __FUNCTION__, thread->priority());


//	pthread_setschedparam();
//	*/

//	struct rusage ru_start, ru_end;


//	int byteCount = _appInfo->getFrameSize();
////	int byteCount = pBuffer->width() * pBuffer->height() * 3;
////	unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * byteCount);
////	memset(buffer, 127, byteCount);


//	Q_ASSERT(doubleBuffer);
//	unsigned char *bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits();


////	QMutex mutex;

//	while( ! threadEnd() ) {
//		if ( _affInfo ) {
//			if ( _affInfo->isChanged() ) {
//				// apply new affinity;
//				//			qDebug("SagePixelReceiver::%s() : applying new affinity parameters", __FUNCTION__);
//				_affInfo->applyNewParameters(); // use NUMA lib

//				// update info in _affInfo
//				// this function must be called in this thread
//				_affInfo->figureOutCurrentAffinity(); // use NUMA lib
//			}
//			else {
//#if defined(Q_OS_LINUX)
//				/* this is called too many times */
//				// if cpu has changed, AffinityInfo::cpuOfMineChanged() will be emitted
//				// which is connected to ResourceMonitor::update_affInfo()
//				_affInfo->setCpuOfMine( sched_getcpu() );
//#endif
//			}
//		}


//		if(_perfMon) {
//			_perfMon->getRecvTimer().start(); //QTime::start()

//	#if defined(Q_OS_LINUX)
//			getrusage(RUSAGE_THREAD, &ru_start); // that of calling thread. Linux specific
//	#elif defined(Q_OS_MAC)
//			getrusage(RUSAGE_SELF, &ru_start);
//	#endif
//		}

//		/**
//		  this must happen after _affInfo->applyNewParameters()
//		  **/
//		if (scheduler) {
//			mutex.lock();
//			waitCond.wait(&mutex);
//		}


//		ssize_t totalread = 0;
//		ssize_t read = 0;

////		gettimeofday(&s, 0);
////		ToPixmap->acquire();
////		gettimeofday(&e, 0);
////		qreal el = ((double)e.tv_sec + (double)e.tv_usec * 0.000001) - ((double)s.tv_sec+(double)s.tv_usec*0.000001);
////		qDebug() << "acquire: " << el * 1000.0 << " msec";

////		mutex->lock();
////		unsigned char *bufptr = (imageArray[*arrayIndex])->bits();

////		if ( !image ||  !(image->bits()) ) {
////			qDebug() << "QImage is null";
////			break;
////		}
////		unsigned char *bufptr = image->bits(); // will detach().. : deep copy


////		int byteCount = pBuffer->width() * pBuffer->height() * 3;
////		pBuffer->detach();

////		qDebug() << (QTime::currentTime()).toString("mm:ss.zzz") << " recevier start receiving " << frameCounter + 1;

//		// PRODUCER
//		while (totalread < byteCount ) {
//			// If remaining byte is smaller than user buffer length (which is groupSize)
//			if ( byteCount-totalread < _appInfo->getNetworkUserBufferLength() ) {
//				read = recv(socket, bufptr, byteCount-totalread , MSG_WAITALL);
//			}
//			// otherwise, always read groupSize bytes
//			else {
//				read = recv(socket, bufptr, _appInfo->getNetworkUserBufferLength(), MSG_WAITALL);
//			}
//			if ( read == -1 ) {
//				qCritical("SagePixelReceiver::%s() : error while reading.", __FUNCTION__);
//				break;
//			}
//			else if ( read == 0 ) {
//				qDebug("SagePixelReceiver::%s() : sender disconnected", __FUNCTION__);
//				break;
//			}

//			// advance pointer
//			bufptr += read;
//			totalread += read;
//		}
//		if ( totalread < byteCount ) break;
//		read = totalread;

////		++frameCounter;
////		qDebug() << (QTime::currentTime()).toString("mm:ss.zzz") << " recevier : " << frameCounter << " received";

//		// _ts_nextframe, _deadline_missed are also updated in this function
////		qDebug() << _perfMon->set_ts_currframe() * 1000.0 << " ms";
//		_perfMon->set_ts_currframe();


//		if (_perfMon) {
//#if defined(Q_OS_LINUX)
//			getrusage(RUSAGE_THREAD, &ru_end);
//#elif defined(Q_OS_MAC)
//			getrusage(RUSAGE_SELF, &ru_end);
//#endif
//			// calculate
//			_perfMon->updateRecvLatency(read, ru_start, ru_end); // QTimer::restart()
////			ru_start = ru_end;

////			qDebug() << "SageStreamWidget" << _perfMon->getRecvFpsVariance();
//		}


//		if ( doubleBuffer ) {

//			// will wait until consumer (SageStreamWidget) consumes the data
//			doubleBuffer->swapBuffer();
////			qDebug("%s() : swapBuffer returned", __FUNCTION__);

////			emit this->frameReceived(); // Queued Connection. Will trigger SageStreamWidget::updateWidget()
////			qDebug("%s() : signal emitted", __FUNCTION__);

//			if ( ! QMetaObject::invokeMethod(this, "updateWidget", Qt::QueuedConnection) ) {
//				qCritical("%s::%s() : invoke updateWidget() failed", metaObject()->className(), __FUNCTION__);
//			}
////			static_cast<SageStreamWidget *>(this)->updateWidget();


//			// getFrontBuffer() will return immediately. There's no mutex waiting in this function
//			bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits(); // bits() will detach
////			qDebug("%s() : grabbed front buffer", __FUNCTION__);
//		}
//		else {
//			break;
//		}


//		if(scheduler) {
//			mutex.unlock();
//		}
//	}


//	/* pixel receiving thread exit */
//	qDebug("SageStreamWidget::%s() : thread exit", __FUNCTION__);
//}





