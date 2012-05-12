#include "fittslawteststreamrecvthread.h"

#include "applications/base/perfmonitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

FittsLawTestStreamReceiverThread::FittsLawTestStreamReceiverThread(const QSize &imgsize, qreal frate,  const QString &streamerip, int tcpport, PerfMonitor *pmon, QSemaphore *sm, QObject *parent)
    : QThread(parent)
    , _framerate(frate)
    , _imgSize(imgsize)
    , _streamerIpaddr(streamerip)
    , _tcpPort(tcpport)
    , _socket(0)
    , _end(false)
    , _perfMon(pmon)
    , _extraDelay(0)
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

    qDebug() << "StreamReceiver::~StreamReceiver()";
}

void FittsLawTestStreamReceiverThread::run() {

    Q_ASSERT(_socket);

    QByteArray buffer(_imgSize.width() * _imgSize.height() * 3, 0); // 3 byte per pixel

    /*
    // Actual time elapsed
	struct timeval tvs, tve;

    // CPU time elapsed
    struct timespec ts_start, ts_end;

    if(_perfMon) {
//		perf->getRecvTimer().start(); //QTime::start()

        gettimeofday(&tvs, 0);

        // #include <time.h>
        // Link with -lrt
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_start);
	}
*/

    ssize_t byterecv = 0;

    while (!_end) {

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



        _sema->acquire(1);


        if (_extraDelay > 0) {
            //
            // This assumes the recv() latency below is neglible
            // in calculating real FPS
            //
            QThread::msleep(_extraDelay);
        }



        byterecv = ::recv(_socket, buffer.data(), buffer.size(), MSG_WAITALL);
        if (byterecv == -1) {
            break;
        }
        else if (byterecv == 0) {
            break;
        }

        emit frameReceived();

        if (_perfMon) {
            _perfMon->addToCumulativeByteReceived(byterecv);
        }

        /*
        if (_perfMon) {
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
			_perfMon->updateDataWithLatencies(byterecv, actualtime_second, cputime_second);
		}
        */
    }

    qDebug() << "StreamReceiver::run() : thread finished";
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
        qCritical() << "StreamReceiver::acceptStreamerConnection() : Invalid port number" << _tcpPort;
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
    streamerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    streamerAddr.sin_port = htons(_tcpPort);

    if ( ::connect(_socket, (sockaddr *)&streamerAddr, sizeof(streamerAddr)) == -1 ) {
        qCritical() << "StreamReceiver::connectToStreamer() : connect() failed";
        return false;
    }

    return true;
}























































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
