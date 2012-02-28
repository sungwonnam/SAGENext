#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QtNetwork>
#include <QtCore>

typedef struct {
	int mediatype;
	char filename[256];
	qint64 filesize;
	char filepath[1024];
} MediaItemInfo;

class SN_PointerUI_DataThread : public QThread
{
	Q_OBJECT
public:
	explicit SN_PointerUI_DataThread(const QHostAddress &hostaddr, quint16 port, QObject *parent = 0);
	~SN_PointerUI_DataThread();

	inline void setUiClientID(quint32 id) {_uiclientid = id;}

protected:

	/*!
	  Only this function will run in a separate thread.
	  If you define QTcpSocket as a member variable of this class, then the socket object will reside in the thread who created this class !
	  */
	void run();

private:
	QHostAddress _hostaddr;
	quint16 _port;

	quint32 _uiclientid;

	/*!
	  This will reside in the thread who created this class. (Which means the main thread)
	  So I can't use this socket in a separate thread !
	  */
	QTcpSocket _dataSock;

	QRegExp _rxImage;
	QRegExp _rxVideo;
	QRegExp _rxPdf;
	QRegExp _rxPlugin;

	QList<MediaItemInfo> _sendList;

	QMutex _mutex;

	void enqueueMedia(const QUrl url);



signals:
	void connected();
	void fileReadyToSend();

private slots:
	void sendData(QTcpSocket &sock);

public slots:
	void recvData();

	void prepareMediaList(const QList<QUrl> urls);

	void endThread();
	void handleSocketError(QAbstractSocket::SocketError);

	void receiveMedia(const QString &medianame, int mediafilesize);
};

#endif // SENDTHREAD_H
