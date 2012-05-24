#include "sagepixelreceiver.h"

#include "railawarewidget.h"
#include "appinfo.h"
#include "perfmonitor.h"
#include "affinityinfo.h"

#include "../../common/imagedoublebuffer.h"
#include "../../sage/fsmanagermsgthread.h"

#include <QTcpSocket>
#include <QUdpSocket>

#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
//#include <sched.h>
#include <unistd.h>


SN_SagePixelReceiver::SN_SagePixelReceiver(int protocol, int sockfd, DoubleBuffer *idb, bool usepbo, void **pbobufarray, pthread_mutex_t *pboMutex, pthread_cond_t *pboCond, AppInfo *ap, PerfMonitor *pm, AffinityInfo *ai, /*RailawareWidget *rw, QMutex *mmm, QWaitCondition *wwcc,*/ const QSettings *s, QObject *parent /* 0 */)
    : QThread(parent)
    , s(s)
    , _end(false)
    , _tcpsocket(sockfd)
    , _udpsocket(0)
    , _delay(0)
    , doubleBuffer(idb)
    , appInfo(ap)
    , perf(pm)
    , affInfo(ai)

    , _usePbo(usepbo)
    , __bufferMapped(false)
    , _pbobufarray(pbobufarray)
    , _pboMutex(pboMutex)
    , _pboCond(pboCond)
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
		// to make this work, this thread has to have its own event loop.
		// Use exec() instead of start()
		_udpsocket = new QUdpSocket(this);
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
    _end = true;
    ::shutdown(_tcpsocket, SHUT_RDWR);
    ::close(_tcpsocket);
}

SN_SagePixelReceiver::~SN_SagePixelReceiver() {
	_end = true;
//	::shutdown(socket, SHUT_RDWR);
	::close(_tcpsocket);

	qDebug() << "[" << QTime::currentTime().toString("hh:mm:ss.zzz") << "] ~SN_SagePixelReceiver";
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
	int sent = send(_tcpsocket, buffer.data(), buffer.size(), 0);
	Q_UNUSED(sent);

	buffer.clear();
	// receive sender's (sail) UDP port
	qDebug("SagePixelReceiver::%s() : waiting for sender to send her UDP port", __FUNCTION__);
	int read = recv(_tcpsocket, buffer.data(), buffer.size(), MSG_WAITALL);
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


void SN_SagePixelReceiver::flip(int idx) {
	__bufferMapped = true;
	_pboBufIdx = idx;
}


void SN_SagePixelReceiver::run() {

	QThread::setTerminationEnabled(true);

	/*
	 * Initially store current affinity settings of this thread using NUMA API
	 */
	if (affInfo) {
		affInfo->figureOutCurrentAffinity();
		affInfo->setCpuOfMine( sched_getcpu() , s->value("system/sailaffinity", false).toBool());
	}


	// Actual time elapsed
	struct timeval tvs, tve;

    // CPU time elapsed
    struct timespec ts_start, ts_end;

    if(perf && s->value("system/resourcemonitor",false).toBool()) {
//		perf->getRecvTimer().start(); //QTime::start()

        gettimeofday(&tvs, 0);

        // #include <time.h>
        // Link with -lrt
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_start);
	}

	int byteCount = appInfo->frameSizeInByte();
	unsigned char *bufptr = 0;

	if (!_usePbo) {
		bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits();
	}

	while(! _end ) {
		if ( affInfo && s->value("system/resourcemonitor",false).toBool()) {
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


		if (_usePbo) {
			// wait for glMapBufferARB
			pthread_mutex_lock(_pboMutex);
			while(!__bufferMapped) {
//				qDebug() << "thread waiting ..";
				pthread_cond_wait(_pboCond, _pboMutex);
			}
			bufptr = (unsigned char *)_pbobufarray[_pboBufIdx];
//			qDebug() << "thread writing to" << _pboBufIdx << bufptr;
		}




        /*
        if (s->value("system/scheduler",false).toBool()) {
//            qreal adjustment = (1.0 / perf->getAdjustedFps()) - (1.0 / perf->getExpetctedFps()); // in second
            qreal adjustment_msec = 1000.0 * (1.0/perf->getAdjustedFps());

            if ( adjustment_msec > 0 ) {
                // adding delay to respect the adjusted quality
                QThread::msleep( (unsigned long)adjustment_msec );

//                //
//                struct timespec request;
//                request.tv_sec = 0;
//                request.tv_nsec = (long)adjustment * 1e+9;
//                if ( clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID, 0, &request, 0) != 0) {
////                        perror("\n\nclock_nanosleep");
//                }
//                //
            }
        }
        */
        if (_delay > 0) {
            QThread::msleep(_delay);
        }



        ssize_t totalread = 0;
		ssize_t read = 0;

//		gettimeofday(&lats, 0);

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

		// PIXEL RECEIVING
		while (totalread < byteCount ) {
			// If remaining byte is smaller than user buffer length (which is groupSize)
			if ( byteCount-totalread < appInfo->networkUserBufferLength() ) {
				read = recv(_tcpsocket, bufptr, byteCount-totalread , MSG_WAITALL);
			}
			// otherwise, always read groupSize bytes
			else {
				read = recv(_tcpsocket, bufptr, appInfo->networkUserBufferLength(), MSG_WAITALL);
			}
			if ( read == -1 ) {
//				qDebug("SagePixelReceiver::run() : error while reading.");
				_end = true;
				break;
			}
			else if ( read == 0 ) {
//				qDebug("SagePixelReceiver::run() : sender disconnected");
				_end = true;
				break;
			}
			// advance pointer
			bufptr += read;
			totalread += read;
		}
		if ( totalread < byteCount  ||  _end ) break;
		read = totalread;


		if (_usePbo) {
			__bufferMapped = false;
			emit frameReceived();
			pthread_mutex_unlock(_pboMutex);
		}
		else {

			/********************************/   // double buffering
			Q_ASSERT(doubleBuffer);

			// will wait until consumer (SageStreamWidget) consumes the data
			// i.e. until consumer calls doubleBuffer->releaseBackBuffer();
			doubleBuffer->swapBuffer();
			//qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "swapBuffer returned";

			emit frameReceived(); // Queued Connection. Will trigger SageStreamWidget::updateWidget()

			//
			// getFrontBuffer() will return immediately. There's no mutex waiting in this function
			//
			bufptr = static_cast<QImage *>(doubleBuffer->getFrontBuffer())->bits(); // bits() will detach
			/***************************************/
		}

//		gettimeofday(&late, 0);


		/**********************************************/ // scheduling with wait condition
        /*
		if (s->value("system/scheduler").toBool()) {
			if ( QString::compare(s->value("system/scheduler_type").toString(), "SMART", Qt::CaseInsensitive) == 0) {
				_mutex.lock();
				_waitCond.wait(&_mutex);
				_mutex.unlock();
			}
			else {
				qreal adjustment = (1.0 / perf->getAdjustedFps()) - (1.0 / perf->getExpetctedFps()); // second
				adjustment *= 1000.0; // milli-second

				if ( adjustment > 0 ) {
					// adding delay to respect the adjusted quality
					QThread::msleep( (unsigned long)adjustment );

//                    struct timespec delay;
//                    delay.tv_sec = 0;
//                    delay.tv_nsec = adjustment * 1000000; // nano sec
//                    nanosleep(&delay, 0);
				}
			}
		}
        */
		/********************************************/


		if (perf && s->value("system/resourcemonitor",false).toBool()) {

            gettimeofday(&tve, 0);

            qreal actualtime_second = ((double)tve.tv_sec + (double)tve.tv_usec * 1e-6) - ((double)tvs.tv_sec + (double)tvs.tv_usec * 1e-6);
            tvs = tve;

            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);

			qreal cputime_second = ((double)ts_end.tv_sec + (double)ts_end.tv_nsec * 1e-9) - ((double)ts_start.tv_sec + (double)ts_start.tv_nsec * 1e-9);
            ts_start = ts_end;


            //
			// calculate delay, fps, cpu usage, everything...
            // perfMon->recvTimer will be restarted in this function.
            // So the delay calculated in this function includes blocking (which is forced by the pixel streamer) delay
            //
			perf->addToCumulativeByteReceived(read, actualtime_second, cputime_second);
		}
	} /*** end of receiving loop ***/

	/* pixel receiving thread exit */
//	qDebug("SagePixelReceiver : thread exit");
}



