#ifndef SAGEPIXELRECEIVER_H
#define SAGEPIXELRECEIVER_H

#include <QThread>
//#include "common/imagedoublebuffer.h"

class ImageDoubleBuffer;
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
  * receives pixel from sail
  * its parent is SageWidget : QWidget
  */
class SagePixelReceiver : public QThread {
	Q_OBJECT

public:
	SagePixelReceiver(int protocol, int sockfd, ImageDoubleBuffer *idb,  AppInfo *ap, PerfMonitor *pm, AffinityInfo *ai, /*RailawareWidget *rw, QMutex *m, QWaitCondition *wwcc,*/ const QSettings *s, QObject *parent = 0);
//	SagePixelReceiver(int protocol, int sockfd, QImage *img,  AppInfo *ap, PerfMonitor *pm, AffinityInfo *ai, /*RailawareWidget *rw,*/ QMutex *m, QWaitCondition *wwcc, const QSettings *s, QObject *parent = 0);
	~SagePixelReceiver();

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
	int socket;

	QAbstractSocket *_socket;

	QImage *image;

	ImageDoubleBuffer *doubleBuffer;

	enum sageNwProtocol {SAGE_TCP, SAGE_UDP};

	int receiveUdpPortNumber();

	AppInfo *appInfo;
	PerfMonitor *perf;
	AffinityInfo *affInfo;

	quint64 frameCounter;

//	RailawareWidget *widget;

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

};

#endif // SAGEPIXELRECEIVER_H
