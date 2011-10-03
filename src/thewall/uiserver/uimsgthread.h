#ifndef UIMSGTHREAD_H
#define UIMSGTHREAD_H

#include <QThread>
#include <QTcpSocket>

class UiMsgThread : public QThread
{
	Q_OBJECT
public:
	explicit UiMsgThread(const quint32 uiclientid, int sockfd, QObject *parent = 0);
	~UiMsgThread();

protected:
	void run();

private:
	const quint32 uiClientId;
	bool _end;
	int sockfd;
//	QTcpSocket *tcpSock;

signals:
	void msgReceived(quint32 myId, UiMsgThread *myself, QByteArray msg);
	void clientDisconnected(quint32 uiclientid);

public slots:
	void breakWhileLoop();
	void sendMsg(const QByteArray &msgstr);
};

#endif // UIMSGTHREAD_H
