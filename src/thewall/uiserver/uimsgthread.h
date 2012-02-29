#ifndef UIMSGTHREAD_H
#define UIMSGTHREAD_H

#include <QThread>
//#include <QTcpSocket>
#include <QHostAddress>

class UiMsgThread : public QThread
{
	Q_OBJECT
public:
	explicit UiMsgThread(const quint32 uiclientid, int sock, QObject *parent = 0);
	~UiMsgThread();

	QHostAddress peerAddress() const {return _peerAddress;}

	inline quint32 getID() const {return _uiClientId;}

protected:
	void run();

private:
	const quint32 _uiClientId;

//	QTcpSocket _tcpSocket;

	QHostAddress _peerAddress;

	bool _end;

	int _sockfd;

signals:
	void msgReceived(const QByteArray msg);

	void clientDisconnected(quint32 uiclientid);

public slots:
	/*!
	  Do note that this function will be running in the caller's thread which can be the main GUI thread (not in a separate thread)
	  */
	void sendMsg(const QByteArray &msgstr);

//	void recvMsg();

	void endThread();

//	void handleSocketError(QAbstractSocket::SocketError);
};

#endif // UIMSGTHREAD_H
