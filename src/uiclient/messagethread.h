#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QThread>
//#include <QHostAddress>
#include <QFileInfo>

class MessageThread : public QThread
{
        Q_OBJECT
public:
        /*!
          This thread represents connection b/w uiclient and wall.
          uiclientid is determined by the wall and sent to uiclient.
          To discriminate specific wall from multiple walls, sockfd must be used instead of uiclientid, because uiclientid is unique in the wall not in the ui client.
          */
        explicit MessageThread(int socket, const quint64 uiid, const QString &myip, QObject *parent = 0);
        ~MessageThread();

protected:
        void run();

private:
        /*!
          This socket descriptor can be used to identify a wall since sockfd is unique in this process
          */
        int sockfd;

        /*!
          externalguimain and messagethread both can alter this value so it's volatile
          */
        volatile bool end;

        QString filen;

        /*!
          Unique identifier of this ui client.
          Note that uiclientid is unique ONLY within the wall represented by sockfd.
          It is absolutely valid and likely that multiple message threads have same uiclientid value
          */
        const quint64 uiclientid;

        QString myipaddr;

signals:
        void ackReceived(const QString &ipaddr, int port);
        void newAppLayout(QByteArray msg);

public slots:
        void endThread();
//	void requestAppLayout();
        void registerApp(int mediatype, const QString &filename);
//	void sendFile(const QHostAddres &addr, int port);
        void sendMsg(const QByteArray &msg);
};

#endif // MESSAGETHREAD_H
