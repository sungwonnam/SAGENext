#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QRegExp>
#include <QUrl>

//#include "externalguimain.h"

class SN_PointerUI_SendThread : public QThread
{
	Q_OBJECT
public:
	explicit SN_PointerUI_SendThread(QObject *parent = 0);
	~SN_PointerUI_SendThread();

	bool setSockFD(int sock);

protected:
	void run();

private:
	QTcpSocket _dataSock;

	QRegExp _rxImage;
	QRegExp _rxVideo;
	QRegExp _rxPdf;
	QRegExp _rxPlugin;

public slots:
	void sendMediaList(const QList<QUrl> urls);
	void sendMedia(const QUrl url);
	void endThread();
	void handleSocketError(QAbstractSocket::SocketError);
};

#endif // SENDTHREAD_H
