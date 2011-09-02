#include "messagethread.h"
#include "externalguimain.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

MessageThread::MessageThread(int sock, const quint64 uiid, const QString &myip, QObject *parent)
    : QThread(parent)
    , sockfd(sock)
    , uiclientid(uiid)
    , myipaddr(myip)
{
    end = false;
}

MessageThread::~MessageThread() {
    ::close(sockfd);
    qDebug() << "~MessageThread()";
}

void MessageThread::run() {

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

            /*
              recvPort is wallport + uiClientID
              */
            sscanf(msg.data(), "%d %s %d", &msgType, recvIP.data(), &recvPort);
            qDebug("MessageThread::%s() : received ACK_FROM_WALL [%s]", __FUNCTION__, msg.constData());
            //QHostAddress recvaddr(QString(recvIP));


            /* connect to data channel */
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


            /* send file */
            QFile file(filen);
            file.open(QIODevice::ReadOnly);
            ::send(datasock, file.readAll().constData(), file.size(), 0);
            file.close();


            /* close socket */
            ::shutdown(datasock, SHUT_RDWR);
            if ( ::close(datasock) == 0 )
                qDebug() << "The file sent. data channel closed";

            break;
        }
        case RESPOND_APP_LAYOUT : {
            //				qDebug("MessageThread::%s() : RESPOND_APP_LAYOUT [%s]", __FUNCTION__, msg.constData());
            emit newAppLayout(msg);
            break;
        }
        }
    }
}

void MessageThread::endThread() {
    end = true;
}

/*
void MessageThread::requestAppLayout() {
 QByteArray msg(EXTUI_MSG_SIZE, '\0');
 snprintf(msg.data(), msg.size(), "%d %llu dummy", TOGGLE_APP_LAYOUT, uiclientid);
 sendMsg(msg);

//	::recv(msgsock, msg.data(), msg.size(), MSG_WAITALL);
//	qDebug() << msg.data();
}
*/

void MessageThread::registerApp(int mediatype, const QString &filename) {
    QFileInfo fileinfo(filename);
    filen = fileinfo.absoluteFilePath(); // for sending the file

    /* prepare message */
    QByteArray msg(EXTUI_MSG_SIZE, '\0');
    int written = sprintf(msg.data(), "%d %s %s %d %lld %d %d %d %d %d %d %d",
                          REG_FROM_UI,
                          qPrintable(myipaddr), /* senderIP */
                          qPrintable(fileinfo.fileName()), /* just the filename */
                          mediatype,
                          fileinfo.size(),
                          FILE_TRANSFER, /* mode */
                          0,0, /* resX, resY */
                          0,0, /* viewX, viewY */
                          1,1); /* posX, posY */
    if ( written >= msg.size() ) {
        qCritical("MessageThread::%s() : message truncated !", __FUNCTION__);
    }

    /* send reg message */
    //qint64 sent = msgSock->write(msg.constData(), msg.size());
    qint64 sent = ::send(sockfd, msg.data(), msg.size(), 0);
    qDebug("MessageThread::%s() : REG_FROM_UI, %lld Byte sent", __FUNCTION__, sent);
}

void MessageThread::sendMsg(const QByteArray &msg) {
    if ( ::send(sockfd, msg.data(), msg.size(), 0) <= 0 ) {
        qDebug("MessageThread::%s() : send error", __FUNCTION__);
        return;
    }
    //	qDebug("MessageThread::%s() : msg [%s] sent", __FUNCTION__, msg.constData());
}


