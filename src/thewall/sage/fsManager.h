#ifndef FSMANAGER_H
#define FSMANAGER_H

#include <QTcpServer>
#include <QRect>
#include "sagecommondefinitions.h"

class fsManagerMsgThread;
class QSettings;
class AffinityInfo;
class SN_Launcher;

/*
typedef struct {
	int sockfd;
	fsManagerMsgThread *thread_ptr;
} ThreadParam;
*/
/*
typedef struct {
//	char fsIP[INET_ADDRSTRLEN];
	int fsPort; // 20002
	int streamPortBase; // 20005
} fsManagerParam;
*/

/**
  * accept connections (nonblock) followed by read messages(nonblock)
  * from sage app
  */
class fsManager : public QTcpServer {
	Q_OBJECT

public:
	fsManager(const QSettings *_settings, SN_Launcher *l, QObject *parent = 0);
	~fsManager();
	//void registerApp(quint64 appid, const QString &msgStr, fsManagerMsgThread const * thread);
//	fsManagerParam* getFsManagerParam() { return &fsmParam; }

	quint64 sageAppId() const {return _sageAppId;}

protected:
	/**
	  * overrides QTcpServer::incomingConnection()
	  * The base implementation emits newConnection() signal
	  *
	  * creates a fsManagerMsgThread for each connection
	  * increment appID
	  * insert this connection to clientMap
	  */
	void incomingConnection(int sockfd);

private:
	const QSettings *_settings;

	SN_Launcher *_launcher;

	/**
	  * reentrant (A reentrant function can be called simultaneously from multiple threads, but only if each invocation uses its own data.)
	  * doesn't necessarily mean it's thread-safe
	  */
	//QVector<int> clientVector;
	//fsManagerMsgThread *msgThread;
//	QMap<quint64, fsManagerMsgThread *> clientMap;
//	QList<fsManagerMsgThread *> msgThreadList;
	quint64 _sageAppId;
//	fsManagerParam fsmParam;
	QHostAddress _fsIP;

    /*!
      creates a fsmMessageThread
      */
    fsManagerMsgThread * _createMsgThread(int sockfd);

signals:
	void sailConnected(const quint64 _sageAppId, QString appName, int protocol, int port, const QRect initRect);
	void sailDisconnected(quint64);
	void shutdownSail(quint64 sageappid);
	void sailSendSetRailMsg(AffinityInfo *, quint64);
	void incomingSail(fsManagerMsgThread *);

public slots:
	/**
	  * receives QThread::finished() signal
	  * erase an element from clientMap
	  * then emits sailDisconnected()
	  */
//	void threadFinished();

	/**
	  * receives sailConnected() from thread
	  * then emits this->sailConnected()
	  */
//	void forwardSailConnection(const quint64, QString appName, int protocol, int port, const QRect initRect);

//	void pixelReceiverFinished(quint64);
//	void shutdownSail(quint64 sageid, quint64 globalid);
};

#endif // FSMANAGER_H
