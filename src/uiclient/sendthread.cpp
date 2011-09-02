#include "sendthread.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

SendThread::SendThread(const QString &recvaddr, int recvport, QObject *parent)
    : QThread(parent)
    , end(false)
    , receiverPort(recvport)
{
    mutex.unlock();
    fileQueue.clear();
    receiverAddr.setAddress(recvaddr);
}

SendThread::~SendThread() {
}

void SendThread::run() {

    /* connect to File Receiving Server at the UiServer */
    int datasock = ::socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in walladdr;
    memset(&walladdr, 0, sizeof(walladdr));
    walladdr.sin_port = htons(receiverPort);
    walladdr.sin_addr.s_addr = receiverAddr.toIPv4Address();
    //        inet_pton(AF_INET, qPrintable(), &walladdr.sin_addr.s_addr);

    if ( ::connect(datasock, (const struct sockaddr *)&walladdr, sizeof(walladdr)) != 0 ) {
        qCritical("SendThread::%s() : connect error", __FUNCTION__);
    }
    qDebug() << "data channel connected to the wall";




    while(!end) {
        mutex.lock();
        fileReady.wait(&mutex);

        /* send file */
        // open for reading
        QString filename = fileQueue.front();
        fileQueue.pop_front();
        int filefd = ::open(qPrintable(filename), O_RDONLY);
        if (filefd == -1) {
            qCritical("%s::%s() : Counldn't open the file", metaObject()->className(), __FUNCTION__);
            break;
        }

        char buff[128*1024];
        int read = 0, totalread= 0;
        int sent = 0, totalsent = 0;

        while (1) {
            read = ::read(filefd, buff, 128*1024);
            if (read < 0) { // read error
                qCritical("%s::%s() : error while reading the file", metaObject()->className(), __FUNCTION__);
                break;
            }
            else if (read == 0) { // no more data
                qDebug() << "send finished. total" << totalread << "Byte";
                break;
            }
            else {
                totalread += read;
                sent = ::send(datasock, buff, read, 0);
                if (sent < 0) { // send error
                    qDebug() << "send error";
                    break;
                }
                else {
                    totalsent += sent;
                }
            }
        }
        qDebug() << totalsent << "Byte sent";

        ::close(filefd);
        mutex.unlock();
    }



    ::shutdown(datasock, SHUT_RDWR);
    ::close(datasock);
}

void SendThread::sendFile( const QString &filename) {
    mutex.lock();
    //    receiverAddr = addr;
    //    receiverPort = port;
    fileQueue.enqueue(filename);
    fileReady.wakeOne();
    mutex.unlock();
}
