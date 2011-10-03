#include "uimsgthread.h"
#include "../common/commondefinitions.h"
#include "uiserver.h"


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
//#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>



UiMsgThread::UiMsgThread(const quint32 id, int sockfd, QObject *parent) :
	QThread(parent), uiClientId(id), _end(false), sockfd(sockfd)
{
	int optval = 1;
	//socket.setSocketOption(QAbstractSocket::LowDelayOption, (const QVariant *)&optval);
	if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_REUSEADDR failed", __FUNCTION__);
	}
	if ( setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_KEEPALIVE failed", __FUNCTION__);
	}
	if ( setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt TCP_NODELAY failed", __FUNCTION__);
	}
//	tcpSock = new QTcpSocket(this);
//	tcpSock->setSocketDescriptor(sockfd); // connected, readwrite


//	sharedPointer = 0;
}

UiMsgThread::~UiMsgThread() {
	_end = true;
	emit clientDisconnected(uiClientId);

	QByteArray msg(EXTUI_MSG_SIZE, 0);
	sprintf(msg.data(), "%d %u goodbye", WALL_IS_CLOSING, uiClientId);

	if (  ::shutdown(sockfd, SHUT_RDWR) != 0 ) {
		qDebug("UiMsgThread::%s() : shutdown error", __FUNCTION__);
	}
	if ( ::close(sockfd) != 0 ) {
		qDebug("UiMsgThread::%s() : close error", __FUNCTION__);
	}
	wait();

	qDebug("UiMsgThread::%s() : ui client %u exiting", __FUNCTION__, uiClientId);
}

void UiMsgThread::sendMsg(const QByteArray &msgstr) {
	if ( ::send(sockfd, msgstr.data(), msgstr.size(), 0) <= 0 ) {
		qCritical("UiMsgThread::%s() : send error", __FUNCTION__);
	}
//	qDebug("UiMsgThread::%s() : msg [%s] sent", __FUNCTION__, msgstr.constData());
}

void UiMsgThread::run() {
	qDebug("UiMsgThread::%s() : ui client thread %u has started", __FUNCTION__, uiClientId);

	QByteArray msgStr(EXTUI_MSG_SIZE, '\0');
	int read = 0;

	while(!_end) {
		msgStr.fill(0);

		if ( (read = recv(sockfd, (void *)msgStr.data(), EXTUI_MSG_SIZE, MSG_WAITALL)) == -1 ) {
			qCritical("UiMsgThread::%s() : socket error", __FUNCTION__);
			break;
		}
		else if ( read == 0 ) {
			qDebug("UiMsgThread::%s() : socket disconnected", __FUNCTION__);
			break;
		}
//		qDebug("UiMsgThread::%s() : received external UI msg [%s]", __FUNCTION__, msgStr.data());

		/**
		  multiple threads will emit this signal in parallel.
		  And by default, this is queued connection. So enqueueing is automatically thread-safe
		  **/
		emit msgReceived(uiClientId, this, msgStr);

		//			QByteArray data(EXTUI_MSG_SIZE, '\0');
		//			sprintf(data.data(), "%d %s %d", ACK_FROM_WALL, "131.193.78.140", 30303);
		//			int t = ::send(sockfd, data.data(), data.size(), 0);
		//			qDebug("UiMsgThread::%s() : %d byte ACK_FROM_WALL [%s] sent", __FUNCTION__, t, data.constData());
	}


//	qDebug("UiMsgThread::%s() : thread finished", __FUNCTION__);
}


void UiMsgThread::breakWhileLoop() {
	_end = true;
}
