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


/**
  * This QThread class is used to receive pixel stream from SAGE application (sail)
  */
class SN_SagePixelReceiver : public QThread
{
	Q_OBJECT

public:
	SN_SagePixelReceiver(int protocol, int sockfd, DoubleBuffer *idb, bool usepbo, void **pbobufarray, pthread_mutex_t *pboMutex, pthread_cond_t *pboCond, SN_SageStreamWidget *sagewidget, const QSettings *_settings, QObject *parent = 0);
	~SN_SagePixelReceiver();

protected:
    void run();

private:
	const QSettings *_settings;

    SN_SageStreamWidget *_sageWidget;

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
	int _udpsocket;

    int m_receiveUdpPortNumber();

    /*!
      based on the quality demanded by the scheduler
      */
    qint64 _delay;


    /*!
      If OpenGL PBO isn't used, traditional double buffering with app buffer
      */
	DoubleBuffer *_doubleBuffer;

	enum sageNwProtocol {SAGE_TCP, SAGE_UDP};


	AppInfo *_appInfo;
	PerfMonitor *_perfMon;
	AffinityInfo *_affInfo;

	bool _usePbo;

    /*!
      Front or Back of _pbobufarray
      */
	int _pboBufIdx;

    /*!
      This pointer array containing pointers to the buffer (in GPU memory)
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


    bool _isRMonitor;
    bool _isScheduler;

public:
    /*!
      This will close sockets
      */
	void endReceiver();

    inline void setEnd(bool b=true) {_end = b;}

signals:
	/*!
	  * This signal is emitted after a frame has received, and is connected to SageStreamWidget::schedule_update()
	  */
	void frameReceived();

public slots:
    inline void setDelay_msec(qint64 delay) {_delay = delay;}

    inline void resumeThreadLoop() {_end = false; start();}
};

#endif // SAGEPIXELRECEIVER_H
