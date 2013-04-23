#include "uiserver/sn_uimsgthread.h"
#include "uiserver/sn_uiserver.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

SN_UiMsgThread::SN_UiMsgThread(const quint32 id, int sock, QObject *parent) :
	QThread(parent)
  , _uiClientId(id)
  , _end(false)
  , _sockfd(sock)
//  , _udpSock(0)
{
	int optval = 1;
	if ( setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_REUSEADDR failed", __FUNCTION__);
	}
	if ( setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_KEEPALIVE failed", __FUNCTION__);
	}
	if ( setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt TCP_NODELAY failed", __FUNCTION__);
	}

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if ( ::getpeername(_sockfd, (struct sockaddr *)&addr, &len) != 0 ) {
		qCritical() << "UiMsgThread failed to get peer address";
	}
	else {
        qDebug() << QHostAddress(addr.sin_addr.s_addr);
        _peerAddress.setAddress(QHostAddress((struct sockaddr *)&addr).toIPv4Address());
//		char str[INET_ADDRSTRLEN];
        QByteArray str(INET_ADDRSTRLEN, 0);
		if ( ::inet_ntop(AF_INET, (void *)&(addr.sin_addr), str.data(), INET_ADDRSTRLEN) ) {
//			_peerAddress = QHostAddress(QString(str));
		}
		else {
			perror("inet_ntop");
		}
	}
    qDebug() << _peerAddress.toString();

    /*
    _tcpSock.setSocketDescriptor(_sockfd);
    _tcpSock.setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _tcpSock.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _peerAddress = _tcpSock.peerAddress();
    qDebug() << __FUNCTION__ << _peerAddress;
    */
}

SN_UiMsgThread::~SN_UiMsgThread() {
	_end = true;
	emit clientDisconnected(_uiClientId);

	QByteArray msg(EXTUI_MSG_SIZE, 0);
	sprintf(msg.data(), "%d %u goodbye",SAGENext::WALL_IS_CLOSING, _uiClientId);

	if (  ::shutdown(_sockfd, SHUT_RDWR) != 0 ) {
//		qDebug("UiMsgThread::%s() : shutdown error", __FUNCTION__);
	}
	if ( ::close(_sockfd) != 0 ) {
//		qDebug("UiMsgThread::%s() : close error", __FUNCTION__);
	}
	wait();

	qDebug("UiMsgThread::%s() : ui client %u exiting", __FUNCTION__, _uiClientId);
}

void SN_UiMsgThread::sendMsg(const QByteArray &msgstr) {
	if (msgstr.size() != EXTUI_MSG_SIZE) {
		qWarning("%s::%s() : You're about to send a message with its size %d", metaObject()->className(), __FUNCTION__, msgstr.size());
	}

	ssize_t sent = ::send(_sockfd, msgstr.data(), msgstr.size(), 0);
//    qint64 sent = _tcpSock.write(msgstr.data(), EXTUI_MSG_SIZE);
	if ( sent <= 0 ) {
		qCritical("UiMsgThread::%s() : send error", __FUNCTION__);
	}
	else {
		//	qDebug("UiMsgThread::%s() : msg [%s] sent", __FUNCTION__, msgstr.constData());
	}
}

void SN_UiMsgThread::run() {
//	qDebug("UiMsgThread::%s() : ui client thread %u has started", __FUNCTION__, _uiClientId);

	QByteArray msgStr(EXTUI_SMALL_MSG_SIZE, 0);
	int read = 0;

	while(!_end) {
		msgStr.fill(0);
        read = recv(_sockfd, (void *)msgStr.data(), EXTUI_SMALL_MSG_SIZE, MSG_WAITALL);

        /*
        while (_tcpSock.bytesAvailable() < EXTUI_SMALL_MSG_SIZE) {
            _tcpSock.waitForReadyRead(-1); // block forever
        }
        read = _tcpSock.read(msgStr.data(), EXTUI_SMALL_MSG_SIZE);
        */

		if ( read == -1 ) {
			qCritical("UiMsgThread::%s() : socket error", __FUNCTION__);
            perror("recv");
			break;
		}
		else if ( read == 0 ) {
//			qDebug("UiMsgThread::%s() : socket disconnected", __FUNCTION__);
			break;
		}


		//
		 // multiple threads will emit this signal in parallel.
		  //And by default, this is queued connection. So enqueueing is automatically thread-safe
		  //
		emit msgReceived(msgStr);

		//			QByteArray data(EXTUI_MSG_SIZE, '\0');
		//			sprintf(data.data(), "%d %s %d", ACK_FROM_WALL, "131.193.78.140", 30303);
		//			int t = ::send(sockfd, data.data(), data.size(), 0);
		//			qDebug("UiMsgThread::%s() : %d byte ACK_FROM_WALL [%s] sent", __FUNCTION__, t, data.constData());
	}
//	qDebug("UiMsgThread::%s() : thread finished", __FUNCTION__);
}


void SN_UiMsgThread::endThread() {
	::shutdown(_sockfd, SHUT_RDWR);
	_end = true;
}



/***
UiMsgThread::UiMsgThread(const quint32 uiclientid, int sockfd, QObject *parent)
    : QThread(parent)
    , _uiClientId(uiclientid)
	, _end(false)
{
	if ( ! _tcpSocket.setSocketDescriptor(sockfd) ) {
		qCritical("%s::%s() : UiMsgThread for %u failed to set socketdescriptor with socket %d", metaObject()->className(), __FUNCTION__, uiclientid, sockfd);
		deleteLater();
		return;
	}

	_tcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1); // disable Nagle's algorithm for small messages

	_peerAddress = _tcpSocket.peerAddress();
	qDebug() << "UiMsgThread created for peer" << _tcpSocket.peerAddress().toString();

//	if ( ! QObject::connect(&_tcpSocket, SIGNAL(readyRead()), this, SLOT(recvMsg())) ) {
//		qCritical("%s::%s() : failed to connect readyRead and recvMsg", metaObject()->className(), __FUNCTION__);
//	}

//	QObject::connect(&_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));

	start();
}

UiMsgThread::~UiMsgThread() {
	_tcpSocket.abort();
	_tcpSocket.close();
    qDebug() << "~UiMsgThread()";
}

void UiMsgThread::run() {
//	exec();
	
	QByteArray msg(EXTUI_MSG_SIZE, 0);
	while(!_end) {
		if (_tcpSocket.state() != QAbstractSocket::ConnectedState) break;

		_tcpSocket.waitForReadyRead(-1);
		qint64 read = _tcpSocket.read(msg.data(), msg.size());
		if ( read < 0 ) {
			qDebug() << "UiMsgThread::run() : read error";
			break;
		}
		else if ( read == 0) {
			qDebug() << "UiMsgThread::run() : no more data is available";
		}
		else {
			emit msgReceived(msg);
		}
	}

	_tcpSocket.abort();
	_tcpSocket.close();
}

void UiMsgThread::recvMsg() {
//	if (_tcpSocket.bytesAvailable() < EXTUI_MSG_SIZE)
//		return;

	QByteArray msg(EXTUI_MSG_SIZE, 0);
	_tcpSocket.read(msg.data(), EXTUI_MSG_SIZE);

	emit msgReceived(msg);
}

void UiMsgThread::sendMsg(const QByteArray &msgstr) {
	if ( _tcpSocket.write(msgstr.constData(), msgstr.size()) == -1 ) {
		qDebug() << "sendMsg error";
	}
	else {
		_tcpSocket.flush();
	}
}

void UiMsgThread::handleSocketError(QAbstractSocket::SocketError error) {
	qDebug() << "UiMsgThread socket error. Exit thread" << error;

	emit clientDisconnected(_uiClientId);
	
	_end = true;

	exit();
}

void UiMsgThread::endThread() {
	_end = true;
	//_tcpSocket.disconnectFromHost();
	_tcpSocket.abort();
	_tcpSocket.close();
//	qDebug() << _tcpSocket.state();
	exit();

	wait();
}
***/



