#include "fittslawteststreamrecvthread.h"

#include "applications/base/perfmonitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

FittsLawTestStreamReceiverThread::FittsLawTestStreamReceiverThread(const QSize &imgsize, qreal frate,  const QString &streamerip, int tcpport, PerfMonitor *pmon, QSemaphore *sm, QObject *parent)
    : QThread(parent)
    , _framerate(frate)
    , _imgSize(imgsize)
    , _streamerIpaddr(streamerip)
    , _tcpPort(tcpport)
    , _socket(0)
    , _end(false)
    , _perfMon(pmon)
    , _demandedDelay(0)
    , _sema(sm)
{
    _socket = ::socket(AF_INET, SOCK_STREAM, 0);
    Q_ASSERT(_socket);
}

FittsLawTestStreamReceiverThread::~FittsLawTestStreamReceiverThread() {
    _end = true;
//    ::shutdown(_socket, SHUT_RDWR);
    ::close(_socket);
    _sema->release();
    wait();

    qDebug() << "FittsLawTestStreamReceiverThread::~FittsLawTestStreamReceiverThread()";
}

void FittsLawTestStreamReceiverThread::run() {
qDebug() << "recvThred::run()";

    Q_ASSERT(_socket);

    QByteArray buffer(_imgSize.width() * _imgSize.height() * 3, 0); // 3 byte per pixel

    // Actual time elapsed
	struct timeval tvs, tve;

    // CPU time elapsed
//    struct timespec ts_start, ts_end;

    if(_perfMon) {
        gettimeofday(&tvs, 0);

        // #include <time.h>
        // Link with -lrt
//        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_start);
	}

    qint64 timestamp = 0;

    while (!_end) {

        timestamp = QDateTime::currentMSecsSinceEpoch();

        /*
        if (s->value("system/scheduler",false).toBool()) {
//            qreal adjustment = (1.0 / perf->getAdjustedFps()) - (1.0 / perf->getExpetctedFps()); // in second
            qreal adjustment_msec = 1000.0 * (1.0/perf->getAdjustedFps());

            if ( adjustment_msec > 0 ) {
                // adding delay to respect the adjusted quality
                QThread::msleep( (unsigned long)adjustment_msec );
            }
        }
        */

        //
        // recv() only when mouse moved
        //
        _sema->acquire(2);

//		qDebug() << "will recv()"  << buffer.size() << "Byte";
		//
		// signal the streamer
		//
		int sent = ::send(_socket, buffer.data(), 1024, 0);
		if (sent <= 0) {
			qDebug() << "FittsLawTestStreamReceiverThread::run() : send() error while signal the streamer !!";
			_end = true;
			break;
		}

		ssize_t bytesRead = 0;
	    int actualRead = 0;
        int buflen = 65536;
        char *bufptr = buffer.data();

	    while (bytesRead < buffer.size()) {

            if ( buffer.size() - bytesRead < buflen ) {
                actualRead = ::recv(_socket, (void *)bufptr, buffer.size()-bytesRead, MSG_WAITALL);
            }
            else {
                actualRead = ::recv(_socket, (void *)bufptr, buflen, MSG_WAITALL);
            }


		   if (actualRead < 0) {
			  qCritical("recv() - error in recv() system call");
//			  qCritical("=== received data: %d bytes, remaining %d bytes", bytesRead, bytes);
			  _end = true;
			  break;
		   }
		   else if (actualRead == 0) {
			  qCritical("recv() : connection shutdown");
			  _end = true;
			  break;
		   }

           bufptr += actualRead;
           bytesRead += actualRead;
	    }

        emit frameReceived();

        if (_perfMon) {
            gettimeofday(&tve, 0);

            qreal actualtime_second = ((double)tve.tv_sec + (double)tve.tv_usec * 1e-6) - ((double)tvs.tv_sec + (double)tvs.tv_usec * 1e-6);
            tvs = tve;

//            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);

//			qreal cputime_second = ((double)ts_end.tv_sec + (double)ts_end.tv_nsec * 1e-9) - ((double)ts_start.tv_sec + (double)ts_start.tv_nsec * 1e-9);
//            ts_start = ts_end;


            //
			// calculate delay, fps, cpu usage, everything...
            // perfMon->recvTimer will be restarted in this function.
            // So the delay calculated in this function includes blocking (which is forced by the pixel streamer) delay
            //
			_perfMon->addToCumulativeByteReceived(bytesRead, actualtime_second, 0);
		}

        qint64 delay = QDateTime::currentMSecsSinceEpoch() - timestamp;
        if (_demandedDelay > 0  &&  _demandedDelay - delay > 0) {
            qDebug() << "run() : " << _demandedDelay << delay << _demandedDelay - delay;
            QThread::msleep(_demandedDelay - delay);
        }

    }

    qDebug() << "FittsLawTestStreamReceiverThread::run() : thread finished";
}

void FittsLawTestStreamReceiverThread::endThreadLoop() {
    _end = true;
}

void FittsLawTestStreamReceiverThread::resumeThreadLoop() {
    _end = false;
    start();
}

bool FittsLawTestStreamReceiverThread::connectToStreamer() {

    if (_tcpPort <= 0) {
        qCritical() << "FittsLawTestStreamReceiverThread::acceptStreamerConnection() : Invalid port number" << _tcpPort;
        return false;
    }

//    QHostAddress addr(_streamerIpaddr);
//    if (addr.isNull()) {
//        qCritical() << "StreamRecvThread::acceptStreamerConnection() : Invalid IP addr" << addr.toString();
//        return false;
//    }

    /*
    // note that an eventloop must be running
   _tcpSocketObj.connectToHost(addr, _tcpPort);

   // will block
   return _tcpSocketObj.waitForConnected(-1);
   */

    struct sockaddr_in streamerAddr;
    memset(&streamerAddr, 0, sizeof(streamerAddr));
    streamerAddr.sin_family = AF_INET;
    inet_pton(AF_INET, qPrintable(_streamerIpaddr), &streamerAddr.sin_addr.s_addr);
    streamerAddr.sin_port = htons(_tcpPort);

    if ( ::connect(_socket, (sockaddr *)&streamerAddr, sizeof(streamerAddr)) == -1 ) {
        qCritical() << "FittsLawTestStreamReceiverThread::connectToStreamer() : connect() failed";
        perror("connect");
        return false;
    }
    else {
		qDebug() << "recvThread connected to the streamer" << _streamerIpaddr << _tcpPort;
    }

    return true;
}















































/**************************

  QObject
  ************************/


FittsLawTestStreamReceiver::FittsLawTestStreamReceiver(const QSize &imgsize, const QString &streamerip, int tcpport, PerfMonitor *pmon, QObject *parent)
    : QObject(parent)
    , _imgsize(imgsize)
    , _streamerIp(streamerip)
    , _streamerPort(tcpport)
    , _perfMon(pmon)
    , _cumulativeByteReceived(0)
    , _cumulativeByteReceivedPrev(0)
{
    QObject::connect(&_tcpSock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));
}

void FittsLawTestStreamReceiver::handleSocketError(QAbstractSocket::SocketError err) {
    qDebug() << "FittsLawTestStreamReceiver::handleSocketError() :" << err;
}

FittsLawTestStreamReceiver::~FittsLawTestStreamReceiver() {
    _tcpSock.close();
}


void FittsLawTestStreamReceiver::connectToStreamer() {
    qDebug() << "FittsLawTestStreamReceiver::connectToStreamer() : " << _streamerIp << _streamerPort;
    _tcpSock.connectToHost(_streamerIp, _streamerPort);

    if (_perfMon) {
        gettimeofday(&_tvs, 0);

        // #include <time.h>
        // Link with -lrt
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &_ts_start);
    }

    if (_tcpSock.waitForConnected(-1)) {
        qDebug() << "FittsLawTestStreamReceiver::connectToStreamer() : Connected to streamer !";

    }
    else {
        qDebug() << "FittsLawTestStreamReceiver::connectToStreamer() : Failed to Connect !";
    }
}

void FittsLawTestStreamReceiver::recvFrame() {
    if (_tcpSock.state() == QAbstractSocket::ConnectedState) {

        QByteArray buffer(_imgsize.width() * _imgsize.height() * 3, 0); // 3 byte per pixel

        qint64 read = _tcpSock.read(buffer.data(), buffer.size());


        if (read == buffer.size()) {
            qDebug() << "FittsLawTestStreamReceiver::recvFrame() : frame received";
            emit frameReceived();

            if (_perfMon) {

                gettimeofday(&_tve, 0);

                qreal actualtime_second = ((double)_tve.tv_sec + (double)_tve.tv_usec * 1e-6) - ((double)_tvs.tv_sec + (double)_tvs.tv_usec * 1e-6);
                _tvs = _tve;

                clock_gettime(CLOCK_THREAD_CPUTIME_ID, &_ts_end);

                qreal cputime_second = ((double)_ts_end.tv_sec + (double)_ts_end.tv_nsec * 1e-9) - ((double)_ts_start.tv_sec + (double)_ts_start.tv_nsec * 1e-9);
                _ts_start = _ts_end;

                _perfMon->addToCumulativeByteReceived(read, actualtime_second, cputime_second);

//                qDebug() << "current BW" << _perfMon->getCurrBW_Mbps() << "Mbps" << _perfMon->getCurrEffectiveFps();
            }
        }
    }
    else {
        qDebug() << "FittsLawTestStreamReceiver::recvFrame() : not connected";
    }
}
