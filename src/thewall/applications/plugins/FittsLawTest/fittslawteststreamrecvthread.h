#ifndef STREAMRECVTHREAD_H
#define STREAMRECVTHREAD_H

#include <QtGui>

class PerfMonitor;

/*!
  Inherits QThread
  */
class FittsLawTestStreamReceiverThread : public QThread
{
    Q_OBJECT
public:
    explicit FittsLawTestStreamReceiverThread(const QSize &imgsize, qreal frate, const QString &streamerip, int tcpport, PerfMonitor *pmon, QObject *parent = 0);
    ~FittsLawTestStreamReceiverThread();

protected:
    void run();

private:
    qreal _framerate;

    QSize _imgSize;

    QString _streamerIpaddr;

    int _tcpPort;

    int _socket;

    bool _end;

    PerfMonitor *_perfMon;

signals:
    /*!
      Upon receiving a complete image (a frame),
      FittsLawTest will call update() (which will trigger paint())
      */
    void frameReceived();
    
public slots:
    /*!
      Thread does nothing (not consuming resource)
      until a user initiates the test (clicking the green circle)
      */
    void endThreadLoop();

    /*!
      startRound() (upon clicking the green circle) will resume streaming
      */
    void resumeThreadLoop();

    /*!
      receives images from socket in a forever loop
      */
//    void recvImage();
    
    /*!
      blocking wait ( ::accept() )
      */
    bool connectToStreamer();
};







#include <QTcpSocket>


class FittsLawTestStreamReceiver : public QObject
{
    Q_OBJECT

public:
    FittsLawTestStreamReceiver(const QSize &imgsize, const QString &streamerip, int tcpport, PerfMonitor *pmon, QObject *parent = 0);
    ~FittsLawTestStreamReceiver();

protected:
    void timerEvent(QTimerEvent *e);

private:
    QSize _imgsize;

    QString _streamerIp;

    int _streamerPort;

    PerfMonitor *_perfMon;

    QTcpSocket _tcpSock;

    int _timerId;

    qint64 _cumulativeByteReceived;

    qint64 _cumulativeByteReceivedPrev;


    /*!
      actual time between frames
      */
    struct timeval _tvs;
    struct timeval _tve;

    /*!
      cpu usage
      */
    struct timespec _ts_start;
    struct timespec _ts_end;

signals:
    void frameReceived();

public slots:
    void connectToStreamer();

    void recvFrame();

    void measurePerf();

    void handleSocketError(QAbstractSocket::SocketError err);
};

#endif // STREAMRECVTHREAD_H
