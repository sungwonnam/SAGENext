#include "sendthread.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

SendThread::SendThread( QObject *parent) :
	QThread(parent)
{
	end = false;
	mutex.unlock();
	fileQueue.clear();
}

SendThread::~SendThread() {
}

void SendThread::run() {

	while(!end) {

		mutex.lock();
		fileReady.wait(&mutex);

		/* connect to data channel */
		int datasock = ::socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in walladdr;
		memset(&walladdr, 0, sizeof(walladdr));
		walladdr.sin_port = htons(receiverPort);
		walladdr.sin_addr.s_addr = receiverAddr.toIPv4Address();
//		inet_pton(AF_INET, ipaddr, &walladdr.sin_addr.s_addr);
		if ( ::connect(datasock, (const struct sockaddr *)&walladdr, sizeof(walladdr)) != 0 ) {
			qCritical("SendThread::%s() : connect error", __FUNCTION__);
		}
		qDebug() << "data channel connected to the wall";


		/* send file */


		mutex.unlock();

	}

}

void SendThread::sendFile(const QHostAddress &addr, int port, const QString &filename) {
	mutex.lock();
	receiverAddr = addr;
	receiverPort = port;
	fileQueue.enqueue(filename);
	fileReady.wakeOne();
	mutex.unlock();
}
