#include "fittslawteststreamrecvthread.h"

#include "applications/base/perfmonitor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

FittsLawTestStreamReceiver::FittsLawTestStreamReceiver(const QSize &imgsize, qreal frate,  const QString &streamerip, int tcpport, PerfMonitor *pmon, QObject *parent)
    : QThread(parent)
    , _framerate(frate)
    , _imgSize(imgsize)
    , _streamerIpaddr(streamerip)
    , _tcpPort(tcpport)
    , _socket(0)
    , _end(false)
    , _perfMon(pmon)
{
    _socket = ::socket(AF_INET, SOCK_STREAM, 0);
    Q_ASSERT(_socket);
}

FittsLawTestStreamReceiver::~FittsLawTestStreamReceiver() {
    _end = true;
//    ::shutdown(_socket, SHUT_RDWR);
    ::close(_socket);
    wait();

    qDebug() << "StreamReceiver::~StreamReceiver()";
}

void FittsLawTestStreamReceiver::run() {

    Q_ASSERT(_socket);

    QByteArray buffer(_imgSize.width() * _imgSize.height() * 3, 0); // 3 byte per pixel


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

        int byterecv = ::recv(_socket, buffer.data(), buffer.size(), MSG_WAITALL);
        if (byterecv == -1) {
            break;
        }
        else if (byterecv == 0) {
            break;
        }

        emit frameReceived();


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
    }

    qDebug() << "StreamReceiver::run() : thread finished";
}

void FittsLawTestStreamReceiver::endThreadLoop() {
    _end = true;
}

void FittsLawTestStreamReceiver::resumeThreadLoop() {
    _end = false;
    start();
}

bool FittsLawTestStreamReceiver::connectToStreamer() {

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
