#ifndef FILESERVER_H
#define FILESERVER_H

#include <QTcpServer>
#include <QThread>

#include "../common/commondefinitions.h"

#include "uiserver.h"

class QSettings;
class SN_Launcher;
class SN_MediaStorage;

class FileServerThread : public QThread
{
	Q_OBJECT
public:
	explicit FileServerThread(int sockfd, const quint32 uiclientid, QObject *parent=0);
	~FileServerThread();

	inline quint32 uiclientid() const {return _uiclientid;}

protected:
	/**
	  starts the event loop
	  */
	void run();

private:
	const quint32 _uiclientid;

//	QTcpSocket _dataSock;

	int _dataSock;

	bool _end;

	int _recvFile(SAGENext::MEDIA_TYPE mediatype, const QString &filename, qint64 filesize);

	int _sendFile(const QString &filepath);

signals:
	void fileReceived(int mediatype, QString filename);

    void bytesWrittenToFile(qint32 uiclientid, QString filename, qint64 bytes);

public slots:
	void endThread();
};





class SN_FileServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit SN_FileServer(const QSettings *s, SN_Launcher *l, SN_UiServer *uiserver, SN_MediaStorage* ms, QObject *parent = 0);
	~SN_FileServer();

	inline int fileServerListenPort() const {return _fileServerPort;}

protected:
#ifdef QT5
	void incomingConnection(qintptr sockfd);
#else
    void incomingConnection(int sockfd);
#endif

private:
	const QSettings *_settings;

	QMap<quint32, FileServerThread *> _uiFileServerThreadMap;

	int _fileServerPort;

	SN_Launcher *_launcher;

    SN_UiServer *_uiServer;

    SN_MediaStorage* _mediaStorage;

signals:

public slots:
	/**
	  update _uiFileServerThreadMap
	  */
	void threadFinished();

    void sendRecvProgress(qint32 uiclientid, QString filename, qint64 bytes);
};

#endif // FILESERVER_H
