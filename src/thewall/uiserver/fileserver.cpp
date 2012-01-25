#include "fileserver.h"
#include "../sagenextlauncher.h"

#include <QSettings>
#include <QTcpSocket>
#include <QFile>
#include <QDir>

#include <sys/types.h>
#include <sys/socket.h>


FileServerThread::FileServerThread(int sockfd, const quint32 uiclientid, QObject *parent)
    : QThread(parent)
    , _uiclientid(uiclientid)
    , _dataSock(sockfd)
    , _end(false)
{
}

FileServerThread::~FileServerThread() {
	_end = true;
	::close(_dataSock);
	qDebug() << "~FileServerThread" << _uiclientid;
}

void FileServerThread::endThread() {
	_end = true;
	::shutdown(_dataSock, SHUT_RDWR);
	::close(_dataSock);
}


void FileServerThread::run() {
//	qDebug() << "FileServerThread is running for uiclient" << _uiclientid;

	// always receive filename, filesize, media type first
	char header[EXTUI_MSG_SIZE];

	while(!_end) {
		int recv = ::recv(_dataSock, header, EXTUI_MSG_SIZE, MSG_WAITALL);
		if ( recv == -1 ) {
			qDebug("%s::%s() : socket error", metaObject()->className(), __FUNCTION__);
			_end = true;
			break;
		}
		else if (recv == 0) {
			qDebug("%s::%s() : socket disconnected", metaObject()->className(), __FUNCTION__);
			_end = true;
			break;
		}

		int mediatype;
		char filename[256];
		qint64 filesize;
		::sscanf(header, "%d %s %lld", &mediatype, filename, &filesize);
		qDebug() << "FileServerThread received header" << mediatype << filename << filesize << "Byte";

		if (mediatype == SAGENext::MEDIA_TYPE_WEBURL) {
			emit fileReceived(mediatype, QString(filename));
			continue;
		}

		if (filesize <= 0) break;

		QString destdir = QDir::homePath().append("/.sagenext/");

		switch(mediatype) {
		case SAGENext::MEDIA_TYPE_IMAGE : {
			destdir.append("media/image/");
			break;
		}
		case SAGENext::MEDIA_TYPE_LOCAL_VIDEO : {
			destdir.append("media/video/");
			break;
		}
		case SAGENext::MEDIA_TYPE_PDF : {
			destdir.append("media/pdf/");
			break;
		}
		case SAGENext::MEDIA_TYPE_PLUGIN : {
			destdir.append("media/plugins/");
			break;
		}
		} // end switch

		QFile file( destdir.append(QString(filename)) );
		if ( ! file.open(QIODevice::WriteOnly) ) {
			qDebug() << "FileServerThread. file can't be opened for writing";
		}
		qDebug() << "FileServerThread will receive" << file.fileName() << filesize << "Byte";

		QByteArray buffer(filesize, 0);
	    if ( ::recv(_dataSock, buffer.data(), filesize, MSG_WAITALL) <= 0 ) {
	        qCritical("%s::%s() : error while receiving the file.", metaObject()->className(), __FUNCTION__);
	        break;
	    }
		else {
			file.write(buffer);
			if (file.exists() && file.size() > 0)
				emit fileReceived(mediatype, file.fileName());
			else
				qDebug("%s::%s() : %s is not a valid file", metaObject()->className(), __FUNCTION__, qPrintable(file.fileName()));
		}
		file.close();
	}
}















SN_FileServer::SN_FileServer(const QSettings *s, SN_Launcher *l, QObject *parent)
    : QTcpServer(parent)
    , _settings(s)
    , _fileServerPort(0)
    , _launcher(l)
{
	_fileServerPort = _settings->value("general/fileserverport", 46000).toInt();

	if ( ! listen(QHostAddress::Any, _fileServerPort) ) {
        qCritical("FileServer::%s() : listen failed", __FUNCTION__);
        deleteLater();
    }
	else {
		qWarning() << "FileServer has started." << serverAddress() << serverPort();
	}
}

SN_FileServer::~SN_FileServer() {
	close(); // stop listening

	qDebug() << "In ~FileServer(), deleting fileserver threads";
	foreach(FileServerThread *thread, _uiFileServerThreadMap.values()) {
		if (thread) {
			if (thread->isRunning()) {
//				thread->exit();
				thread->endThread();
				thread->wait();
			}

			if(thread->isFinished()) {
				delete thread;
			}
		}
	}

	qDebug() << "~FileServer";
}

void SN_FileServer::incomingConnection(int handle) {

	// receive uiclientid from the client
	quint32 uiclientid = 0;
	char msg[EXTUI_MSG_SIZE];
	if ( ::recv(handle, msg, EXTUI_MSG_SIZE, MSG_WAITALL) < 0 ) {
		qDebug("%s::%s() : error while receiving ", metaObject()->className(), __FUNCTION__);
		return;
	}
	::sscanf(msg, "%u", &uiclientid); // read uiclientid
	qDebug("%s::%s() : The ui client %u has connected to FileServer", metaObject()->className(), __FUNCTION__, uiclientid);


	FileServerThread *thread = new FileServerThread(handle, uiclientid);
	connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));

	connect(thread, SIGNAL(fileReceived(int,QString)), _launcher, SLOT(launch(int,QString)));

	_uiFileServerThreadMap.insert(uiclientid, thread);

	thread->start();
}

void SN_FileServer::threadFinished() {
	foreach(const FileServerThread *thread, _uiFileServerThreadMap.values()) {
		if (thread) {
			if(thread->isFinished()) {
				_uiFileServerThreadMap.erase( _uiFileServerThreadMap.find( thread->uiclientid() ) );
				delete thread;
			}
		}
	}
}
