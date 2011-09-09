#ifndef SAGESTREAMWIDGET_H
#define SAGESTREAMWIDGET_H

#include "base/railawarewidget.h"
//#include "common/imagedoublebuffer.h"

#if defined(Q_OS_LINUX)
//#include <GL/gl.h>
//#include <GL/glu.h>
#endif

class fsManagerMsgThread;
class ImageDoubleBuffer;
class SagePixelReceiver;
class AffinityInfo;

//#include <QFutureWatcher>
//#include <QWaitCondition>

class SageStreamWidget : public RailawareWidget
{
	Q_OBJECT

public:
//	SageStreamWidget(const quint64 sageappid,QString appName, int protocol, int receiverPort, const QRect initRect, const quint64 globalAppId, const QSettings *s, ResourceMonitor *rm=0, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	SageStreamWidget(QString filename, const quint64 globalappid, const QSettings *s, QString senderIP = "127.0.0.1", ResourceMonitor *rm = 0, QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);

	~SageStreamWidget();

//	quint16 getRatioToTheWall() const;

//	inline int getImageSize() const { return imageSize; }
//	inline AppInfo * getAppInfoPtr() { return appInfo; }
//	inline PerfMonitor * getPerfMonPtr() { return perfMon; }

	inline quint64 sageAppId() const {return _sageAppId;}


protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
//	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
//	GLuint texhandle;

	fsManagerMsgThread *_fsmMsgThread;

	quint64 _sageAppId;

	/*!
	  Pointer to QThread object
	  */
	SagePixelReceiver *receiverThread;


	/**
	  * single pixel storage
	  * QImage is for I/O. So this is used for receiving pixels from sAIL
	  */
	QImage *image;
//	QImage image2;

//	QSemaphore *convertedToPixmap;

	ImageDoubleBuffer *doubleBuffer;


	/**
	  * QPixmap is for drawing.
	  */
	QPixmap _pixmap;

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
	int imageSize;

	quint64 frameCounter;

	enum sagePixFmt {PIXFMT_NULL, PIXFMT_555, PIXFMT_555_INV, PIXFMT_565, PIXFMT_565_INV,
		  PIXFMT_888, PIXFMT_888_INV, PIXFMT_8888, PIXFMT_8888_INV, PIXFMT_RLE, PIXFMT_LUV,
		  PIXFMT_DXT, PIXFMT_YUV};


	int getPixelSize(sagePixFmt pixfmt);

	/**
	  * instantiate _image based on sagePixFmt
	  */
	int createImageBuffer(int resx, int resy, sagePixFmt pixelFormat);


//	QMutex *mutex;
//	QWaitCondition *wc;

signals:
//	void pixelReceiverFinished();

	/*!
	  * This signal is emited in the destructor and connected to fsManager::shutdownSail() by GraphicsViewMain::startSageApp()
	  * It is to send APP_QUIT to sail
	  */
	void destructor(quint64 sageappid);

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
	int initialize(quint64 sageappid, QString appname, QRect initrect, int protocol, int port);

	/*!
	  releases back buffer (consumes data) so that producer's swap buffer can return.
	  It calls QGraphicsWidget::update()
	  */
	void scheduleUpdate();

	/*!
	  Wake waitCondition
	  */
	void scheduleReceive();
};

#endif // SAGESTREAMWIDGET_H
