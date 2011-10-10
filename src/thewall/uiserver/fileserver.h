#ifndef FILESERVER_H
#define FILESERVER_H

#include <QTcpServer>
#include <QThread>

#include "../common/commondefinitions.h"

#include "uiserver.h"

class QSettings;
class SN_Launcher;

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

signals:
	void fileReceived(int mediatype, QString filename);

public slots:
	void endThread();
};





class SN_FileServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit SN_FileServer(const QSettings *s, SN_Launcher *l, QObject *parent = 0);
	~SN_FileServer();

	inline int fileServerListenPort() const {return _fileServerPort;}

protected:
	void incomingConnection(int handle);

private:
	const QSettings *_settings;

	QMap<quint32, FileServerThread *> _uiFileServerThreadMap;

	int _fileServerPort;

	SN_Launcher *_launcher;

signals:

public slots:
	/**
	  update _uiFileServerThreadMap
	  */
	void threadFinished();

};

#endif // FILESERVER_H
