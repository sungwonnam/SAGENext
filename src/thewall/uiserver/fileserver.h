#ifndef FILESERVER_H
#define FILESERVER_H

#include <QTcpServer>
#include <QThread>

#include "../common/commondefinitions.h"

#define EXTUI_MSG_SIZE 1280

class QSettings;
class SAGENextLauncher;

class FileServerThread : public QThread
{
	Q_OBJECT
public:
	explicit FileServerThread(int sockfd, const quint64 uiclientid, QObject *parent=0);
	~FileServerThread();

	inline quint64 uiclientid() const {return _uiclientid;}

protected:
	/**
	  starts the event loop
	  */
	void run();

private:
	const quint64 _uiclientid;

//	QTcpSocket _dataSock;

	int _dataSock;

	bool _end;

signals:
	void fileReceived(int mediatype, QString filename);

public slots:
	void endThread();
};





class FileServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit FileServer(const QSettings *s, SAGENextLauncher *l, QObject *parent = 0);
	~FileServer();

protected:
	void incomingConnection(int handle);

private:
	const QSettings *_settings;

	QMap<quint64, FileServerThread *> _uiFileServerThreadMap;

	int _fileServerPort;

	SAGENextLauncher *_launcher;

signals:

public slots:
	/**
	  update _uiFileServerThreadMap
	  */
	void threadFinished();

};

#endif // FILESERVER_H
