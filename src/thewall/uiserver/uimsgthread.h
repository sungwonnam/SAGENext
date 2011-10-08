#ifndef UIMSGTHREAD_H
#define UIMSGTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QHostAddress>

class UiMsgThread : public QThread
{
	Q_OBJECT
public:
	explicit UiMsgThread(const quint32 uiclientid, int sockfd, QObject *parent = 0);
	~UiMsgThread();

	QHostAddress peerAddress() const {return _peerAddress;}

protected:
	void run();

private:
	const quint32 _uiClientId;

	QTcpSocket _tcpSocket;

	QHostAddress _peerAddress;


	bool _end;

signals:
	void msgReceived(const QByteArray msg);

	void clientDisconnected(quint32 uiclientid);

public slots:
	void sendMsg(const QByteArray &msgstr);
	void recvMsg();

	void endThread();

	void handleSocketError(QAbstractSocket::SocketError);
};

#endif // UIMSGTHREAD_H
