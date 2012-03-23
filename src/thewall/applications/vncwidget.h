#ifndef VNCWIDGET_H
#define VNCWIDGET_H

#include "base/railawarewidget.h"
#include "base/basewidget.h"



#include <QHostAddress>
#include <QtCore>


#if defined(Q_OS_LINUX)
//#define GLEW_STATIC 1
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#elif defined(Q_OS_MAC)
#define GL_GLEXT_PROTOTYPES
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#include <rfb/rfbproto.h>
#include <rfb/rfbclient.h>


/*!
  This class uses LibVNCServer library to receive
  image from a vncserver.
  */
class SN_VNCClientWidget : public SN_RailawareWidget
{
	Q_OBJECT
public:
        explicit SN_VNCClientWidget(quint64 globalappid, const QString senderIP, int display, const QString username, const QString passwd, int _framerate, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);
        ~SN_VNCClientWidget();

//		void handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

//		void mousePressEvent(QGraphicsSceneMouseEvent *event);
//		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);


private:
        rfbClient *vncclient;
        QHostAddress serverAddr;
        quint16 serverPort;

        /*!
          pixel buffer
          */
        QImage *_image;

		/**
		  texture id I'm going to draw
		  */
		GLuint _texid;

		/*!
		  We're going to do doublebuffering with pbo
		  */
		GLuint _pboIds[2];

		/*!
		  true if the OpenGL system has PBO extension
		  */
		bool _usePbo;

		/*!
		  thread will wait on this condition
		  */
		bool __bufferMapped;

		unsigned char *_mappedBufferPtr;

		bool __firstFrame;

		/*!
		  The current pbo buffer array (_pboIds[]) index
		  we're going to WRITE
		  */
		int _pboBufIdx;


        /*!
          paint() will draw this if OpenGL isn't enabled
          */
        QPixmap _pixmapForDrawing;

        /*!
          copy vnc framebuffer to QImage
          */
        void receivingThread();

        bool _end;

        int _framerate;

		/**
		  to send mouse click event to VNC server
		  */
		int _buttonMask;

        QFuture<void> future;


		bool initPboMutex();
		pthread_mutex_t *_pbomutex;
		pthread_cond_t *_pbobufferready;


		void initGL(bool usepbo);

		static rfbCredential * getCredential(struct _rfbClient *client, int credentialType);

        static rfbBool got_data;
		static QString username;
        static QString vncpasswd;

        static void signal_handler(int signal);

        static rfbBool resize_func(rfbClient* client);

        static void frame_func(rfbClient* client);

        static rfbBool position_func(rfbClient* client, int x, int y);

        static char *password_func(rfbClient* client);

        static void update_func(rfbClient* client,int x,int y,int w,int h);

public slots:
		/*!
		  The receivingThread() should be invoked after the constructor returns
		  to be able to get QGLContext
		  */
		void startThread();


        /*!
          Reimplementing virtual function.
          convert QImage to QPixmap and schedule QGraphicsItem::update()
          */
        void scheduleUpdate();

        /*!
          Reimplementing virtual function.
          */
        void scheduleReceive() {}
};



/***
class VNCClientThread : public QThread {
	Q_OBJECT
public:
	VNCClientThread(rfbClient *client, QObject *parent=0);
	~VNCClientThread();

	inline void setPerfMon(PerfMonitor *pm) {_perfMon = pm;}

	inline void setRgbBuffer(unsigned char *b) {_rgbBuffer = b;}

private:
	rfbClient *_vncclient;

	PerfMonitor *_perfMon;

	unsigned char *_rgbBuffer;

protected:
	void run();

signals:
	void frameReady();

	void vncError();

public slots:
	void terminateThread();

	void receiveFrame();
};
**/

#endif // VNCWIDGET_H
