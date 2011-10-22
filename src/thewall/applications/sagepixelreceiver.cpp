#include "sagepixelreceiver.h"

#include "base/railawarewidget.h"
#include "base/appinfo.h"
#include "base/perfmonitor.h"
#include "base/affinityinfo.h"

#include "../common/imagedoublebuffer.h"

#include "../sage/fsmanagermsgthread.h"

#include <QTcpSocket>
#include <QUdpSocket>

#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
//#include <sched.h>


SN_SagePixelReceiver::SN_SagePixelReceiver(int protocol, int sockfd, /*QImage *img*/ DoubleBuffer *idb, AppInfo *ap, PerfMonitor *pm, AffinityInfo *ai, /*RailawareWidget *rw, QMutex *mmm, QWaitCondition *wwcc,*/ const QSettings *s, QObject *parent /* 0 */) :
	QThread(parent),
	s(s),
	_end(false),
	socket(sockfd),
	_socket(0),
	image(0),
	doubleBuffer(idb),
	appInfo(ap),
	perf(pm),
	affInfo(ai),
	frameCounter(0)
{

	QThread::setTerminationEnabled(true);

	if ( protocol == SAGE_TCP ) {
		/*
		 // if you do this, you MUST use _socket instead of directly using socket
		_socket = new QTcpSocket(this);
		if ( ! _socket->setSocketDescriptor(sockfd) ) {
			qDebug("%s() : AH ssibal", __FUNCTION__);
		}
		*/
	}
	else {
		_socket = new QUdpSocket(this);
		int udpPortNumber = receiveUdpPortNumber();
		if ( udpPortNumber <= 0 ) {
			qCritical("%s::%s() : failed to receive UDP port number for streaming", qPrintable(objectName()), __FUNCTION__);
			_end = true;
		}
		else {
		}
	}

}


void SN_SagePixelReceiver::endReceiver() {
//    _mutex.lock();
    _end = true;
    ::shutdown(socket, SHUT_RDWR);
    ::close(socket);
//    _waitCond.wakeOne();
//    _mutex.unlock();
}

SN_SagePixelReceiver::~SN_SagePixelReceiver() {

//	_mutex.lock();

	_end = true;
//	::shutdown(socket, SHUT_RDWR);
	::close(socket);

//	_waitCond.wakeOne();
//	_mutex.unlock();

//        terminate();
//        wait();

	qDebug() << "return ~SagePixelReceiver" << QTime::currentTime().toString("hh:mm:ss.zzz");
}

void SN_SagePixelReceiver::receivePixel() {
	QMutexLocker locker(&_mutex);
	_waitCond.wakeOne();
}

int SN_SagePixelReceiver::receiveUdpPortNumber() {
	QByteArray buffer(1024, 0);
	buffer.clear();

	QThread::msleep(1000);

	// send receiver's(display) UDP port
	sprintf(buffer.data(), "%d", 24243);
	int sent = send(socket, buffer.data(), buffer.size(), 0);
	Q_UNUSED(sent);

	buffer.clear();
	// receive sender's (sail) UDP port
	qDebug("SagePixelReceiver::%s() : waiting for sender to send her UDP port", __FUNCTION__);
	int read = recv(socket, buffer.data(), buffer.size(), MSG_WAITALL);
	int udpPortNumber = 0;
	sscanf(buffer.data(), "%d", &udpPortNumber);
	qDebug("SagePixelReceiver::%s() : received sender's UDP port number %d", __FUNCTION__, udpPortNumber);

	if ( read == 1024 && udpPortNumber > 0 ) {
		return udpPortNumber;
	}
	else if ( read <= 0 ){
		return read;
	}
	else {
		return -1;
	}
}


void SN_SagePixelReceiver::run() {

//	fprintf(stderr, "SagePixelReceiver::run() : starting pixel receiving thread");

	QThread::setTerminationEnabled(true);

	/**
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(3, &cpuset);
	if ( sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0 ) {
		qDebug("SagePixelReceiver::%s() : sched_setaffinity error", __FUNCTION__);
	}
	**/

	/*
	 * Initially store current affinity settings of this thread using NUMA API
	 */
	if (affInfo) {
		affInfo->figureOutCurrentAffinity();

#if defined(Q_OS_LINUX)
		affInfo->setCpuOfMine( sched_getcpu() ,s->value("system/sailaffinity", false).toBool()); // true will cause sail affinityinfo
#endif
	}


	/*
	QThread *thread = this->thread();
	// The operating system will schedule the thread according to the priority parameter. The effect of the priority parameter is dependent on the operating system's scheduling policy. In particular, the priority will be ignored on systems that do not support thread priorities (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler for more details).
	qDebug("SagePixelReceiver::%s() : priority %d", __FUNCTION__, thread->priority());
	*/

	// network recv latency
	struct timeval lats, late;

	struct rusage ru_start, ru_end;
	if(perf) {
		perf->getRecvTimer().start(); //QTime::start()

#if defined(Q_OS_LINUX)
		getrusage(RUSAGE_THREAD, &ru_start); // that of calling thread. Linux specific
#elif defined(Q_OS_MAC)
		getrusage(RUSAGE_SELF, &ru_start);
#endif
	}

	int byteCount = appInfo->frameSizeInByte();

//	unsigned char *bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits();
	unsigned char *bufptr = (unsigned char *)doubleBuffer->getFrontBuffer();

//	unsigned char *bufptr = 0;
//	QImage localImage(appInfo->nativeSize(), QImage::Format_RGB888);

	while(! _end ) {
		if ( affInfo ) {
			if ( affInfo->isChanged() ) {
				// apply new affinity;
				//	qDebug("SagePixelReceiver::%s() : applying new affinity parameters", __FUNCTION__);
				affInfo->applyNewParameters(); // use NUMA lib

				// update info in affInfo
				// this function must be called in this thread
				affInfo->figureOutCurrentAffinity(); // use NUMA lib
			}
			else {
#if defined(Q_OS_LINUX)
				/* this is called too many times */
				// if cpu has changed, affinfo::cpuOfMineChanged() will be emitted
				// which is connected to ResourceMonitor::updateAffInfo()
				affInfo->setCpuOfMine( sched_getcpu() , s->value("system/sailaffinity", false).toBool());
#endif
			}
		}


		// recv header
		/*
		read = recv(socket, (void *)header.data(), header.size(), MSG_WAITALL);
		if ( read == -1 ) {
			qCritical("PixelReceiver::%s() : error while reading BLOCK_HEADER. %s", __FUNCTION__, "");
			break;
		}
		else if ( read == 0 ) {
			qCritical("PixelReceiver::%s() : sender disconnected", __FUNCTION__);
			break;
		}
		// frame number, pixel size, memwidth , bufsize
		int fnum, pixelSize, memWidth, bufSize;
		sscanf(header.constData(), "%d %d %d %d", &fnum, &pixelSize, &memWidth, &bufSize);
		qDebug("PixelReceiver::%s() : received block header [%s]", __FUNCTION__, header.constData());
		*/

		ssize_t totalread = 0;
		ssize_t read = 0;

		gettimeofday(&lats, 0);

		// PRODUCER
		while (totalread < byteCount ) {
			// If remaining byte is smaller than user buffer length (which is groupSize)
			if ( byteCount-totalread < appInfo->networkUserBufferLength() ) {
				read = recv(socket, bufptr, byteCount-totalread , MSG_WAITALL);
			}
			// otherwise, always read groupSize bytes
			else {
				read = recv(socket, bufptr, appInfo->networkUserBufferLength(), MSG_WAITALL);
			}
			if ( read == -1 ) {
				qDebug("SagePixelReceiver::run() : error while reading.");
				_end = true;
				break;
			}
			else if ( read == 0 ) {
				qDebug("SagePixelReceiver::run() : sender disconnected");
				_end = true;
				break;
			}
			// advance pointer
			bufptr += read;
			totalread += read;
		}
		if ( totalread < byteCount  ||  _end ) break;
		read = totalread;
		gettimeofday(&late, 0);

//		++frameCounter;
//		qDebug() << (QTime::currentTime()).toString("mm:ss.zzz") << " recevier : " << frameCounter << " received";



		/********************************/   // double buffering
		Q_ASSERT(doubleBuffer);

		// will wait until consumer (SageStreamWidget) consumes the data
		// i.e. until consumer calls doubleBuffer->releaseBackBuffer();
		doubleBuffer->swapBuffer();
		//qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "swapBuffer returned";

//		perf->getConvTimer().start();

		emit frameReceived(); // Queued Connection. Will trigger SageStreamWidget::updateWidget()

		//qDebug("%s() : signal emitted", __FUNCTION__);
		//QMetaObject::invokeMethod(widget, "scheduleUpdate", Qt::QueuedConnection);

		//getFrontBuffer() will return immediately. There's no mutex waiting in this function
//		bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits(); // bits() will detach
		bufptr = (unsigned char *)doubleBuffer->getFrontBuffer();
		//qDebug("%s() : grabbed front buffer", __FUNCTION__);
		/***************************************/




		/**********************************************/ // scheduling with wait condition
		if (s->value("system/scheduler").toBool()) {
			if ( QString::compare(s->value("system/scheduler_type").toString(), "SMART", Qt::CaseInsensitive) == 0) {
				_mutex.lock();
				_waitCond.wait(&_mutex);
				_mutex.unlock();
			}
			else {
				qreal adjustment = (1.0 / perf->getAdjustedFps()) - (1.0 / perf->getExpetctedFps()); // second
				adjustment *= 1000.0; // millisecond
				if ( adjustment >= 1 ) {
					// adding delay
					QThread::msleep( (unsigned long)adjustment );
				}
			}
		}
		/********************************************/

		/*********************************/
//		qreal adjustment = (1.0 / perf->getAdjustedFps()) - (1.0 / perf->getExpetctedFps()); // second
//		adjustment *= 1000.0; // millisecond
//		if ( adjustment >= 1 ) {
//			// adding delay
//			QThread::msleep( (unsigned long)adjustment );
//		}
		/*************************************/






		if (perf) {
#if defined(Q_OS_LINUX)
			getrusage(RUSAGE_THREAD, &ru_end);
#elif defined(Q_OS_MAC)
			getrusage(RUSAGE_SELF, &ru_end);
#endif

			qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);

			// calculate
//			perf->updateRecvLatency(read, ru_start, ru_end); // QTimer::restart()
			perf->updateObservedRecvLatency(read, networkrecvdelay, ru_start, ru_end);
			ru_start = ru_end;

//			qDebug() << perf->getCpuUsage();
		}
	} /*** end of receiving loop ***/

//	if(doubleBuffer) {

//		doubleBuffer->swapBuffer(); // not a good idea

//		doubleBuffer->releaseLocks();
//		delete doubleBuffer; // do not delete doublebuffer here
//	}

	/* pixel receiving thread exit */
        qDebug("SagePixelReceiver : thread exit");
}



