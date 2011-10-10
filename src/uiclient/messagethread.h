#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QThread>
//#include <QHostAddress>
//#include <QFileInfo>
#include <QTcpSocket>

class SN_PointerUI_MsgThread : public QThread
{
	Q_OBJECT
public:
	/*!
	  This thread represents connection b/w uiclient and wall.
	  uiclientid is determined by the wall and sent to uiclient.
	  To discriminate specific wall from multiple walls, sockfd must be used instead of uiclientid, because uiclientid is unique in the wall not in the ui client.
	*/
	explicit SN_PointerUI_MsgThread(QObject *parent = 0);
	~SN_PointerUI_MsgThread();
	
	bool setSocketFD(int s);
	
	inline void setUiClientId(quint64 i) {uiclientid = i;}
	
//	inline void setMyIpAddr(const QString ip) {myipaddr = ip;}

protected:
	void run();

private:
        /*!
          This socket descriptor can be used to identify a wall since sockfd is unique in this process
          */
	int sockfd;

	QTcpSocket _tcpMsgSock;

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
	quint64 uiclientid;

//	QString myipaddr;

signals:
	void ackReceived(const QString &ipaddr, int port);
//	void newAppLayout(QByteArray msg);

public slots:
	void endThread();
//	void requestAppLayout();
//	void registerApp(int mediatype, const QString &filename);
//	void sendFile(const QHostAddres &addr, int port);
	void sendMsg(const QByteArray msg);
	void recvMsg();
	void handleSocketError(QAbstractSocket::SocketError error);
};

#endif // MESSAGETHREAD_H
