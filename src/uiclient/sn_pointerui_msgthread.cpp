#include "sn_pointerui_msgthread.h"
#include "sn_pointerui.h"

//#include <sys/socket.h>
//#include <sys/types.h>
//#include <arpa/inet.h>


SN_PointerUI_MsgThread::SN_PointerUI_MsgThread(QObject *parent)
    : QThread(parent)
//    , sockfd(0)
	, end(false)
    , uiclientid(0)
{
	connect(&_tcpMsgSock, SIGNAL(readyRead()), this, SLOT(recvMsg()));
	connect(&_tcpMsgSock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));
}

SN_PointerUI_MsgThread::~SN_PointerUI_MsgThread() {
//    ::close(sockfd);
	_tcpMsgSock.abort();
	_tcpMsgSock.close();
    qDebug() << "~MessageThread()";
}

bool SN_PointerUI_MsgThread::setSocketFD(int s) {
//	sockfd = s;
	return _tcpMsgSock.setSocketDescriptor(s);
}

void SN_PointerUI_MsgThread::handleSocketError(QAbstractSocket::SocketError error) {
	/**
	switch (error) {
	case QAbstractSocket::RemoteHostClosedError : {
		qDebug("%s::%s() : RemoteHostClosedError. exit() eventloop", metaObject()->className(), __FUNCTION__);
		exit();
		break;
	}
	case QAbstractSocket::NetworkError : {
		qDebug("%s::%s() : NetworkError. exit() eventloop", metaObject()->className(), __FUNCTION__);
		exit();
		break;
	}
	case QAbstractSocket::UnknownSocketError : {
		qDebug("%s::%s() : UnknownSocketError. exit() eventloop", metaObject()->className(), __FUNCTION__);
		exit();
		break;
	}
	}
	**/
	qDebug() << "MessageThread socket error" << error;
	exit();
}

void SN_PointerUI_MsgThread::endThread() {
//	::close(sockfd);
    end = true;

	//	_tcpMsgSock.disconnectFromHost();
	_tcpMsgSock.abort();
	_tcpMsgSock.close();
	qDebug() << _tcpMsgSock.state();
	exit();

	wait();
	sockfd = 0;
	uiclientid = -1;
	qDebug() << "MessageThread finished";
}

void SN_PointerUI_MsgThread::run() {
	qDebug() << "MessageThread starting event loop";

	exec(); // will block until exit() is called

	qDebug() << "MessageThread exit from event loop";

	if (_tcpMsgSock.state() == QAbstractSocket::ConnectedState) {
		_tcpMsgSock.abort();
		_tcpMsgSock.close();
	}
}


void SN_PointerUI_MsgThread::sendMsg(const QByteArray msg) {
	/*
    if ( ::send(sockfd, msg.data(), msg.size(), 0) <= 0 ) {
        qDebug("MessageThread::%s() : send error", __FUNCTION__);
    }
	*/
	if ( _tcpMsgSock.write(msg.constData(), msg.size()) == -1 ) {
		qDebug() << "sendMsg error";
	}
	else {
		_tcpMsgSock.flush();
	}
//		qDebug("MessageThread::%s() : msg [%s] sent", __FUNCTION__, msg.constData());
}


void SN_PointerUI_MsgThread::recvMsg() {
	if (_tcpMsgSock.bytesAvailable() < EXTUI_MSG_SIZE)
		return;

	QByteArray msg(EXTUI_MSG_SIZE, 0);
	int msgType = 0;
	_tcpMsgSock.read(msg.data(), EXTUI_MSG_SIZE);

	sscanf(msg.constData(), "%d", &msgType);

	switch( msgType ) {
    case ACK_FROM_WALL : {
        qDebug() << "MessageThread received ACK_FROM_WALL";

        int recvPort; // unsigned short
        QByteArray recvIP(64, 0);

        //
		//recvPort is wallport + uiClientID
		//
        sscanf(msg.data(), "%d %s %d", &msgType, recvIP.data(), &recvPort);
        qDebug("MessageThread::%s() : received ACK_FROM_WALL [%s]", __FUNCTION__, msg.constData());
        //QHostAddress recvaddr(QString(recvIP));

		break;

		/*
        // connect to data channel
        int datasock = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in walladdr;
        memset(&walladdr, 0, sizeof(walladdr));
        walladdr.sin_family = AF_INET;
        walladdr.sin_port = htons(recvPort);
        //walladdr.sin_addr.s_addr = recvaddr.toIPv4Address();
        inet_pton(AF_INET, recvIP.constData(), &walladdr.sin_addr.s_addr);
        if ( ::connect(datasock, (const struct sockaddr *)&walladdr, sizeof(walladdr)) != 0 ) {
            qCritical("%s::%s() : connect error", metaObject()->className(), __FUNCTION__);
            perror("connect");
            break;
        }
        qDebug() << "data channel connected to the wall";


        // send file
        QFile file(filen);
        file.open(QIODevice::ReadOnly);
        ::send(datasock, file.readAll().constData(), file.size(), 0);
        file.close();


        // close socket
        ::shutdown(datasock, SHUT_RDWR);
        if ( ::close(datasock) == 0 )
            qDebug() << "The file sent. data channel closed";
        */
    }
	}
}

	/**
void MessageThread::run() {
	if ( sockfd == 0 || uiclientid == -1 ) {
		qWarning() << "MessageThread failed to run. Finishing thread. socket" << sockfd << "uiclientid" << uiclientid;
		return;
	}
	
	end = false;
	qDebug() << "MessageThread is running";
	
    QByteArray msg(EXTUI_MSG_SIZE, 0);
    int msgType = 0;

    while(! end) {
        if ( ::recv(sockfd, msg.data(), msg.size(), MSG_WAITALL) <= 0 ) {
            qDebug("MessageThread::%s() : recv() error", __FUNCTION__);
            end = true;
            break;
        }
        //qDebug("MessageThread::%s() : new message arrived [%s]",__FUNCTION__, msg.constData());

        sscanf(msg.constData(), "%d", &msgType);

        switch( msgType ) {
        case ACK_FROM_WALL : {
            qDebug() << "MessageThread received ACK_FROM_WALL";

            int recvPort; // unsigned short
            QByteArray recvIP(INET_ADDRSTRLEN, 0);

            //
//              recvPort is wallport + uiClientID
              //
            sscanf(msg.data(), "%d %s %d", &msgType, recvIP.data(), &recvPort);
            qDebug("MessageThread::%s() : received ACK_FROM_WALL [%s]", __FUNCTION__, msg.constData());
            //QHostAddress recvaddr(QString(recvIP));


            // connect to data channel
            int datasock = ::socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in walladdr;
            memset(&walladdr, 0, sizeof(walladdr));
            walladdr.sin_family = AF_INET;
            walladdr.sin_port = htons(recvPort);
            //walladdr.sin_addr.s_addr = recvaddr.toIPv4Address();
            inet_pton(AF_INET, recvIP.constData(), &walladdr.sin_addr.s_addr);
            if ( ::connect(datasock, (const struct sockaddr *)&walladdr, sizeof(walladdr)) != 0 ) {
                qCritical("%s::%s() : connect error", metaObject()->className(), __FUNCTION__);
                perror("connect");
                break;
            }
            qDebug() << "data channel connected to the wall";


            // send file
            QFile file(filen);
            file.open(QIODevice::ReadOnly);
            ::send(datasock, file.readAll().constData(), file.size(), 0);
            file.close();


            // close socket
            ::shutdown(datasock, SHUT_RDWR);
            if ( ::close(datasock) == 0 )
                qDebug() << "The file sent. data channel closed";

            break;
        }
        case RESPOND_APP_LAYOUT : {
            //qDebug("MessageThread::%s() : RESPOND_APP_LAYOUT [%s]", __FUNCTION__, msg.constData());
            emit newAppLayout(msg);
            break;
        }
        }
    }
}
**/


//void MessageThread::registerApp(int mediatype, const QString &filename) {
//    QFileInfo fileinfo(filename);
//    filen = fileinfo.absoluteFilePath(); // for sending the file

//    /* prepare message */
//    QByteArray msg(EXTUI_MSG_SIZE, '\0');
//    int written = sprintf(msg.data(), "%d %s %s %d %lld %d %d %d %d %d %d %d",
//                          REG_FROM_UI,
//                          qPrintable(myipaddr), /* senderIP */
//                          qPrintable(fileinfo.fileName()), /* just the filename */
//                          mediatype,
//                          fileinfo.size(),
//                          FILE_TRANSFER, /* mode */
//                          0,0, /* resX, resY */
//                          0,0, /* viewX, viewY */
//                          1,1); /* posX, posY */
//    if ( written >= msg.size() ) {
//        qCritical("MessageThread::%s() : message truncated !", __FUNCTION__);
//    }

//	sendMsg(msg);

////    qDebug("MessageThread::%s() : REG_FROM_UI, %lld Byte sent", __FUNCTION__, sent);
//}


/*
void MessageThread::requestAppLayout() {
 QByteArray msg(EXTUI_MSG_SIZE, '\0');
 snprintf(msg.data(), msg.size(), "%d %llu dummy", TOGGLE_APP_LAYOUT, uiclientid);
 sendMsg(msg);

//	::recv(msgsock, msg.data(), msg.size(), MSG_WAITALL);
//	qDebug() << msg.data();
}
*/



