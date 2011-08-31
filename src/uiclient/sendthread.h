#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QHostAddress>

class SendThread : public QThread
{
	Q_OBJECT
public:
	explicit SendThread(QObject *parent = 0);
	~SendThread();

protected:
	void run();

private:
	bool end;
	QMutex mutex;
	QWaitCondition fileReady;
	QQueue<QString> fileQueue;
	QHostAddress receiverAddr;
	int receiverPort;

signals:

public slots:
	void sendFile(const QHostAddress &addr, int port, const QString &filename);

};

#endif // SENDTHREAD_H
