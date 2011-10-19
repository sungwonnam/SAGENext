#ifndef SAGESTREAMWIDGET_H
#define SAGESTREAMWIDGET_H

#include "base/railawarewidget.h"
//#include "common/imagedoublebuffer.h"

#if defined(Q_OS_LINUX)
//#include <GL/gl.h>
//#include <GL/glu.h>
#endif

class fsManagerMsgThread;
class RawDoubleBuffer;
class SN_SagePixelReceiver;
class AffinityInfo;
class QProcess;

#include <QFutureWatcher>
//#include <QWaitCondition>

class SN_SageStreamWidget : public SN_RailawareWidget
{
	Q_OBJECT

public:
//	SageStreamWidget(const quint64 sageappid,QString appName, int protocol, int receiverPort, const QRect initRect, const quint64 globalAppId, const QSettings *s, ResourceMonitor *rm=0, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	SN_SageStreamWidget(QString filename, const quint64 globalappid, const QSettings *s, QString senderIP = "127.0.0.1", SN_ResourceMonitor *rm = 0, QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);

	~SN_SageStreamWidget();

//	quint16 getRatioToTheWall() const;

//	inline int getImageSize() const { return imageSize; }
//	inline AppInfo * getAppInfoPtr() { return appInfo; }
//	inline PerfMonitor * getPerfMonPtr() { return perfMon; }

	inline quint64 sageAppId() const {return _sageAppId;}

	inline void setSailAppProc(QProcess *p) {_sailAppProc = p;}
	inline QProcess * sailAppProc() {return _sailAppProc;}

	inline bool isWaitingSailToConnect() const {return _readyForStreamer;}

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
//	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
//	GLuint texhandle;
	const QSettings *_settings;

	fsManagerMsgThread *_fsmMsgThread;

	/**
	  The local application that sends pixel
	  */
	QProcess *_sailAppProc;

	quint64 _sageAppId;

	/*!
	  Pointer to QThread object
	  */
	SN_SagePixelReceiver *_receiverThread;


	/**
	  Drawing image is expansive than pixmap. But conversion to pixmap isn't needed
	  */
	QImage _imageForDrawing;

//	QPixmap _pixmapForDrawing;


//	QSemaphore *convertedToPixmap;

	RawDoubleBuffer *doubleBuffer;


	/**
	  * QPixmap is for drawing.

	  In X11, QPixmap is stored at the X Server. So converting image to pixmap involves converting plus copy to server.
	  However, drawing pixmap is cheaper.

	  */
//	QPixmap _pixmap;

	/**
	  * pixel receiver socket on which this widget will listen in initialize()
	  */
	int serversocket;

	/**
	  * represent SAIL socket used to recv pixels by receiverThread
	  * initialize() enables this
	  */
	int streamsocket;

	/**
	  * byte count of a frame
	  */
//	int imageSize;

	quint64 frameCounter;

	int _bordersize;

	enum sagePixFmt {PIXFMT_NULL, PIXFMT_555, PIXFMT_555_INV, PIXFMT_565, PIXFMT_565_INV,
		  PIXFMT_888, PIXFMT_888_INV, PIXFMT_8888, PIXFMT_8888_INV, PIXFMT_RLE, PIXFMT_LUV,
		  PIXFMT_DXT, PIXFMT_YUV};


	int getPixelSize(sagePixFmt pixfmt);

	/**
	  * instantiate _image based on sagePixFmt
	  */
	int createImageBuffer(int resx, int resy, sagePixFmt pixelFormat);


	QFuture<int> _initReceiverFuture;
	QFutureWatcher<int> _initReceiverWatcher;

	int _streamProtocol;

	volatile bool _readyForStreamer;


//	QMutex *mutex;
//	QWaitCondition *wc;

signals:
//	void pixelReceiverFinished();

	/*!
	  * This signal is emited in the destructor and connected to fsManager::shutdownSail() by GraphicsViewMain::startSageApp()
	  * It is to send APP_QUIT to sail
	  */
//	void destructor(quint64 sageappid);

	/*!
	  To pause thread
	  */
	void pauseThread();

	void resumeThread();


public slots:
	/**
	  fsManager creates fsManagerMsgThread instance once a SAIL connected
	  Launcher attaches fsManagerMsgThread to sageWidget.

	  Once this is set, remaining handshaking should be done followed by pixel streaming
	  */
	void setFsmMsgThread(fsManagerMsgThread *);

	/**
	  This slot is invoked in fsManagerMsgThread::parseMessage().
	  It creates image buffer and pixel streaming socket is created in this function.

	  receiverThread is created and started in here
	  */
//	int initialize(quint64 sageappid, QString appname, QRect initrect, int protocol, int port);


	/**
	  This slot runs waitForPixelStreamerConnection in separate thread because ::accept() will block in that slot
	  */
	void doInitReceiver(quint64 sageappid, const QString &appname, const QRect &initrect, int protocol, int port);

	/**
	  This slots blocking waits streamer's connection.
	  Image double buffer is created upon connection
	  */
	int waitForPixelStreamerConnection(int protocol, int port, const QString &appname);

	/**
	  This slot starts pixel receiving thread and is called after waitForPixelStreamerConnection() finished
	  */
	void startReceivingThread();

	/*!
	  This slot is called (Qt::QueuedConnection) by pixel receiving thread whenever the thread received a frame.
      In this slot, the back buffer is released (consumes data) after QImage is converted to QPixmap. (This delay is conversion latency)
	  It calls then QGraphicsWidget::update()
	  */
	void scheduleUpdate();

	/*!
	  Wake waitCondition
	  */
	void scheduleReceive();
};

#endif // SAGESTREAMWIDGET_H
