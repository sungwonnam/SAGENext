#include "sagepixelreceiver.h"

#include "sagestreamwidget.h"
#include "railawarewidget.h"
#include "appinfo.h"
#include "perfmonitor.h"
#include "affinityinfo.h"

#include "../../common/imagedoublebuffer.h"
#include "../../sage/fsmanagermsgthread.h"

#include <QTcpSocket>
#include <QUdpSocket>
#include <QSemaphore>

#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
//#include <sched.h>
#include <unistd.h>


SN_SagePixelReceiver::SN_SagePixelReceiver(int protocol, int sockfd, DoubleBuffer *idb, bool usepbo, void **pbobufarray, pthread_mutex_t *pboMutex, pthread_cond_t *pboCond, SN_SageStreamWidget *sw, /*RailawareWidget *rw, QMutex *mmm, QWaitCondition *wwcc,*/ const QSettings *s, QObject *parent /* 0 */)
    : QThread(parent)
    , _settings(s)
    , _sageWidget(sw)
    , _end(false)

    , _tcpsocket(sockfd)
    , _udpsocket(0)

    , _delay(0)

    , _doubleBuffer(idb)

    , _appInfo(sw->appInfo())
    , _perfMon(sw->perfMon())
    , _affInfo(sw->affInfo())

    , _usePbo(usepbo)
    , _pboBufIdx(0)
    , _pbobufarray(pbobufarray)
    , _pboMutex(pboMutex)
    , _pboCond(pboCond)
    , _isRMonitor(false)
    , _isScheduler(false)
{
	QThread::setTerminationEnabled(true);

    int optVal = _settings->value("network/recvwindow", 4 * 1048576).toInt();
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

    _isRMonitor = _settings->value("system/resourcemonitor", false).toBool();
    _isScheduler = _settings->value("system/scheduler", false).toBool();
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

    //
	// Actual time elapsed
    // This includes the extra delay the scheduler demands
    //
	struct timeval tvs, tve;

    // CPU time elapsed
//    struct timespec ts_start, ts_end;

    if(_perfMon && _isRMonitor) {
        gettimeofday(&tvs, 0);

        // #include <time.h>
        // Link with -lrt
//        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_start);
	}

	int byteCount = _appInfo->frameSizeInByte();
	unsigned char *bufptr = 0;

	int userBufferLength = _appInfo->networkUserBufferLength();

	if (!_usePbo) {
		bufptr = static_cast<QImage *>(_doubleBuffer->getFrontBuffer())->bits();
		//bufptr = (unsigned char *)malloc(byteCount);
	}

    //
    // recvDelay is the time took to iterate the while(!_end) loop once.
    // This is used to calculate how much more delay is needed to obey the scheduler's demand.
    //
    qint64 recvDelay = 0;
    qint64 start;


    if (_perfMon) {
        _perfMon->setMeasureStartTime(QDateTime::currentMSecsSinceEpoch());
    }

	while(! _end ) {

        if (_isScheduler)
            start = QDateTime::currentMSecsSinceEpoch();

		if ( _affInfo && _isRMonitor) {
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
            pthread_mutex_lock(_pboMutex);

            //
            // This signal invokes the schedulePboUpdate()
            //
            emit frameReceived();

            //
            // unlock the mutex and blocking wait for glMapBufferARB() is done in schedulePboUpdate()
            //
            pthread_cond_wait(_pboCond, _pboMutex);
            pthread_mutex_unlock(_pboMutex);

            _pboBufIdx = (_pboBufIdx + 1) % 2;

            bufptr = (unsigned char *)_pbobufarray[_pboBufIdx];
            if (!bufptr) continue;
        }
        else {
            Q_ASSERT(_doubleBuffer);

            //
			// This will wait until consumer (SageStreamWidget) consumes the data if the queueLength > 0
			// (i.e. until consumer calls doubleBuffer->releaseBackBuffer())
            // Otherwise, it will increase the queueLength and do pthread_cond_signal(notEmpty)
            //
			_doubleBuffer->swapBuffer();
			//qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "swapBuffer returned";

			emit frameReceived(); // Queued Connection. Will trigger SageStreamWidget::updateWidget()

			//
			// getFrontBuffer() will return immediately. There's no mutex waiting in this function
			//
			bufptr = static_cast<QImage *>(_doubleBuffer->getFrontBuffer())->bits(); // bits() will detach
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

        if (_sageWidget->__sema) {
            _sageWidget->__sema->acquire(1);
        }

        //
		// PIXEL RECEIVING
        //
        ssize_t totalread = 0;
        ssize_t read = 0;
        while (totalread < byteCount ) {
            // If remaining byte is smaller than user buffer length (which is groupSize)
            if ( byteCount-totalread < userBufferLength ) {
                read = recv(_tcpsocket, bufptr, byteCount-totalread , MSG_WAITALL);
            }
            // otherwise, always read groupSize bytes
            else {
                read = recv(_tcpsocket, bufptr, userBufferLength, MSG_WAITALL);
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


		if (_perfMon && _isRMonitor) {
            gettimeofday(&tve, 0);

            //
            // This includes the extra delay the scheduler demands
            // So it's effective per frame delay (effective B/W and FPS)
            //
            qreal effectiveDelay_second = ((double)tve.tv_sec + (double)tve.tv_usec * 1e-6) - ((double)tvs.tv_sec + (double)tvs.tv_usec * 1e-6);
            tvs = tve;

/*
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);

			qreal cputime_second = ((double)ts_end.tv_sec + (double)ts_end.tv_nsec * 1e-9) - ((double)ts_start.tv_sec + (double)ts_start.tv_nsec * 1e-9);
            ts_start = ts_end;
*/

            //
			// calculate the effective delay, fps, cpu usage, everything...
            // So the delay calculated in this function includes all the delays
            //
			//_perfMon->addToCumulativeByteReceived(read, actualtime_second, cputime_second);
			_perfMon->addToCumulativeByteReceived(totalread, effectiveDelay_second, 0);
		}

        //
        // If the scheduelr demands specific quality
        //
        if (_isScheduler) {
            recvDelay = QDateTime::currentMSecsSinceEpoch() - start;

            if (_delay > 0  &&  _delay > recvDelay) {
//                qDebug() << "run() : widget" << _sageWidget->globalAppId() << "demanded delay" << _delay << ". so delaying" << _delay-recvDelay << "msec more";
                QThread::msleep( _delay - recvDelay );
            }
            else if (_delay < 0) {

                while (_delay < 0) {
                    QThread::msleep(500);
                }
            }
        }

	} /*** end of receiving loop ***/

    if (_perfMon) {
        _perfMon->setMeasureEndTime(QDateTime::currentMSecsSinceEpoch());
    }

	/* pixel receiving thread exit */
//	qDebug("SagePixelReceiver : thread exit");
}

