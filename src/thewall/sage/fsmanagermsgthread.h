#ifndef FSMANAGERMSGTHREAD_H
#define FSMANAGERMSGTHREAD_H

#include <QThread>
#include <QRect>
//#include "sage/fsManager.h"
#include "sagecommondefinitions.h"

class QSettings;
class AffinityInfo;
class SageStreamWidget;

/**
  * represents control channel for an application
  */
class fsManagerMsgThread : public QThread {
	Q_OBJECT

public:
	fsManagerMsgThread(const quint64 sageAppId, int sockfd, const QSettings *settings, QObject *parent = 0);
	~fsManagerMsgThread();

	inline void setSageWidget(SageStreamWidget *s) {_sageWidget = s;}

	/**
	  * fsCore::checkClient()
	  */
	void run();

//	int getSocketFd() const { return socket; }

private:
	const QSettings *settings;

	SageStreamWidget *_sageWidget;

	int socket;
	bool _end;
	const quint64 sageAppId;
//	const fsManagerParam *fsmParam;

	/**
	  * fsCore::parseMessage()
	  * calls corresponding function according to the sageMsg.getCode()
	  */
	void parseMessage(OldSage::sageMessage &sageMsg);

signals:
	/**
	  * after the handshaking process between fsManager and sail,
	  * fsm sends SAIL_INIT_MSG (appID, socket window sizes)
	  * then emit this signal to create SageReceiver object that waits for sail connection (for pixel streaming)
	  *
	  * port means streaming port that SageReceiver has to open for pixel receiving
	  */
	void sailConnected(const quint64 sageAppId, QString appName, int protocol, int port, const QRect rect);

public slots:
	/**
	  * sets _end = true which breaks the thread loop
	  */
	void breakWhileLoop();
	void sendSailShutdownMsg();
	void sendSailShutdownMsg(quint64 sageappid);
	void sendSailSetRailMsg(AffinityInfo*,quint64);
	void sendSailMsg(OldSage::sageMessage &msg);
};

#endif // FSMANAGERMSGTHREAD_H
