#ifndef SAGESTREAMWIDGET_H
#define SAGESTREAMWIDGET_H

#include "railawarewidget.h"

#if defined(Q_OS_LINUX)
//#define GLEW_STATIC 1
#define GL_GLEXT_PROTOTYPES
//#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/glut.h>
//#include <GL/glext.h>
#elif defined(Q_OS_MAC)
#define GL_GLEXT_PROTOTYPES
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#include <fcntl.h>

#include "../../sage/fsmanagermsgthread.h"
#include "../../common/commonitem.h"

#include <QFutureWatcher>
#include <QMutex>
#include <QWaitCondition>

/**
  below is for when sage app streams YUV
  */
#define GLSLVertexShader   1
#define GLSLFragmentShader 2
int GLSLreadShaderSource(char *fileName, GLchar **vertexShader, GLchar **fragmentShader);
GLuint GLSLinstallShaders(const GLchar *Vertex, const GLchar *Fragment);


class DoubleBuffer;
class SN_SagePixelReceiver;
class AffinityInfo;
class QProcess;


class SN_SageStreamWidget : public SN_RailawareWidget
{
	Q_OBJECT
	Q_ENUMS(sagePixFmt)

public:
//	SageStreamWidget(const quint64 sageappid,QString appName, int protocol, int receiverPort, const QRect initRect, const quint64 globalAppId, const QSettings *s, ResourceMonitor *rm=0, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	SN_SageStreamWidget(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm = 0, QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
	~SN_SageStreamWidget();

	/*!
	  pixel format enum from SAGE
	  */
	enum sagePixFmt {PIXFMT_NULL, PIXFMT_555, PIXFMT_555_INV, PIXFMT_565, PIXFMT_565_INV,
		  PIXFMT_888, PIXFMT_888_INV, PIXFMT_8888, PIXFMT_8888_INV, PIXFMT_RLE, PIXFMT_LUV,
		  PIXFMT_DXT, PIXFMT_YUV};


	inline quint64 sageAppId() const {return _sageAppId;}

	inline void setSailAppProc(QProcess *p) {_sailAppProc = p;}
	inline QProcess * sailAppProc() {return _sailAppProc;}

	/*!
	  fsManagerMsgThread has to know if the receiver (which is this widget) is ready to receive the streamer message from SAIL.
	  The SageStreamWidget is ready to receive the message if it is blocking waiting for SAIL to connect ( ::accept())
	  */
	inline bool isWaitingSailToConnect() const {return _readyForStreamer;}

	/**
	  fsManager creates fsManagerMsgThread instance once a SAIL connected
	  Launcher attaches fsManagerMsgThread to sageWidget.

	  Once this is set, remaining handshaking should be done followed by pixel streaming
	  */
	inline void setFsmMsgThread(fsManagerMsgThread *thread) {_fsmMsgThread = thread;}


    /*!
      This will determine the delay in the pixel receiving thread
      */
    int setQuality(qreal newQuality);


    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	/*!
	  Override virtual function temporarily to display priority related info.
	  Please remove this !
	  */
	void updateInfoTextItem();



	/*!
	  The corresponding fsManager msg thread for this widget.
	  The object pointed by this is the TCP channel between the SAGE application (SAIL) and the receiver (SAGENext)
	  */
	fsManagerMsgThread *_fsmMsgThread;

	/**
	  The process of local SAGE application (SAIL) that sends pixels.
	  */
	QProcess *_sailAppProc;

	/*!
	  This is determined at the fsManager and given to me
	  */
	quint64 _sageAppId;

	/*!
	  The pointer to the QThread object that receives pixels from SAGE application.
	  */
	SN_SagePixelReceiver *_receiverThread;


	/**
	  In X11, QPixmap is stored at the X Server. So converting QImage to QPixmap involves converting plus copying data to Xserver, and this is expansive !!.
	  However, drawing QPixmap is much cheaper.
	  And we want to make the paint() as light as possible. So, use QPixmap for drawing if you're not using native OpenGL calls.
	  */
	QPixmap _pixmapForDrawing;

	/*!
	  Texture handle we're going to draw onto the screen
	  */
	GLuint _textureid;


	GLenum _pixelFormat;

	/*!
	  Currently this is set automatically if system supports PBO
	  However, when PIXFMT_YUV is detected then PBO will be disabled
	  */
	bool _usePbo;

	/*!
	  Two pboIds for double buffering
	  */
	GLuint _pboIds[2];

	int _pboBufIdx;

	/*!
	  Initializes mutex needed to synchronize the widget and the pixel receiver when PBO is used.
	  */
	bool _initPboMutex();

	/*!
	  The app side image double buffer.
	  This won't be created when _usePbo is true
	  */
	DoubleBuffer *doubleBuffer;


	/**
	  * pixel receiver socket on which this widget will listen in initialize()
	  */
	int serversocket;

	/**
	  * represent SAIL socket used to recv pixels by receiverThread
	  * initialize() enables this
	  */
	int streamsocket;


	int getPixelSize(sagePixFmt pixfmt);

	/**
	  * instantiate _image based on sagePixFmt
	  */
	int createImageBuffer(int resx, int resy, sagePixFmt pixelFormat);


	/*!
	  This is the future of waitForPixelStreamerConnection()
	  */
	QFuture<int> _streamerConnected;

	/*!
	  This monitors _initReceverFuture and starts startReceivingThread() upon _initReceiverFuture finishes.
	  */
	QFutureWatcher<int> _initReceiverWatcher;

	/*!
	  This info comes from SAIL.
	  Either TCP 0 or UDP 1
	  */
	int _streamProtocol;

	/*!
	  The widget is about to call ::accept() and will be ready to receive the streamer's message.
	  This is to let fsManagerMsgThread to know if i'm ready or not.
	  */
	volatile bool _readyForStreamer;


	/*!
	  double buffer for mapped pbo
	  */
	void * _pbobufarray[2];

//	QFuture<void> _recvThreadFuture;
//	QFutureWatcher<void> _recvThreadWatcher;
//	void __recvThread();
//	ssize_t __recvFrame(int sock, int bytecount, void *ptr);

	bool _isFirstFrame;


	pthread_mutex_t *_pbomutex;
	pthread_cond_t *_pbobufferready;

	/*!
	  If uyuv from mplayer then use shader to convert it to rgb
	  */
	bool _useShader;

	/*!
	  A shader program is used for the image with PIXFMT_YUV
	  The shader program is located at $SAGE_DIRECTORY/bin/yuv.vert/frag
	  */
	GLhandleARB _shaderProgHandle;



    /*!
      BLAME_XINERAMA
      */
    bool _blameXinerama;


    void m_initOpenGL();

signals:
    /*!
      This singal is emiited after the sage streamer (SAGE application) connected to this widget.
      Thus, the streaming channel is established and the frame size, rate, appname, etc are known.
      */
    void streamerInitialized();

public slots:
	/**
	  This slot runs waitForPixelStreamerConnection() in separate thread because ::accept() will block in that slot.
	  fsManagerMsgThread invokes this slot. QFuture _streamerConnected is the future of this slot.
	  */
	void doInitReceiver(quint64 sageappid, const QString &appname, const QRect &initrect, int protocol, int port);

	/**
	  This slot does blocking waiting streamer's connection.
	  Upon returning, _streamerConnected future will be notified.
	  Image double buffer is created upon connection if PBO is disabled.
	  */
	int waitForPixelStreamerConnection(int protocol, int port, const QString &appname);

	/**
	  This slot starts pixel receiving thread and is called after waitForPixelStreamerConnection() finished
	  */
	void startReceivingThread();


	/**
	  This slot is invoked in fsManagerMsgThread::parseMessage().
	  It creates image buffer and pixel streaming socket is created in this function.

	  receiverThread is created and started in here
	  */
//	int initialize(quint64 sageappid, QString appname, QRect initrect, int protocol, int port);

	/*!
	  This slot is called (Qt::QueuedConnection) by pixel receiving thread whenever the thread received a frame.
      In this slot, the back buffer is released (consumes data) after QImage is converted to QPixmap. (This delay is conversion latency)
	  It calls then QGraphicsWidget::update()
	  */
	void scheduleUpdate();


	/*!
	  This slot is invoked by __recvThread() after DMA writing is done.
	  This slot do

	  unmap previous buffer
	  map new buffer
	  signal thread for next frame
	  texture update with previous buffer
	  */
	void schedulePboUpdate();


    /*!
      no opengl no pbo no paint
      */
    void scheduleDummyUpdate();

};


#endif // SAGESTREAMWIDGET_H
