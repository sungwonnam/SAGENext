#ifndef MESSAGETHREAD_H
#define MESSAGETHREAD_H

#include <QThread>
//#include <QHostAddress>
//#include <QFileInfo>
#include <QTcpSocket>
#include <QHostAddress>

class SN_PointerUI;

class SN_MsgObject : public QObject
{
	Q_OBJECT
public:
	/*!
	  This thread represents connection b/w uiclient and wall.
	  uiclientid is determined by the wall and sent to uiclient.
	  To discriminate specific wall from multiple walls, sockfd must be used instead of uiclientid, because uiclientid is unique in the wall not in the ui client.
	*/
	explicit SN_MsgObject(const QHostAddress &hostaddr, quint16 port, QObject *parent = 0);
	~SN_MsgObject();
	

private:
	QHostAddress _hostaddr;

	quint16 _port;

	QTcpSocket _tcpMsgSock;


        /*!
          Unique identifier of this ui client.
          Note that uiclientid is unique ONLY within the wall represented by sockfd.
          It is absolutely valid and likely that multiple message threads have same uiclientid value
          */
	quint64 _uiclientid;

//	QString myipaddr;

signals:
	void ackReceived(const QString &ipaddr, int port);
//	void newAppLayout(QByteArray msg);

	void connectedToTheWall(quint32 uiclientid, int wallwidth, int wallheight, int ftpport);

public slots:
	void connectToTheWall();
//	void requestAppLayout();
//	void registerApp(int mediatype, const QString &filename);
//	void sendFile(const QHostAddres &addr, int port);
	void sendMsg(const QByteArray msg);
	void recvMsg();
	void handleSocketError(QAbstractSocket::SocketError error);
};

#endif // MESSAGETHREAD_H
