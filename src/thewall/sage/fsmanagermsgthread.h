#ifndef FSMANAGERMSGTHREAD_H
#define FSMANAGERMSGTHREAD_H

#include <QtCore>
//#include "sage/fsManager.h"
#include "sagecommondefinitions.h"

class QSettings;
class AffinityInfo;
class SN_SageStreamWidget;

/**
  * represents control channel for an application
  */
class fsManagerMsgThread : public QThread {
	Q_OBJECT

public:
	fsManagerMsgThread(const quint64 _sageAppId, int sockfd, const QSettings *_settings, QObject *parent = 0);
	~fsManagerMsgThread();

	inline void setSageWidget(SN_SageStreamWidget *s) {_sageWidget = s;}

	inline quint64 sageAppId() const {return _sageAppId;}

	inline QString sageAppName() const {return _sageAppName;}

	inline void setGlobalAppId(quint64 v) {_globalAppId = v;}
	quint64 globalAppId() const {return _globalAppId;}

	/**
	  * fsCore::checkClient()
	  */
	void run();

//	int getSocketFd() const { return socket; }

private:
	const QSettings *_settings;

	SN_SageStreamWidget *_sageWidget;

	int socket;
	bool _end;

	QString _sageAppName;

	/*!
	  This is determined at the fsManager and given to me
	  */
	const quint64 _sageAppId;

	/*!
	  global add id of the basewidget
	  */
	quint64 _globalAppId;

	/*!
	  * fsCore::parseMessage()
	  * calls corresponding function according to the sageMsg.getCode()
	  */
	void parseMessage(OldSage::sageMessage &sageMsg);

    /*!
      fsmThread should wait until _sageWidget is not null.
      The SN_SageStreamWidget will be created by the SN_Launcher.
      */
    QWaitCondition _isSageWidgetCreated;
    QMutex _mutex;

signals:
	/*!
	  * after the handshaking process between fsManager and sail,
	  * fsm sends SAIL_INIT_MSG (appID, socket window sizes)
	  * then emit this signal to create SageReceiver object that waits for sail connection (for pixel streaming)
	  *
	  * port means streaming port that SageReceiver has to open for pixel receiving
	  */
//	void sailConnected(const quint64 sageAppId, QString appName, int protocol, int port, const QRect rect);

public slots:
    /*!
      To signal _isSageWidgetCreated condition
      */
    void signalSageWidgetCreated();

	/*!
	  * sets _end = true which breaks the thread loop
	  */
	void breakWhileLoop();
	void sendSailShutdownMsg();
	void sendSailShutdownMsg(quint64 sageappid);
	void sendSailSetRailMsg(AffinityInfo*,quint64);
	void sendSailMsg(OldSage::sageMessage &msg);
	void sendSailMsg(int msgcode, const QString &msgdata);
};

#endif // FSMANAGERMSGTHREAD_H
