#include "fsManager.h"
#include "fsmanagermsgthread.h"
#include "sn_sagenextlauncher.h"

#include <QSettings>

fsManager::fsManager(const QSettings *settings, SN_Launcher *l, QObject *parent) :

	QTcpServer(parent),

	_settings(settings),
	_launcher(l),
	_sageAppId(0)
{
//	clientMap.clear();
//	msgThreadList.clear();

	// read fsManager.conf and set values
	_fsIP.setAddress( settings->value("general/fsmip", "127.0.0.1").toString() );
//	fsmParam.fsPort = settings->value("general/fsmport", 20002).toInt();

	// this + appID is used for streaming port number
	// udp streaming adds 1 to it
//	fsmParam.streamPortBase = fsmParam.fsPort + 3;

	if ( ! isListening() ) {
		if ( ! listen(/*_fsIP*/ QHostAddress::Any, settings->value("general/fsmport", 20002).toInt()) ) {
			qCritical("fsManager::%s() : listen failed", __FUNCTION__);
		}
	}
	else {
		qDebug("fsManager::%s() : fsm is listening on %s:%d already", __FUNCTION__, qPrintable(serverAddress().toString()), serverPort());
	}
}

fsManager::~fsManager() {
	close();

	/* iterate clientMap and close all connections */
	/*
	QMap<quint64, ThreadParam>::const_iterator it;
	for ( it=clientMap.begin(); it!= clientMap.end(); ++it) {
		fsManagerMsgThread *thread = it.value().thread_ptr;
		if ( thread && !thread->isFinished()) {
			qDebug("fsManager::%s() : terminate thread appID %llu, socket %d", __FUNCTION__, it.key(), it.value().sockfd);
			thread->stop();
			//thread->terminate();
			thread->wait();
		}
		//qDebug("%s() : %d, %d", __FUNCTION__, it.key(), it.value());
	}
*/
	/*
	fsManagerMsgThread *thr = 0;
	QList<fsManagerMsgThread *>::iterator it = msgThreadList.begin();
	for (; it != msgThreadList.end(); ++it ) {
		thr = (*it);
		thr->stop();
		thr->wait(); // deleteLater will be called once finished()
	}
	*/
	qDebug("fsManager::%s()", __FUNCTION__);
}

/**
  * msgStr argument is not deep-copied.
  * The string will be invalid when fsmanagerMsgThread::parseMessage() returns
  */
/*
void fsManager::registerApp(quint64 appid, const QString &msgStr, fsManagerMsgThread const * thread) {
	qDebug("fsManager::%s() : ", __FUNCTION__);

	QMap<quint64, ThreadParam>::const_iterator it = clientMap.find(appid);
	if ( it == clientMap.end() ) {
		qCritical("fsManager::%s() : sail(appID %llu) connected to fsManager but there's no ThreadParam !!!", __FUNCTION__, appid);
		return;
	}

	// send SAIL_INIT_MSG. will trigger
	//sageNwConfig nwCfg;
	// reading network parameters set in fsManager.conf
	//sscanf(msgData, "%d %d %d %d", &winID, &nwCfg.rcvBufSize, &nwCfg.sendBufSize, &nwCfg.mtuSize);
	//fprintf(stderr,"sail::%s() : SAIL_INIT_MSG(code %d) : received [%s]\n", __FUNCTION__, msg.getCode(), msgData);

	//if (config.rendering) {
	//pixelStreamer->setWinID(winID);
	//pixelStreamer->setNwConfig(nwCfg); // block partition and block group objects are created here

	int sock = it.value().sockfd;
	//int sock = thread->getSocketFd();
	SAGE::sageMessage sageMsg;
	QByteArray initMsg(32, '\0');
	sprintf(initMsg.data(), "%d %d %d %d", appid, 16777216, 1048576, 1450); // snd/rcv network buffer size followed by MTU
	if (sageMsg.init(appid, SAIL_INIT_MSG, 0, initMsg.size(), initMsg.data()) < 0) {
		qCritical("fsManager::%s() : failed to init sageMessage", __FUNCTION__);
		return;
	}

	int dataSize = sageMsg.getBufSize();
	int sent = send(sock, (char *)sageMsg.getBuffer(), dataSize, 0);
	if ( sent == -1 ) {
		qCritical("fsManager::%s() : error while sending SAIL_INIT_MSG", __FUNCTION__);
	}
	if ( sent == 0 ) {
		// remote side disconnected
		qCritical("fsManager::%s() : while sending SAIL_INIT_MSG, sail disconnected.", __FUNCTION__);
		fsManagerMsgThread *thread = it.value().thread_ptr;
		thread->stop();
	}
	sageMsg.destroy();


	// send SAIL_CONNECT_TO_RCV
	//pixelStreamer->initNetworks(msgData)
	// sail will send back SAIL_CONNECTED_TO_RCV
	// [22000 1 131.193.78.140 0 ]
}
*/

/**
  * This overrides default implementation and will no longer emits newConnection() signal.
  * This is a message thread for each application
  */
#ifdef QT5
void fsManager::incomingConnection(qintptr sockfd) {
#else
void fsManager::incomingConnection(int sockfd) {
#endif

    fsManagerMsgThread *thread = _createMsgThread(sockfd);

    if (!thread) {
        qDebug() << "fsManager::incomingConnection() : failed to create fsmMsgThread";
    }
    else {
        thread->start();
    }
}

fsManagerMsgThread * fsManager::_createMsgThread(int sockfd) {
    ++_sageAppId;

//	qDebug("fsManager::%s() : Incoming Connection, sockfd %d, sageAppId %llu. Starting msgThread", __FUNCTION__, sockfd, _sageAppId);

	fsManagerMsgThread *thread = new fsManagerMsgThread(_sageAppId, sockfd, _settings);

    if ( ! QObject::connect(thread, SIGNAL(sageAppConnectedToFSM(QString,QString,fsManagerMsgThread*)), this, SIGNAL(sageAppConnectedToFSM(QString,QString,fsManagerMsgThread*))) ) {
        qDebug() << "fsManager::_createMsgThread() : failed to connect sageAppConnectedToFSM signals";
    }

//	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(this, SIGNAL(destroyed()), thread, SLOT(sendSailShutdownMsg()));

	//connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
//	connect(thread, SIGNAL(sailConnected(const quint64,QString, int, int, const QRect)), this, SIGNAL(sailConnected(quint64,QString,int,int,QRect)));

	connect(this, SIGNAL(shutdownSail(quint64)), thread, SLOT(sendSailShutdownMsg(quint64)));
	connect(this, SIGNAL(sailSendSetRailMsg(SN_AffinityInfo*,quint64)), thread, SLOT(sendSailSetRailMsg(SN_AffinityInfo*,quint64)));

    return thread;
}

/**
  * private slot connected to fsManagerMsgThread's finished() signal
  */
/*
void fsManager::threadFinished() {
	QMap<quint64, ThreadParam>::iterator it;
	for ( it=clientMap.begin(); it!= clientMap.end(); ++it) {
		fsManagerMsgThread *thread = it.value().thread_ptr;
		if ( thread && thread->isFinished() ) {

			quint64 id = it.key();
			int sock = it.value().sockfd;

			clientMap.erase(it);
			--it;

			qDebug("fsManager::%s() : thread for appID %llu, socket %d has finished. map size %d. EMITTING sailDisconnected()", __FUNCTION__, id, sock, clientMap.size());
			emit sailDisconnected(id); // direct connection. MainWindow::stopSageApp() will be called right here.

			delete thread;
		}
	}
}
*/

/**
  * private slot connected to fsManagerMsgThread::sailConnected() signal
  */
//void fsManager::forwardSailConnection(const quint64 sageappid, QString appName, int protocol, int streamPort, const QRect initRect) {
//	qDebug("fsManager::%s() : EMITTING sailConnected(%llu, %d)", __FUNCTION__, sageappid, socket);
//	emit sailConnected(sageappid, appName, protocol, streamPort, initRect);
//}


//void fsManager::pixelReceiverFinished(quint64 appid) {
	/**
	  should send SHUTDOWN_APP message to sail
	  **/
//	qDebug("fsManager::%s() : appid %llu", __FUNCTION__, appid);
//	QMap<quint64, ThreadParam>::iterator it;

//	it = clientMap.find(appid);
//	if ( it == clientMap.end() ) return;

//	qDebug("fsManager::%s() : stopping msgThread", __FUNCTION__);
//	it.value().thread_ptr->terminate();
//}




