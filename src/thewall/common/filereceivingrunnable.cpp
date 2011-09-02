#include "filereceivingrunnable.h"
#include "../sagenextlauncher.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>

#include <QtGlobal>

FileReceivingTcpServer::FileReceivingTcpServer(SAGENextLauncher *l, int recvport)
    : QThread()
    , _end(false)
    , _launcher(l)
//    , _recvAddr(recvaddr)
    , _recvPort(recvport)
    , _serversocket(0)
    , _recvsocket(0)

{
    mutex.unlock();
    setAutoDelete(false);
    _destDir = QDir( QDir::homePath().append("/.sagenext/") );
}

FileReceivingTcpServer::~FileReceivingTcpServer() {
    ::shutdown(_recvsocket, SHUT_RDWR);
    ::close(_serversocket);
}

void FileReceivingTcpServer::setFileInfo(const QString &fname, int fsize) {
    mutex.lock();
    _filename = fname;
    _filesize = fsize;
    mutex.unlock();
}


void FileReceivingTcpServer::waitForConnection() {
    /* accept connection from sageStreamer */
    _serversocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if ( _serversocket == -1 ) {
//            qCritical("%s::%s() : couldn't create socket", metaObject()->className(), __FUNCTION__);
            perror("socket");
//            deleteLater();
    }

    // setsockopt
    int optval = 1;
    if ( ::setsockopt(_serversocket, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
//            qWarning("%s::%s() : setsockopt SO_REUSEADDR failed",metaObject()->className(),  __FUNCTION__);
    }

    // bind to port
    struct sockaddr_in localAddr, clientAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
//    if ( _recvAddr == "")
//        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
//    else
//        ::inet_pton(AF_INET, qPrintable(_recvAddr), &localAddr.sin_addr.s_addr);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(_recvPort);

    // bind
    if( ::bind(_serversocket, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
//            qCritical("%s::%s() : bind error", metaObject()->className(), __FUNCTION__);
            perror("bind");
//            deleteLater();
    }

    // put in listen mode
    ::listen(_serversocket, 15);

    // accept
    /** accept will BLOCK **/
//	qDebug("SageStreamWidget::%s() : Blocking waiting for sender to connect to TCP port %d", __FUNCTION__,protocol+port);
    memset(&clientAddr, 0, sizeof(clientAddr));
    int addrLen = sizeof(struct sockaddr_in);
    if ((_recvsocket = accept(_serversocket, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1) {
//            qCritical("%s::%s() : accept error",metaObject()->className(), __FUNCTION__);
            perror("accept");
//            deleteLater();
    }
}

void FileReceivingTcpServer::run() {

    while(!_end) {
        waitForConnection();

        Q_ASSERT(_recvsocket > 0);

        QString filename = QDir::homePath().append("/.sagenext/");
        filename.append(_filename);

        mutex.lock();

        int filefd = ::open(qPrintable(filename), O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IROTH);
        if (filefd<0) {
            qDebug() << "FileReceivingRunnable could not open the file for writing";
            return;
        }


        int recvsize = 0;
        int written = 0, totalwritten = 0;

        while (totalwritten < _filesize) {
            char buff[128 * 1024];
            recvsize = ::recv(_recvsocket, (void *)buff, 128*1024, MSG_WAITALL);
            if ( recvsize <= 0 ) {
                break;
            }

            written = ::write(filefd, buff, recvsize);
            if (written <=0) {
                break;
            }
            else {
                totalwritten += written;
            }
        }
        qDebug() << filename << totalwritten << "Byte";


        ::close(filefd);


        mutex.unlock();

        Q_ASSERT(_launcher);
        //    _launcher->launch(media type, filename);

        ::shutdown(_recvsocket, SHUT_RDWR);
        ::close(_recvsocket);
    }

    ::close(_serversocket);
}
