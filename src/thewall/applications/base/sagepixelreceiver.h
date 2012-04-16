#ifndef SAGEPIXELRECEIVER_H
#define SAGEPIXELRECEIVER_H

#include <QThread>
//#include "common/imagedoublebuffer.h"

class DoubleBuffer;
class SN_RailawareWidget;
class SN_SageStreamWidget;
class QImage;
//class QPixmap;
//class QSemaphore;
//class QMutex;
class QAbstractSocket;
class PerfMonitor;
class AppInfo;
class AffinityInfo;
class QSettings;


#include <QMutex>
#include <QWaitCondition>


/**
  * This QThread class is used to receive pixel stream from SAGE application (sail)
  */
class SN_SagePixelReceiver : public QThread
{
	Q_OBJECT

public:
	SN_SagePixelReceiver(int protocol, int sockfd, DoubleBuffer *idb, bool usepbo, void **pbobufarray, pthread_mutex_t *pboMutex, pthread_cond_t *pboCond, AppInfo *ap, PerfMonitor *pm, AffinityInfo *ai, const QSettings *s, QObject *parent = 0);
//	SagePixelReceiver(int protocol, int sockfd, QImage *img,  AppInfo *ap, PerfMonitor *pm, AffinityInfo *ai, /*RailawareWidget *rw,*/ QMutex *m, QWaitCondition *wwcc, const QSettings *s, QObject *parent = 0);
	~SN_SagePixelReceiver();

	void run();

private:
	const QSettings *s;

	/*!
	  * this breaks while(1) loop in run()
	  * and can be set by different thread (SageStreamWidget)
	  */
	volatile bool _end;

	/*!
	  * streaming channel b/w sender (SAIL) and me.
	  * this is created at the SageStreamWidget
	  */
	int _tcpsocket;

	/*!
	  UDP streaming is not implemented yet
	  */
	QAbstractSocket *_udpsocket;

	/*!
	  texture handle for the image frame
	  */
//	GLuint _textureid;

//	GLuint *_pboIds;

	/*!
	  For the OpenGL context to which my _glbuffers will bind
	  */
//	QGLWidget *_myGlWidget;

	/*!
	  SageStreamWidget has to pass the pointer to the viewport widget (which is QGLWidget for the OpenGL Viewport)
	  _myGLWidget is sharing with _shareWidget
	  to share texture objects
	  */
//	QGLWidget *_shareWidget;

    /*!
      If OpenGL PBO isn't used, traditional double buffering with app buffer
      */
	DoubleBuffer *doubleBuffer;

	enum sageNwProtocol {SAGE_TCP, SAGE_UDP};

	int receiveUdpPortNumber();

	AppInfo *appInfo;
	PerfMonitor *perf;
	AffinityInfo *affInfo;

	bool _usePbo;

    /*!
      This flag is set in SageStreamWidget after the buffer point is successfuly mapped to GPU memory
      */
	bool __bufferMapped;

    /*!
      Front or Back of _pbobufarray
      */
	int _pboBufIdx;

    /*!
      This pointer array contains pointers to the buffer (in GPU memory)
      */
	void **_pbobufarray;

    /*!
      Mutex for _pboCond
      */
	pthread_mutex_t * _pboMutex;

    /*!
      Condition variable to wait on __bufferMapped
      */
	pthread_cond_t * _pboCond;




	QMutex _mutex;
	QWaitCondition _waitCond;

public:
	void receivePixel();
	void endReceiver();

signals:
	/*!
	  * This signal is emitted after a frame has received, and is connected to SageStreamWidget::schedule_update()
	  */
	void frameReceived();

public slots:
	/*!
	  This slot is invoked in SageStreamWidget to flip the pbo buffer index
	  */
	void flip(int idx);

};

#endif // SAGEPIXELRECEIVER_H
