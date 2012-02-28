#include "sn_pointerui_msgthread.h"
#include "sn_pointerui.h"

//#include <sys/socket.h>
//#include <sys/types.h>
//#include <arpa/inet.h>


SN_MsgObject::SN_MsgObject(const QHostAddress &hostaddr, quint16 port, QObject *parent)
    : QObject(parent)
    , _hostaddr(hostaddr)
    , _port(port)
    , _uiclientid(0)
{
	if (! QObject::connect(&_tcpMsgSock, SIGNAL(readyRead()), this, SLOT(recvMsg())) ) {
		qDebug() << "failed to connect readRead() and recvMsg()";
	}

//	if ( !QObject::connect(&_tcpMsgSock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError))) ) {
//		qDebug() << "failed to connect socketError ";
//	}
}

SN_MsgObject::~SN_MsgObject() {
	_tcpMsgSock.abort();
	_tcpMsgSock.close();
    qDebug() << "~~SN_PointerUI_MsgThread()";
}

void SN_MsgObject::connectToTheWall() {
	_tcpMsgSock.connectToHost(_hostaddr, _port);
	_tcpMsgSock.setSocketOption(QAbstractSocket::LowDelayOption, 1); // disable Nagle's algorithm


	/*!
	  receive the scene size and my uiclientid
	  */
	QByteArray buf(EXTUI_MSG_SIZE, '\0');

//	_tcpMsgSock.waitForReadyRead(-1); // block permanently until data is available
	while (_tcpMsgSock.bytesAvailable() < EXTUI_MSG_SIZE) {

		_tcpMsgSock.waitForReadyRead(10);

	}
	if (_tcpMsgSock.read(buf.data(), EXTUI_MSG_SIZE) == -1) {
		// error
		qDebug() << "SN_PointerUI_MsgThread::run() : read error while receving scene size";
		return;
	}

	quint32 uiclientid;
	int x,y,ftpport;
	sscanf(buf.constData(), "%u %d %d %d", &uiclientid, &x, &y, &ftpport);
	_uiclientid = uiclientid;

	emit connectedToTheWall(_uiclientid, x, y, ftpport);
}


void SN_MsgObject::handleSocketError(QAbstractSocket::SocketError error) {
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
//	qDebug() << "MessageThread socket error" << error;
}



void SN_MsgObject::sendMsg(const QByteArray msg) {
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
//	qDebug("SN_MsgObject::%s() : msg [%s] sent", __FUNCTION__, msg.constData());
}


void SN_MsgObject::recvMsg() {
	if (_tcpMsgSock.bytesAvailable() < EXTUI_MSG_SIZE) {
		qDebug() << _tcpMsgSock.bytesAvailable();
		return;
	}

	QByteArray msg(EXTUI_MSG_SIZE, '\0');

	qint64 r = _tcpMsgSock.read(msg.data(), EXTUI_MSG_SIZE);

	qDebug() << "msg received" << r << "bytes" << msg;

	int msgType = 0;
	sscanf(msg.constData(), "%d", &msgType);

	switch( msgType ) {
    case SAGENext::ACK_FROM_WALL : {
        qDebug() << "msg ACK_FROM_WALL";

        int recvPort; // unsigned short
        QByteArray recvIP(64, 0);

        //
		//recvPort is wallport + uiClientID
		//
        sscanf(msg.data(), "%d %s %d", &msgType, recvIP.data(), &recvPort);
        qDebug("MessageThread::%s() : received ACK_FROM_WALL [%s]", __FUNCTION__, msg.constData());
        //QHostAddress recvaddr(QString(recvIP));

		break;
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



