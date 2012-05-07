#ifndef STREAMRECVTHREAD_H
#define STREAMRECVTHREAD_H

#include <QtGui>

class PerfMonitor;

class FittsLawTestStreamReceiver : public QThread
{
    Q_OBJECT
public:
    explicit FittsLawTestStreamReceiver(const QSize &imgsize, qreal frate, const QString &streamerip, int tcpport, PerfMonitor *pmon, QObject *parent = 0);
    ~FittsLawTestStreamReceiver();

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

#endif // STREAMRECVTHREAD_H
