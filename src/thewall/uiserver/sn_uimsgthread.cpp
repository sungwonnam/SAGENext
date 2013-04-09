#include "uimsgthread.h"
#include "uiserver.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

UiMsgThread::UiMsgThread(const quint32 id, int sock, QObject *parent) :
	QThread(parent)
  , _uiClientId(id)
  , _end(false)
  , _sockfd(sock)
//  , _udpSock(0)
{
	int optval = 1;
	//socket.setSocketOption(QAbstractSocket::LowDelayOption, (const QVariant *)&optval);
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
	if ( getpeername(_sockfd, (struct sockaddr *)&addr, &len) != 0 ) {
		qCritical() << "UiMsgThread failed to get peer address";
	}
	else {
		char str[INET_ADDRSTRLEN];
		if ( inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN) ) {
//			qDebug() << QString(str);
			_peerAddress = QHostAddress(QString(str));
		}
		else {
			perror("inet_ntop");
		}
	}

    /*
    _udpSock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (_udpSock == -1) {
        perror("UDP socket");
    }
    else {
        struct sockaddr_in serveraddr;
        bzero(&serveraddr, sizeof(struct sockaddr_in));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = 30003 + 10 + _uiClientId;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if ( -1 == ::bind(_udpSock, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in)) ) {
            perror("UDP bind");
        }
    }
    */
}

UiMsgThread::~UiMsgThread() {
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

void UiMsgThread::sendMsg(const QByteArray &msgstr) {
	if (msgstr.size() != EXTUI_MSG_SIZE) {
		qWarning("%s::%s() : You're about to send a message with its size %d", metaObject()->className(), __FUNCTION__, msgstr.size());
	}

	ssize_t sent = ::send(_sockfd, msgstr.data(), msgstr.size(), 0);
	if ( sent <= 0 ) {
		qCritical("UiMsgThread::%s() : send error", __FUNCTION__);
	}
	else {
		//	qDebug("UiMsgThread::%s() : msg [%s] sent", __FUNCTION__, msgstr.constData());
	}
}

void UiMsgThread::run() {
//	qDebug("UiMsgThread::%s() : ui client thread %u has started", __FUNCTION__, _uiClientId);

	QByteArray msgStr(EXTUI_SMALL_MSG_SIZE, 0);
	int read = 0;

	while(!_end) {
		msgStr.fill(0);
		if ( (read = recv(_sockfd, (void *)msgStr.data(), EXTUI_SMALL_MSG_SIZE, MSG_WAITALL)) == -1 ) {
			qCritical("UiMsgThread::%s() : socket error", __FUNCTION__);
            perror("recv");
			break;
		}
		else if ( read == 0 ) {
//			qDebug("UiMsgThread::%s() : socket disconnected", __FUNCTION__);
			break;
		}

        //
        // if srcaddr is null then connection is required
        //
        /*
        if ( (read = recvfrom(_udpSock, (void *)msgStr.data(), EXTUI_SMALL_MSG_SIZE, 0, 0, 0)) == -1) {
            qCritical("UiMsgThread::%s() : socket error", __FUNCTION__);
			break;
		}
		else if ( read == 0 ) {
			qDebug("UiMsgThread::%s() : socket disconnected", __FUNCTION__);
			break;
        }
        */
//		qDebug("UiMsgThread::%s() : received external UI msg [%s]", __FUNCTION__, msgStr.data());

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


void UiMsgThread::endThread() {
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



