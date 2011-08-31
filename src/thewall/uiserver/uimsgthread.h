#ifndef UIMSGTHREAD_H
#define UIMSGTHREAD_H

#include <QThread>
#include <QTcpSocket>

class UiMsgThread : public QThread
{
	Q_OBJECT
public:
	explicit UiMsgThread(const quint64 uiclientid, int sockfd, QObject *parent = 0);
	~UiMsgThread();

protected:
	void run();

private:
	const quint64 uiClientId;
	bool _end;
	int sockfd;
//	QTcpSocket *tcpSock;

signals:
	void msgReceived(quint64 myId, UiMsgThread *myself, QByteArray msg);
	void clientDisconnected(quint64 uiclientid);

public slots:
	void breakWhileLoop();
	void sendMsg(const QByteArray &msgstr);
};

#endif // UIMSGTHREAD_H
