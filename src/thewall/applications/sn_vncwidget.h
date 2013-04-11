#ifndef VNCWIDGET_H
#define VNCWIDGET_H

#include "applications/base/sn_railawarewidget.h"
#include "applications/base/sn_basewidget.h"


#include <rfb/rfbproto.h>
#include <rfb/rfbclient.h>

class QGLWidget;

#include <QHostAddress>


#ifdef QT5
//#include <QtWidgets>
#undef max
#include <QtConcurrent>
#include <QOpenGLBuffer>
#else
#include <QtCore>
#include <QtOpenGL>
class QGLBuffer;
#endif

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

        inline QFutureWatcher<int> * initVNC_FutureWatcher() {return &_initVNC_futureWatcher;}


private:
        rfbClient *_vncclient;
        quint16 _serverPort;

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

#ifdef QT5
        QOpenGLBuffer *_pbobuf[2];
#else
        QGLBuffer *_pbobuf[2];
#endif

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

        QString _vncServerIpAddr;

        int _displayNumber;

        QFuture<int> _initVNC_future;
        QFutureWatcher<int> _initVNC_futureWatcher;
        int m_initVNC();


        /*!
          QFuture Handle for the recv thread
          */
        QFuture<void> _recvThread_future;


		bool initPboMutex();
		pthread_mutex_t *_pbomutex;
		pthread_cond_t *_pbobufferready;


        /*!
          This text item will be displayed during VNC initialization
          */
        QGraphicsSimpleTextItem *_initVNCtext;


		void initGL(bool usepbo);

        /*!
         * \brief set to true if initGL succeed
         */
        bool _isGLinitialized;


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

        QGLWidget *_textureUpdateWidget;

signals:
    void frameReceived();

public slots:
		/*!
		  The receivingThread() should be invoked after the constructor returns
		  to be able to get QGLContext
		  */
		void startImageRecvThread();


        /*!
          Reimplementing virtual function.
          convert QImage to QPixmap and schedule QGraphicsItem::update()
          */
        void scheduleUpdate();

	void upup();
	inline void _update() {update();}

        /*!
          Reimplementing virtual function.
          */
        void scheduleReceive() {}
};



#endif // VNCWIDGET_H
