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
    , _settings(s)
    , _end(false)

    , _tcpsocket(sockfd)
    , _udpsocket(0)

    , _delay(0)

    , _doubleBuffer(idb)

    , _appInfo(ap)
    , _perfMon(pm)
    , _affInfo(ai)

    , _usePbo(usepbo)
    , _pboBufIdx(0)
    , _pbobufarray(pbobufarray)
    , _pboMutex(pboMutex)
    , _pboCond(pboCond)
{
	QThread::setTerminationEnabled(true);

        int optVal = 1048576;
        int optLen = sizeof(optVal);
        setsockopt(_tcpsocket, SOL_SOCKET, SO_RCVBUF, (void *)&optVal, (socklen_t)optLen);

    if (protocol == SAGE_UDP) {
		// to make this work, this thread has to have its own event loop.
		// Use exec() instead of start()
        _udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
        Q_ASSERT(_udpsocket);
		int udpPortNumber = m_receiveUdpPortNumber();
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



int SN_SagePixelReceiver::m_receiveUdpPortNumber() {
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




void SN_SagePixelReceiver::run() {

	QThread::setTerminationEnabled(true);

	/*
	 * Initially store current affinity settings of this thread using NUMA API
	 */
	if (_affInfo) {
		_affInfo->figureOutCurrentAffinity();
		_affInfo->setCpuOfMine( sched_getcpu() , _settings->value("system/sailaffinity", false).toBool());
	}


	// Actual time elapsed
	struct timeval tvs, tve;

    // CPU time elapsed
    struct timespec ts_start, ts_end;

    if(_perfMon && _settings->value("system/resourcemonitor",false).toBool()) {
        gettimeofday(&tvs, 0);

        // #include <time.h>
        // Link with -lrt
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_start);
	}

	int byteCount = _appInfo->frameSizeInByte();
	unsigned char *bufptr = 0;

	if (!_usePbo) {
		bufptr = static_cast<QImage *>(_doubleBuffer->getFrontBuffer())->bits();
		//bufptr = (unsigned char *)malloc(byteCount);
	}

	while(! _end ) {
		if ( _affInfo && _settings->value("system/resourcemonitor",false).toBool()) {
			if ( _affInfo->isChanged() ) {
				// apply new affinity;
				//	qDebug("SagePixelReceiver::%s() : applying new affinity parameters", __FUNCTION__);
				_affInfo->applyNewParameters(); // use NUMA lib

				// update info in affInfo
				// this function must be called in this thread
				_affInfo->figureOutCurrentAffinity(); // use NUMA lib
			}
			else {
#if defined(Q_OS_LINUX)
				// this is called too many times 
				// if cpu has changed, affinfo::cpuOfMineChanged() will be emitted
				// which is connected to ResourceMonitor::updateAffInfo()

				_affInfo->setCpuOfMine( sched_getcpu() , _settings->value("system/sailaffinity", false).toBool());
#endif
			}
        }

        if (_usePbo) {
            //	qint64 ss,ee;
            //	if (appInfo->GID()==1) {
            //		ss = QDateTime::currentMSecsSinceEpoch();
            //	}
            //
            // if the mutex is currently locked by any thread (includeing this thread), then
            // it will return immediately
            //
            pthread_mutex_lock(_pboMutex);

            //
            // trigger schedulePboUpdate()
            //
            emit frameReceived();

            //
            // unlock the mutex and blocking wait for glMapBufferARB() in schedulePboUpdate()
            //
            pthread_cond_wait(_pboCond, _pboMutex);
            pthread_mutex_unlock(_pboMutex);

            _pboBufIdx = (_pboBufIdx + 1) % 2;

            bufptr = (unsigned char *)_pbobufarray[_pboBufIdx];
            //	qDebug() << "thread woken up" << _pboBufIdx << bufptr;

            //	if (appInfo->GID()==1) {
            //		ee = QDateTime::currentMSecsSinceEpoch();
            //		qDebug() << "thread: condwait : " << ee-ss << "msec";
            //	}
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
//            qDebug() << "sagepixelreceiver::run() : _delay" << _delay << "msec";
            QThread::msleep(_delay);
        }

        ssize_t totalread = 0;
		ssize_t read = 0;

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
			if ( byteCount-totalread < _appInfo->networkUserBufferLength() ) {
				read = recv(_tcpsocket, bufptr, byteCount-totalread ,0 );
			}
			// otherwise, always read groupSize bytes
			else {
				read = recv(_tcpsocket, bufptr, _appInfo->networkUserBufferLength(), 0);
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

        if (!_usePbo) {
			Q_ASSERT(_doubleBuffer);

			// will wait until consumer (SageStreamWidget) consumes the data
			// i.e. until consumer calls doubleBuffer->releaseBackBuffer();
			_doubleBuffer->swapBuffer();
			//qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "swapBuffer returned";

			emit frameReceived(); // Queued Connection. Will trigger SageStreamWidget::updateWidget()

			//
			// getFrontBuffer() will return immediately. There's no mutex waiting in this function
			//
			bufptr = static_cast<QImage *>(_doubleBuffer->getFrontBuffer())->bits(); // bits() will detach
		}

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

		if (_perfMon && _settings->value("system/resourcemonitor",false).toBool()) {
            gettimeofday(&tve, 0);

            qreal actualtime_second = ((double)tve.tv_sec + (double)tve.tv_usec * 1e-6) - ((double)tvs.tv_sec + (double)tvs.tv_usec * 1e-6);
            tvs = tve;

/*
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);

			qreal cputime_second = ((double)ts_end.tv_sec + (double)ts_end.tv_nsec * 1e-9) - ((double)ts_start.tv_sec + (double)ts_start.tv_nsec * 1e-9);
            ts_start = ts_end;
*/

            //
			// calculate delay, fps, cpu usage, everything...
            // perfMon->recvTimer will be restarted in this function.
            // So the delay calculated in this function includes blocking (which is forced by the pixel streamer) delay
            //
			//_perfMon->addToCumulativeByteReceived(read, actualtime_second, cputime_second);
			_perfMon->addToCumulativeByteReceived(read, actualtime_second, 0);
		}
	} /*** end of receiving loop ***/

	/* pixel receiving thread exit */
//	qDebug("SagePixelReceiver : thread exit");
}



