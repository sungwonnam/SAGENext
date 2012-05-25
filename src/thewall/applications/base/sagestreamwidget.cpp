#include "sagestreamwidget.h"
#include "sagepixelreceiver.h"

#include "appinfo.h"
#include "affinityinfo.h"
#include "perfmonitor.h"
#include "sn_priority.h"

#include "../../sage/sagecommondefinitions.h"
#include "../../common/commonitem.h"
#include "../../common/imagedoublebuffer.h"
#include "../../system/resourcemonitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>

//#include <QProcess>

/*
#define YUV444toRGB888(Y,U,V,R,G,B)					\
    R = clip(( 298 * (Y-16)                 + 409 * (V-128) + 128) >> 8); \
    G = clip(( 298 * (Y-16) - 100 * (U-128) - 208 * (V-128) + 128) >> 8); \
    B = clip(( 298 * (Y-16) + 516 * (U-128)
*/

#include <QGLPixelBuffer>
#include <QHostAddress>


SN_SageStreamWidget::SN_SageStreamWidget(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_RailawareWidget(globalappid, s, rm, parent, wFlags)
    , _fsmMsgThread(0)
    , _sailAppProc(0) // QProcess *
    , _sageAppId(0)
    , _receiverThread(0) // QThread *
    , _textureid(-1) // will draw this texture
    , _usePbo(false)
    , _pboBufIdx(0)
//    , _imagePointer(0)
    , doubleBuffer(0) // application buffer
    //, _pixmap(0)
    , serversocket(0)
    , streamsocket(0) // streaming channel
//    , frameCounter(0)
//	, _bordersize(0)
    , _streamProtocol(0)
	, _readyForStreamer(false) // fsm thread polls on this

//    , __firstFrame(true)
//    , __bufferMapped(false)
//    , _recvThreadEnd(false)

    , _pbomutex(0)
    , _pbobufferready(0)

    , _useShader(false)

{
	setWidgetType(SN_BaseWidget::Widget_RealTime);

//	_appInfo->setFileInfo(filename);
//	_appInfo->setSrcAddr(senderIP);

	if ( ! QObject::connect(&_initReceiverWatcher, SIGNAL(finished()), this, SLOT(startReceivingThread())) ) {
		qCritical("SN_SageStreamWidget constructor : Failed to connect _initReceiverWatcher->finished() signal to this->startReceivingThread() slot");
	}

//	_usePbo = s->value("graphics/openglpbo", false).toBool();
	if (_useOpenGL) {
		if (QGLPixelBuffer::hasOpenGLPbuffers()) {
			_usePbo = true;
		}
	}
	else {
		_usePbo = false;
	}

	//
	// Temporary
	//
    /*
    if (s->value("system/scheduler",false).toBool()) {
        Q_ASSERT(infoTextItem);
        infoTextItem->setFontPointSize(26);
        drawInfo();
    }
    */
}



bool SN_SageStreamWidget::_initPboMutex() {
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

SN_SageStreamWidget::~SN_SageStreamWidget()
{
//    qDebug() << _globalAppId << "begin destructor" << QTime::currentTime().toString("hh:mm:ss.zzz");

    if (_affInfo)  {
        if ( !  _affInfo->disconnect() ) {
            qDebug() << "affInfo disconnect() failed";
        }
    }

    /**
      Remove myself from schedulable widget list
      */
    if (_rMonitor) {
        //_affInfo->disconnect();
        _rMonitor->removeSchedulableWidget(this); // remove this from ResourceMonitor::widgetListMap
//        _rMonitor->removeApp(this); // will emit appRemoved(int) which is connected to Scheduler::loadBalance()
        //qDebug() << "affInfo removed from resourceMonitor";
    }

    disconnect(this);


    /**
      1. close the fsManager message channel
    **/
    if (_fsmMsgThread) {
        QMetaObject::invokeMethod(_fsmMsgThread, "sendSailShutdownMsg", Qt::DirectConnection);
        _fsmMsgThread->wait();
    }


    if (_receiverThread) {
        disconnect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()));

        /**
          2. The pixel receiving thread must exit from run()
          **/
        _receiverThread->endReceiver();
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



	if (_receiverThread) {
		if (_pbomutex) {
			//
			// without below two statements, _receiverThread->wait() will block forever
			//
//			_receiverThread->flip(0); // very important !!!!!!!!!!!

			pthread_cond_signal(_pbobufferready);
		}
    /**
      5. make sure receiverThread finished safely
      **/
		_receiverThread->wait();

    /**
      6. Then schedule deletion
      **/
		_receiverThread->deleteLater();
	}

    /**
      7. Now the double buffer can be deleted
      **/
    if (doubleBuffer) delete doubleBuffer;
    doubleBuffer = 0;


	if (_useOpenGL && glIsTexture(_textureid)) {
		glDeleteTextures(1, &_textureid);
	}

	if (_usePbo) {
//		_recvThreadEnd = true;
//		_recvThreadFuture.cancel();
//		_recvThreadWatcher.cancel();

		glDeleteBuffersARB(2, _pboIds);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

		if (_pbobufferready) {
//			__bufferMapped = true;
			pthread_cond_signal(_pbobufferready);
		}
		if (_pbomutex) {
			pthread_mutex_unlock(_pbomutex);
	//		pthread_mutex_destroy(_mutex);
		}

//		_recvThreadFuture.waitForFinished();
//		qDebug() << "~SageStreamWidget finished\n\n";
		free(_pbobufferready);
		free(_pbomutex);
	}

	// this causes other sagestreamwidget gets killed
	// don't know why
//	if (_sailAppProc) {
//		_sailAppProc->kill();
//	}

//    qDebug() << _globalAppId << "end destructor" << QTime::currentTime().toString("hh:mm:ss.zzz");
    qDebug("%s::%s() ",metaObject()->className(),  __FUNCTION__);
}


int SN_SageStreamWidget::setQuality(qreal newQuality) {
    if (!_perfMon) return -1;

    if (_quality == newQuality) {
        return 0;
    }

    unsigned long delayneeded = 0;


    if ( newQuality >= 1.0 ) {
		_quality = 1.0;
        delayneeded = 0;
	}

	else {
        //
        // newQuality == 0 can happen when
        // 1. this app's requiredBW is set to 0 by itself
        // or
        // 2. its priority is 0 (completely obscured by other widget for instance)
        //
        // And it means the streamer (SAGE app) isn't sending any pixel
        //
        if ( newQuality <= 0.0 ) {
            _quality = 0.1;
        }
        else {
            _quality = newQuality;
        }

        //    qreal BWallowed_Mbps = _perfMon->getRequiredBW_Mbps( _quality );
        qreal newfps = _quality * _perfMon->getExpetctedFps();
        delayneeded = 1000 * ((1.0/newfps) - (1.0/_perfMon->getExpetctedFps()));
	}


    if (_receiverThread) {
        if ( ! QMetaObject::invokeMethod(_receiverThread, "setDelay_msec", Qt::QueuedConnection, Q_ARG(unsigned long, delayneeded)) ) {
            qDebug() << "SN_SageStreamWidget::setQuality() : failed to invoke setDelay_msec()";
            return -1;
        }
    }
    else {
        qDebug() << "SN_SageStreamWidget::setQuality() : _receiverThread is null";
        return -1;
    }

    return 0;
}



/**
   This slot is invoked by fsManagerMsgThread right after sending SAIL_INIT_MSG.
  It runs waitForPixelStreamerConnection() does blocking waiting for the streamer in a separate thread.
  */
void SN_SageStreamWidget::doInitReceiver(quint64 sageappid, const QString &appname, const QRect &initrect, int protocol, int port) {
//	qDebug() << "\nSN_SageStreamWidget::doInitReceiver() : Running waitForPixelStreamConnection() in a separate thread";
	Q_UNUSED(initrect);
	_sageAppId = sageappid;

	//
	// receives REG_MSG from sail
	//
	_streamerConnected = QtConcurrent::run(this, &SN_SageStreamWidget::waitForPixelStreamerConnection, protocol, port, appname);
	_initReceiverWatcher.setFuture(_streamerConnected);
}

/**
  Now we know all the sail info and streaming channel established !

  This is invoked (by _initReceiverWatcher) after the waitForPixelStreamingConnection() thread finishes.
  */
void SN_SageStreamWidget::startReceivingThread() {
	Q_ASSERT(streamsocket > 0);

	if (_useOpenGL) {

		glGenTextures(1, &_textureid);

		if (_useShader) {
			GLchar *FragmentShaderSource;
			GLchar *VertexShaderSource;

			char *sfn, *sd;
			sfn = (char*)malloc(256);
			memset(sfn, 0, 256);
			sd = getenv("SAGE_DIRECTORY");
			sprintf(sfn, "%s/bin/yuv", sd);

			GLSLreadShaderSource(sfn, &VertexShaderSource, &FragmentShaderSource);
			_shaderProgHandle = GLSLinstallShaders(VertexShaderSource, FragmentShaderSource);

			/* Finally, use the program. */
			glUseProgramObjectARB(_shaderProgHandle);
			free(sfn);
			glUseProgramObjectARB(0);
		}

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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
		}
	}

	qDebug() << "SN_SageStreamWidget (" << _globalAppId << _sageAppId << ") now starts its receiving thread.";
	qDebug() << "\t" << "app name" << _appInfo->executableName() << ",media file" << _appInfo->fileInfo().fileName() << "from" << _appInfo->srcAddr();
	qDebug() << "\t" << _appInfo->nativeSize().width() <<"x" << _appInfo->nativeSize().height() << _appInfo->bitPerPixel() << "bpp" << _appInfo->frameSizeInByte() << "Byte/frame at" << _perfMon->getExpetctedFps() << "fps";
	qDebug() << "\t" << "network user buffer length (groupsize)" << _appInfo->networkUserBufferLength() << "Byte";
	qDebug() << "\t" << "GL pixel format" << _pixelFormat << ",use SHADER (for YUV format)" << _useShader << ",use OpenGL PBO" << _usePbo;


	_receiverThread = new SN_SagePixelReceiver(_streamProtocol, streamsocket, doubleBuffer, _usePbo, _pbobufarray, _pbomutex, _pbobufferready, _appInfo, _perfMon, _affInfo, _settings);
    Q_ASSERT(_receiverThread);

    QObject::connect(_receiverThread, SIGNAL(finished()), this, SLOT(close())); // WA_Delete_on_close is defined

    // don't do below.
//		connect(receiverThread, SIGNAL(finished()), receiverThread, SLOT(deleteLater()));

//		if (!scheduler) {
            // This is queued connection because receiverThread reside outside of the main thread

	if (_usePbo) {
		if ( ! QObject::connect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(schedulePboUpdate())) ) {
			qCritical("%s::%s() : Failed to connect frameReceived() signal and schedulePboUpdate() slot", metaObject()->className(), __FUNCTION__);
			return;
        }

		///
		// schedulePboUpdate() must be called once before the _receiverThread emits the frameReceived() signal
		// to wake up the thread
		///
		//QObject::connect(_receiverThread, SIGNAL(started()), this, SLOT(schedulePboUpdate()));
	}
	else {
		if ( ! QObject::connect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate())) ) {
			qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleUpdate() slot", metaObject()->className(), __FUNCTION__);
			return;
		}
	}
//		}
    _receiverThread->start();

}



void SN_SageStreamWidget::schedulePboUpdate() {

	Q_ASSERT(_pbomutex);
	Q_ASSERT(_pbobufferready);
	Q_ASSERT(_appInfo);

//	qint64 ss,ee;
//	if (_globalAppId == 1) {
//		ss = QDateTime::currentMSecsSinceEpoch();
//	}


//	_perfMon->getUpdtTimer().start();

	//
	// flip array index
	//
	_pboBufIdx = (_pboBufIdx + 1) % 2;
	int nextbufidx = (_pboBufIdx + 1) % 2;

	GLenum error = glGetError();

    static bool isFirstFrame = true;

	//
	// unmap the previous buffer
	//
	if (!isFirstFrame) {
//		qDebug() << "unmap buffer" << nextbufidx;
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[nextbufidx]);
		if ( ! glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB) ) {
			qDebug() << "SN_SageStreamWidget::schedulePboUpdate() : glUnmapBufferARB() failed";
		}
	}

    //
    // If it's the first time then nothing has been mapped yet
    //
	else {
        isFirstFrame = false;
	}



    //
	// update the texture with the pbo buffer. use offset instead of pointer
	//
//	qDebug() << "update texture" << nextbufidx;
	glBindTexture(/*GL_TEXTURE_2D*/GL_TEXTURE_RECTANGLE_ARB, _textureid);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pboIds[nextbufidx]);
	glTexSubImage2D(/*GL_TEXTURE_2D */GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, _appInfo->nativeSize().width(), _appInfo->nativeSize().height(), _pixelFormat, GL_UNSIGNED_BYTE, 0);

	//
	// schedule paintEvent
	//
	update();


//    if (_globalAppId == 1) {
//		ee = QDateTime::currentMSecsSinceEpoch();
//        qDebug() << "schedulePboUpdate : update() called " << ee-ss << "msec";
//        ss = ee;
//	}



	//
	// map the other buffer
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
       // the receiving thread will emit frameReceived() right before it blocking waits for the condition
        // to prevent lost wakeup situation
        //

//		qDebug() << "mapped buffer" << _pboBufIdx << ptr;

		pthread_cond_signal(_pbobufferready);
	}
	else {
		qCritical() << "SN_SageStreamWidget::schedulePboUpdate() : glMapBUffer failed()";
	}


	//
	// reset GL state
	//
	glBindTexture(/*GL_TEXTURE_2D*/GL_TEXTURE_RECTANGLE_ARB, 0);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

//	_perfMon->updateUpdateDelay();
	
//	if (_globalAppId==1) {
//		ee = QDateTime::currentMSecsSinceEpoch();
//		qDebug() << "schedulePboUpdate() : mapped, signaled" << ee-ss << "msec";
//	}
}


/**
  This slot connected to the signal SagePixelReceiver::frameReceived() so that this can be called after receiving each image frame
  */
void SN_SageStreamWidget::scheduleUpdate() {

    if ( !doubleBuffer || !_receiverThread || _receiverThread->isFinished() || !isVisible() )
        return;

	//qDebug() << _globalAppId << "scheduleUpdate" << QTime::currentTime().toString("hh:mm:ss.zzz");

	const QImage *constImageRef = static_cast<QImage *>(doubleBuffer->getBackBuffer());

	if ( !constImageRef || constImageRef->isNull()) {
		qCritical("SageStreamWidget::%s() : globalAppId %llu, sageAppId %llu : imgPtr is null. Failed to retrieve back buffer from double buffer", __FUNCTION__, globalAppId(), _sageAppId);
		return;
	}

//    if (_perfMon)
//        _perfMon->getUpdtTimer().start();

	if (_useOpenGL) {
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, constImageRef->width(), constImageRef->height(), _pixelFormat, GL_UNSIGNED_BYTE, constImageRef->bits());

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glEnable(GL_TEXTURE_2D);

			// QGLContext::InvertedYBindOption Because In OpenGL 0,0 is bottom left, In Qt 0,0 is top left
			//
			// Below is awfully slow. Probably because QImage::scaled() inside qgl.cpp to convert image size near power of two
			// _textureid = glContext->bindTexture(constImageRef->convertToFormat(QImage::Format_RGB32), GL_TEXTURE_2D, QGLContext::InvertedYBindOption);

//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	else {
		/*
		  // drawing RGB32 is the fastest. But drawing QImage is slower than drawing QPixmap
		_imageForDrawing = constImageRef->convertToFormat(QImage::Format_RGB32);
		if (_imageForDrawing.isNull()) {
			qCritical("SN_SageStreamWidget::%s() : Image conversion failed", __FUNCTION__);
			return;
		}
		*/

		//
	    // Drawing QPixmap is the fastest. But in X11, this means image is converted and copied to X Server -> slow !
	    // For static images this is ok but for animations this results bad frame rate
	    //
	   /*
      _pixmap = QPixmap::fromImage(*constImageRef);
      if (_pixmap.isNull()) {
	   qCritical("SageStreamWidget::scheduleUpdate() : QPixmap::fromImage() error");
	   return;
      }
      */

		//
		// Below doesn't work under X11 backend. Use raster backend
		//
		_pixmapForDrawing.convertFromImage(*constImageRef, Qt::AutoColor | Qt::ThresholdDither);


   //	There's small conversion delay but drawing is faster
   //	_imageForDrawing = constImageRef->convertToFormat(QImage::Format_RGB32); // faster drawing !!
   //	_imageForDrawing = constImageRef->convertToFormat(QImage::Format_ARGB32_Premultiplied); // faster drawing !!

	   // Don't create QImage like this. This slot is called frequently
   //	_imageForDrawing = QImage(rawptr, doubleBuffer->imageWidth(), doubleBuffer->imageHeight(), doubleBuffer->imageFormat()).convertToFormat(QImage::Format_RGB32);

   //	_imageForDrawing = constImageRef->convertToFormat(QImage::Format_RGB32);
   //	if (_imageForDrawing.isNull()) {
   //		qCritical("SN_SageStreamWidget::%s() : Image conversion failed", __FUNCTION__);
   //		return;
   //	}
	}

//    if (_perfMon)
//	_perfMon->updateUpdateDelay();

	setScheduled(false); // reset scheduling flag for SMART scheduler

	//qDebug() << QTime::currentTime().toString("mm:ss.zzz") << " widget : " << frameCounter << " has converted";

	/*
	  Maybe I should schedule update() and releaseBackBuffer in the scheduler
	 */
	doubleBuffer->releaseBackBuffer();


	// Schedules a redraw. This is not an immediate paint. This actually is postEvent()
	// QGraphicsView will process the event
	update(); // post paint event to myself
	//qApp->sendPostedEvents(this, QEvent::MetaCall);
	//qApp->flush();
	//qApp->processEvents();

	//this->scene()->views().front()->update( mapRectToScene(boundingRect()).toRect() );
}



void SN_SageStreamWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
//	if (_perfMon) {
//		_perfMon->getDrawTimer().start();
//	}
//	painter->setCompositionMode(QPainter::CompositionMode_Source);

	if (_useOpenGL && painter->paintEngine()->type() == QPaintEngine::OpenGL2
	//|| painter->paintEngine()->type() == QPaintEngine::OpenGL
	)
	{
		// 0 draw latency because it's drawn from the cache
		// but higher latency in scheduleUpdate()
		//
		//	QGLWidget *viewportWidget = (QGLWidget *)w;
		//	viewportWidget->drawTexture(QPointF(0,0), _textureid);

		//
		// This takes lots of time when doing DMA write to GPU memory using QGLBuffer due to the context switching.
		// However, it's just fine with pure OpenGL calls.
		//
		painter->beginNativePainting();

		if (_useShader) {
			glUseProgramObjectARB(_shaderProgHandle);
			glActiveTexture(GL_TEXTURE0);
			int h=glGetUniformLocationARB(_shaderProgHandle,"yuvtex");
			glUniform1iARB(h,0);  /* Bind yuvtex to texture unit 0 */
		}

		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

		glBegin(GL_QUADS);
		//
		// below is same with QGLContext::InvertedYBindOption
		//
		glTexCoord2f(0.0,            size().height()); glVertex2f(0,              size().height());
		glTexCoord2f(size().width(), size().height()); glVertex2f(size().width(), size().height());
		glTexCoord2f(size().width(), 0.0);             glVertex2f(size().width(), 0);
		glTexCoord2f(0.0,            0.0);             glVertex2f(0,              0);

		//
		// below is normal (In OpenGL, 0,0 is bottom-left, In Qt, 0,0 is top-left)
		//
		//glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
		//glTexCoord2f(1.0, 1.0); glVertex2f(_imagePointer->width(), 0);
		//glTexCoord2f(1.0, 0.0); glVertex2f(_imagePointer->width(), _imagePointer->height());
		//glTexCoord2f(0.0, 0.0); glVertex2f(0, _imagePointer->height());

		glEnd();

		if (_useShader) glUseProgramObjectARB(0);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
		glEnable(GL_TEXTURE_2D);

		painter->endNativePainting();
	}
	else {
		/***
		  On Linux/X11 platforms which support it, Qt will use glX/EGL texture-from-pixmap extension.
	This means that if your QPixmap has a real X11 pixmap backend, we simply bind that X11 pixmap as a texture and avoid copying it.
	You will be using the X11 pixmap backend if the pixmap was created with QPixmap::fromX11Pixmap() or you’re using the “native” graphics system.
	Not only does this avoid overhead but it also allows you to write a composition manager or even a widget which shows previews of all your windows.


	QPixmap, unlike QImage, may be hardware dependent.
	On X11, Mac and Symbian, a QPixmap is stored on the server side while a QImage is stored on the client side
	(on Windows,these two classes have an equivalent internal representation,
	i.e. both QImage and QPixmap are stored on the client side and don't use any GDI resources).

	So, drawing pixmap is much faster but QImage has to be converted to QPixmap for every frame which involves converting plus copy to X Server.
		  ***/

		if (!_pixmapForDrawing.isNull()) {
			painter->drawPixmap(0, 0, _pixmapForDrawing);
		}
        else {
            QPixmap pm(size().toSize());
            pm.loadFromData((uchar *)(_pbobufarray[_pboBufIdx]), _appInfo->frameSizeInByte(), 0, Qt::AutoColor | Qt::ThresholdDither);
            painter->drawPixmap(0, 0, pm);
        }
	}

	SN_BaseWidget::paint(painter,o,w);

//	if (_perfMon)
//		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
	
}




/**
  Receive SAGE message from the sender

  This slot will run in a separate thread.
  */
int SN_SageStreamWidget::waitForPixelStreamerConnection(int protocol, int port, const QString &appname) {
	_streamProtocol = protocol;

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
//	qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConn() : sageappid" << _sageAppId << "Before accept(). TCP port" << protocol+port << QTime::currentTime().toString("hh:mm:ss.zzz");

    memset(&clientAddr, 0, sizeof(clientAddr));
    int addrLen = sizeof(struct sockaddr_in);

    //
    // The fsmMsgThread for this widget will blocing wait for this bool variable
    // right before it sends SAIL_CONNECT_TO_RCV to the streamer.
    //
	_readyForStreamer = true;

    //qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConnection() : about to enter blocking waiting (accept()). GID" << _globalAppId;

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

	QHostAddress srcaddr((struct sockaddr *)&clientAddr);
	_appInfo->setSrcAddr(srcaddr.toString());


    QByteArray regMsg(OldSage::REG_MSG_SIZE, '\0');
    int read = recv(streamsocket, (void *)regMsg.data(), regMsg.size(), MSG_WAITALL);
    if ( read == -1 ) {
            qCritical("SageStreamWidget::%s() : error while reading regMsg. %s",__FUNCTION__, "");
            return -1;
    }
    else if ( read == 0 ) {
            qCritical("SageStreamWidget::%s() : sender disconnected, while reading regMsg",__FUNCTION__);
            return -1;
    }

    QString regMsgStr(regMsg);
    QStringList regMsgStrList = regMsgStr.split(" ", QString::SkipEmptyParts);
//    qDebug("SageStreamWidget::%s() : recved regMsg, port %d, sageStreamer::connectToRcv() [%s]",  __FUNCTION__, protocol+port, regMsg.constData());
    int framerate = regMsgStrList.at(1).toInt();
    int groupsize = regMsgStrList.at(3).toInt(); // this is going to be the network user buffer size

    _appInfo->setNetworkUserBufferLength(groupsize);

    int pixfmt = regMsgStrList.at(6).toInt();
    int resX = regMsgStrList.at(9).toInt();
    int resY = regMsgStrList.at(10).toInt();
    Q_ASSERT(resX > 0 && resY > 0);


	//	int fmargin = _settings->value("gui/framemargin",0).toInt();
	//    resize(resX + fmargin*2, resY + fmargin*2); // BaseWidget::ResizeEvent will call setTransforOriginPoint
	resize(resX, resY);
	_appInfo->setFrameSize(resX, resY, getPixelSize((sagePixFmt)pixfmt) * 8);

	if ( (sagePixFmt)pixfmt == PIXFMT_YUV ) {
//		qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConnection() : PIXFMT_YUV -> use Shader";
		_useShader = true;
	}

//	qDebug() << "SN_SageStreamWidget : streamer connected. groupSize" << _appInfo->networkUserBufferLength() << "Byte. Framerate" << framerate << "fps";
    if (framerate > 0 && _perfMon) {
        _perfMon->setPriori(true);
        _perfMon->setExpectedFps( (qreal)framerate );
        _perfMon->setAdjustedFps( (qreal)framerate );

        /*!
          A priori (in Mbps)
         */
        _perfMon->setRequiredBW_Mbps( (_appInfo->frameSizeInByte() * 8 * (qreal)framerate) / 1e+6 );
    }


    /* create double buffer if PBO is disabled */
	if (!_usePbo) {
		if ( createImageBuffer(resX, resY, (sagePixFmt)pixfmt) != 0 ) {
			qCritical("%s::%s() : imagedoublebuffer is not valid", metaObject()->className(), __FUNCTION__);
			::shutdown(streamsocket, SHUT_RDWR);
			QMetaObject::invokeMethod(_fsmMsgThread, "sendSailShutdownMsg", Qt::QueuedConnection);
			deleteLater();
			return -1;
		}
	}

    if(_affInfo)
		_affInfo->setWidgetID(_sageAppId);

//		qDebug("SageStreamWidget::%s() : sageappid %llu, groupsize %d, frameSize(SAIL) %d, frameSize(QImage) %d, expectedFps %.2f", __FUNCTION__, sageAppId, _appInfo->getNetworkUserBufferLength(), imageSize, _appInfo->getFrameBytecount(), _perfMon->getExpetctedFps());

    _appInfo->setExecutableName( appname );

	if ( appname == "checker" ) {
		QString arg = "";
		arg.append(QString::number(0)); arg.append(" ");
		arg.append(QString::number(resX)); arg.append(" ");
		arg.append(QString::number(resY)); arg.append(" ");
		arg.append(QString::number(framerate));
		_appInfo->setCmdArgs(arg);

//		qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConnection() : srcAddr :" << _appInfo->srcAddr();
//		qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConnection() : Execname(sageappname) :" << _appInfo->executableName();
//		qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConnection() : CmdArgs :" << _appInfo->cmdArgsString();
	}

    emit streamerInitialized();

//	qDebug() << "waitForStreamerConnection returning";
	return streamsocket;
}


int SN_SageStreamWidget::createImageBuffer(int resX, int resY, sagePixFmt pixfmt) {
//    int bytePerPixel = getPixelSize(pixfmt);
//    int memwidth = resX * bytePerPixel; //Byte (single row of frame)
//    qDebug("%s::%s() : recved regMsg. size %d x %d, pixfmt %d, pixelSize %d Byte, bytecount %d Byte", metaObject()->className(), __FUNCTION__, resX, resY, pixfmt, bytePerPixel, resX * bytePerPixel * resY);

	Q_ASSERT(!_usePbo);

	if (!doubleBuffer)
		doubleBuffer = new ImageDoubleBuffer;

    /*
         Do not draw ARGB32 images into the raster engine.
         ARGB32_premultiplied and RGB32 are the best ! (they are pixel wise compatible)
         http://labs.qt.nokia.com/2009/12/18/qt-graphics-and-performance-the-raster-engine/
      */
    switch(pixfmt) {
	case PIXFMT_YUV : {
		_pixelFormat = GL_LUMINANCE_ALPHA; // internal format is 2

		//
		// QImage::Format_RGB444 is wrong format for displaying purpose.
		// It is used anyway because it also is 2 Byte per pixel.
		// So, for the pixel receiving purpose, this is ok.
		//
		if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB444); // 16 bit (2 Byte per pixel)
		break;
	}
    case PIXFMT_888 : { // GL_RGB
		_pixelFormat = GL_RGB;
        if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
        //image = new QImage(resX, resY, QImage::Format_RGB32); // x0ffRRGGBB
        break;
    }
    case PIXFMT_888_INV : { // GL_BGR
		_pixelFormat = GL_BGR;
        if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
        if (doubleBuffer) doubleBuffer->rgbSwapped();
        break;
    }
    case PIXFMT_8888 : { // GL_RGBA
		_pixelFormat = GL_RGBA;
        if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB32);
        break;
    }
    case PIXFMT_8888_INV : { // GL_BGRA
		_pixelFormat = GL_BGRA;
        if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB32);
        if (doubleBuffer) doubleBuffer->rgbSwapped();
        break;
    }
    case PIXFMT_555 : { // GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1
        if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB555);
        break;
    }
    default: {
		_pixelFormat = GL_RGB;
        if (doubleBuffer) doubleBuffer->initBuffer(resX, resY, QImage::Format_RGB888);
        break;
    }
    }

	return 0;
}

int SN_SageStreamWidget::getPixelSize(sagePixFmt type)
{
	int bytesPerPixel = 0;
	switch(type) {
	case PIXFMT_555:
	case PIXFMT_555_INV:
	case PIXFMT_565:
	case PIXFMT_565_INV:

	case PIXFMT_888: {
		_pixelFormat = GL_RGB;
		bytesPerPixel = 3;
		break;
	}
	case PIXFMT_888_INV: {
		_pixelFormat = GL_BGR;
		bytesPerPixel = 3;
		break;
	}

	case PIXFMT_8888: {
		_pixelFormat = GL_RGBA;
		bytesPerPixel = 4;
		break;
	}
	case PIXFMT_8888_INV: {
		_pixelFormat = GL_BGRA;
		bytesPerPixel = 4;
		break;
	}

	case PIXFMT_YUV: {
		_pixelFormat = GL_LUMINANCE_ALPHA;
		bytesPerPixel = 2;
		break;
	}

	case PIXFMT_DXT: {
		bytesPerPixel = 8;
		break;
	}

	default: {
		_pixelFormat = GL_RGB;
		bytesPerPixel = 3;
		break;
	}
	}
	return bytesPerPixel;
}










int GLSLprintlError(const char * file, int line)
{
   //
   // Returns 1 if an OpenGL error occurred, 0 otherwise.
   //
   GLenum glErr;
   int    retCode = 0;

   glErr = glGetError();
   while (glErr != GL_NO_ERROR)
   {
      fprintf(stderr, "GLSL> glError in file %s @ line %d: %s\n", file, line , gluErrorString(glErr));
      retCode = 1;
      glErr = glGetError();
   }
   return retCode;
}

//
// Print out the information log for a shader object
//
static void GLSLprintShaderInfoLog(GLuint shader)
{
   int infologLength = 0;
   int charsWritten  = 0;
   GLchar *infoLog;

   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors

   glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);

   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors

   if (infologLength > 0)
   {
      infoLog = (GLchar *)malloc(infologLength);
      if (infoLog == NULL)
      {
         fprintf(stderr, "GLSL> ERROR: Could not allocate InfoLog buffer\n");
         exit(1);
      }
      glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
      fprintf(stderr, "GLSL> Shader InfoLog:%s\n", infoLog);
      free(infoLog);
   }
   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors
}

//
// Print out the information log for a program object
//
static void GLSLprintProgramInfoLog(GLuint program)
{
   int infologLength = 0;
   int charsWritten  = 0;
   GLchar *infoLog;

   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors

   glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);

   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors

   if (infologLength > 0)
   {
      infoLog = (GLchar *)malloc(infologLength);
      if (infoLog == NULL)
      {
         fprintf(stderr, "GLSL> ERROR: Could not allocate InfoLog buffer\n");
         exit(1);
      }
      glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);
      fprintf(stderr, "GLSL> Program InfoLog:%s\n", infoLog);
      free(infoLog);
   }
   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors
}

static int GLSLShaderSize(char *fileName, int shaderType)
{
   //
   // Returns the size in bytes of the shader fileName.
   // If an error occurred, it returns -1.
   //
   // File name convention:
   //
   // <fileName>.vert
   // <fileName>.frag
   //
   int fd;
   char name[256];
   int count = -1;

   memset(name, 0, 256);
   strcpy(name, fileName);

   switch (shaderType)
   {
      case GLSLVertexShader:
         strcat(name, ".vert");
         break;
      case GLSLFragmentShader:
         strcat(name, ".frag");
         break;
      default:
         fprintf(stderr, "GLSL> ERROR: unknown shader file type\n");
         exit(1);
         break;
   }

   //
   // Open the file, seek to the end to find its length
   //
#ifdef WIN32
   fd = _open(name, _O_RDONLY);
   if (fd != -1)
   {
      count = _lseek(fd, 0, SEEK_END) + 1;
      _close(fd);
   }
#else
   fd = open(name, O_RDONLY);
   if (fd != -1)
   {
      count = lseek(fd, 0, SEEK_END) + 1;
      close(fd);
   }
#endif
   return count;
}

static
int GLSLreadShader(char *fileName, int shaderType, char *shaderText, int size)
{
   //
   // Reads a shader from the supplied file and returns the shader in the
   // arrays passed in. Returns 1 if successful, 0 if an error occurred.
   // The parameter size is an upper limit of the amount of bytes to read.
   // It is ok for it to be too big.
   //
   FILE *fh;
   char name[100];
   int count;

   strcpy(name, fileName);

   switch (shaderType)
   {
      case GLSLVertexShader:
         strcat(name, ".vert");
         break;
      case GLSLFragmentShader:
         strcat(name, ".frag");
         break;
      default:
         fprintf(stderr, "GLSL> ERROR: unknown shader file type\n");
         exit(1);
         break;
   }

   //
   // Open the file
   //
   fh = fopen(name, "r");
   if (!fh)
      return -1;

   //
   // Get the shader from a file.
   //
   fseek(fh, 0, SEEK_SET);
   count = (int) fread(shaderText, 1, size, fh);
   shaderText[count] = '\0';

   if (ferror(fh))
      count = 0;

   fclose(fh);
   return count;
}

int GLSLreadShaderSource(char *fileName, GLchar **vertexShader, GLchar **fragmentShader)
{
   int vSize, fSize;

   //
   // Allocate memory to hold the source of our shaders.
   //

   fprintf(stderr, "GLSL> load shader %s\n", fileName);
   if (vertexShader) {
      vSize = GLSLShaderSize(fileName, GLSLVertexShader);

      if (vSize == -1) {
         fprintf(stderr, "GLSL> Cannot determine size of the vertex shader %s\n", fileName);
         return 0;
      }

      *vertexShader = (GLchar *) malloc(vSize);

      //
      // Read the source code
      //
      if (!GLSLreadShader(fileName, GLSLVertexShader, *vertexShader, vSize)) {
         fprintf(stderr, "GLSL> Cannot read the file %s.vert\n", fileName);
         return 0;
      }
   }

   if (fragmentShader) {
      fSize = GLSLShaderSize(fileName, GLSLFragmentShader);

      if (fSize == -1) {
         fprintf(stderr, "GLSL> Cannot determine size of the fragment shader %s\n", fileName);
         return 0;
      }

      *fragmentShader = (GLchar *) malloc(fSize);

      if (!GLSLreadShader(fileName, GLSLFragmentShader, *fragmentShader, fSize)) {
         fprintf(stderr, "GLSL> Cannot read the file %s.frag\n", fileName);
         return 0;
      }
   }

   return 1;
}

GLuint GLSLinstallShaders(const GLchar *Vertex, const GLchar *Fragment)
{
   GLuint VS, FS, Prog;   // handles to objects
   GLint  vertCompiled, fragCompiled;    // status values
   GLint  linked;

   // Create a program object
   Prog = glCreateProgram();

   // Create a vertex shader object and a fragment shader object
   if (Vertex) {
      VS = glCreateShader(GL_VERTEX_SHADER);

      // Load source code strings into shaders
      glShaderSource(VS, 1, &Vertex, NULL);

      // Compile the vertex shader, and print out
      // the compiler log file.

      glCompileShader(VS);
      GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors
      glGetShaderiv(VS, GL_COMPILE_STATUS, &vertCompiled);
      GLSLprintShaderInfoLog(VS);

      if (!vertCompiled)
         return 0;

      glAttachShader(Prog, VS);
   }

   if (Fragment) {
      FS = glCreateShader(GL_FRAGMENT_SHADER);

      glShaderSource(FS, 1, &Fragment, NULL);

      glCompileShader(FS);
      GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors
      glGetShaderiv(FS, GL_COMPILE_STATUS, &fragCompiled);
      GLSLprintShaderInfoLog(FS);

      if (!fragCompiled)
         return 0;

      glAttachShader(Prog, FS);
   }

   // Link the program object and print out the info log
   glLinkProgram(Prog);
   GLSLprintlError(__FILE__, __LINE__);  // Check for OpenGL errors
   glGetProgramiv(Prog, GL_LINK_STATUS, &linked);
   GLSLprintProgramInfoLog(Prog);

   if (!linked)
      return 0;

   return Prog;
}





void SN_SageStreamWidget::updateInfoTextItem() {
	if (!infoTextItem || !_showInfo) return;

    QString text = "";

	QByteArray priorityText(256, '\0');
	if(_priorityData) {
		sprintf(priorityText.data(), "%llu\n%.2f (Win %hu, Wal %hu, ipm %.3f)"
		        , _globalAppId
                , priority() /* qreal */
                , _priorityData->evrToWin() /* unsigned short - quint16 */
                , _priorityData->evrToWall()  /* unsigned short - quint16 */
                , _priorityData->ipm() /* qreal */
		        );
	}

    QByteArray qualityText(256, 0);
    sprintf(qualityText.data(), "\nOQ_Rq %.2f / OQ_Dq %.2f / DQ %.2f"
            , observedQuality_Rq()
            , observedQuality_Dq()
            , demandedQuality()
            );

    QByteArray perfText(256, 0);
    qreal totaldelay = 1.0 / _perfMon->getCurrEffectiveFps(); // in second
    qreal cputime = _perfMon->getCpuUsage() * totaldelay; // in second
    cputime *= 1000; // millisecond

    sprintf(perfText.data(), "\nC %.2f / A %.2f (E %.2f) \n CurBW %.3f ReqBW %.3f \n CPU %.3f"
//            , _appInfo->frameSizeInByte()
            , _perfMon->getCurrEffectiveFps()
            , _perfMon->getAdjustedFps()
            , _perfMon->getExpetctedFps()

            , _perfMon->getCurrBW_Mbps()
            , _perfMon->getRequiredBW_Mbps()

            , _perfMon->getCpuTimeSpent_sec() * 1000.0
            );

	if (infoTextItem) {
        text.append(priorityText);
        text.append(qualityText);
        text.append(perfText);

		infoTextItem->setText(text);
		infoTextItem->update();
//        infoTextItem->setScale(1.6);
	}
}







/*!
  This will run in a separate thread
  */
/***
void SN_SageStreamWidget::__recvThread() {
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

	ssize_t totalread = 0;

	while(!_recvThreadEnd) {
		// wait for glMapBufferARB
		pthread_mutex_lock(_pbomutex);
//		qDebug() << QDateTime::currentMSecsSinceEpoch() << "will wait";
		while(!__bufferMapped)
			pthread_cond_wait(_pbobufferready, _pbomutex);


		gettimeofday(&lats, 0);

		// receive frame (DMA write to GPU memory)
		// pointer in the _bufarray must be valid at this point
		totalread = __recvFrame(streamsocket, _appInfo->frameSizeInByte(), _pbobufarray[_pboBufIdx]);
		if (totalread <= 0 ) break;

		gettimeofday(&late, 0);


		// reset flag
		__bufferMapped = false;

		// schedule screen update
		emit frameReady();


		if (_perfMon) {
#if defined(Q_OS_LINUX)
			getrusage(RUSAGE_THREAD, &ru_end);
#elif defined(Q_OS_MAC)
			getrusage(RUSAGE_SELF, &ru_end);
#endif
			qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);

			// calculate
			_perfMon->updateObservedRecvLatency(totalread, networkrecvdelay, ru_start, ru_end);
			ru_start = ru_end;
		}

		pthread_mutex_unlock(_pbomutex);
	}
	qDebug() << "SN_SageStreamWidget::__recvThread finished\n";
}

ssize_t SN_SageStreamWidget::__recvFrame(int socket, int byteCount, void *ptr) {
	Q_ASSERT(socket);

	ssize_t totalread = 0;
	ssize_t read = 0;

//	gettimeofday(&lats, 0);

	GLubyte *bufptr = (GLubyte *)ptr;

	// PIXEL RECEIVING
	while (totalread < byteCount ) {
		// If remaining byte is smaller than user buffer length (which is groupSize)
		if ( byteCount-totalread < _appInfo->networkUserBufferLength() ) {
			read = recv(socket, bufptr, byteCount-totalread , MSG_WAITALL);
		}
		// otherwise, always read groupSize bytes
		else {
			read = recv(socket, bufptr, _appInfo->networkUserBufferLength(), MSG_WAITALL);
		}
		if ( read == -1 ) {
			qDebug("SagePixver::run() : error while reading.");
			break;
		}
		else if ( read == 0 ) {
			qDebug("SagePiver::run() : sender disconnected");
			break;
		}
		// advance pointer
		bufptr += read;
		totalread += read;
	}
	if ( totalread < byteCount ) {
		qDebug() << "totalread < bytecount";
	}

	return totalread;
}
***/





/*
int SageStreamWidget::initialize(quint64 sageappid, QString appname, QRect initrect, int protocol, int port) {

        _sageAppId = sageappid;

        // accept connection from sageStreamer
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
//	qDebug("SageStreamWidget::%s() : Blocking waiting for sender to connect to TCP port %d", __FUNCTION__,protocol+port);
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(struct sockaddr_in);
        if ((streamsocket = accept(serversocket, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1) {
                qCritical("SageStreamWidget::%s() : accept error", __FUNCTION__);
                perror("accept");
                return -1;
        }



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


        // create double buffer
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
			// this is done in the Launcher
//                _appInfo->setMediaType(MEDIA_TYPE_VIDEO);
        }



        // starting receiving thread

        // image->bits() will do deep copy (detach)
        receiverThread = new SagePixelReceiver(protocol, streamsocket, doubleBuffer, _appInfo, _perfMon, _affInfo,  settings);
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




		//
		// I shouldnt do this here
		// because Launcher will want to set pos when opening a session
		//
//        setPos(initrect.x(), initrect.y());

        return 0;
}
*/


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





