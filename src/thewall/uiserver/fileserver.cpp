#include "fileserver.h"
#include "../sagenextlauncher.h"

#include <QSettings>
#include <QTcpSocket>
#include <QFile>
#include <QDir>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


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

int FileServerThread::_recvFile(SAGENext::MEDIA_TYPE mediatype, const QString &filename, qint64 filesize) {
	//
	// if it's just web url
	//
	if (mediatype == SAGENext::MEDIA_TYPE_WEBURL) {
		emit fileReceived(mediatype, filename);
		return 0;
	}

	//
	// if it's a real file
	//
	if (filesize <= 0) return -1;

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
	default: {
		qDebug() << "FileServerThread::_recvFile() : unknown media type";
		break;
	}
	} // end switch

	QFile file( destdir.append(filename) );
	if ( ! file.open(QIODevice::WriteOnly) ) {
		qDebug() << "FileServerThread::_recvFile() : file can't be opened for writing";
		return -1;
	}
	qDebug() << "FileServerThread::_recvFile() :  will receive" << file.fileName() << filesize << "Byte";

	QByteArray buffer(filesize, 0);
    if ( ::recv(_dataSock, buffer.data(), filesize, MSG_WAITALL) <= 0 ) {
        qCritical("%s::%s() : error while receiving the file.", metaObject()->className(), __FUNCTION__);
        return -1;
    }

	file.write(buffer);
	if (!file.exists() || file.size() <= 0) {
		qDebug("%s::%s() : %s is not a valid file", metaObject()->className(), __FUNCTION__, qPrintable(file.fileName()));
		return -1;
	}

	emit fileReceived(mediatype, file.fileName());

	file.close();

	return 0;
}

int FileServerThread::_sendFile(const QString &filepath) {
	QFile f(filepath);
	if (! f.open(QIODevice::ReadOnly)) {
		qDebug() << "FileServerThread::_sendFile() : couldn't open the file" << filepath;
		return -1;
	}

	//
	// very inefficient way to send file !!
	//
	if ( ::send(_dataSock, f.readAll().constData(), f.size(), 0) <= 0 ) {
		qDebug() << "FileServerThread::_sendFile() : ::send error";
		return -1;
	}

	f.close();
	return 0;
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

		int mode; // up or down
//		int mediatype;
		SAGENext::MEDIA_TYPE mediatype;
		char filename[256];
		qint64 filesize;
		::sscanf(header, "%d %d %s %lld", &mode, &mediatype, filename, &filesize);



		if (mode == 0) {
			// download from uiclient
			qDebug() << "FileServerThread received the header for downloading" << mediatype << filename << filesize << "Byte";
			if ( _recvFile(mediatype, QString(filename), filesize) != 0) {
				qDebug() << "FileServerThread failed to receive a file" << filename;
			}
		}
		else if (mode == 1) {
			// upload to uiclient
			qDebug() << "FileServerThread received the header for uploading" << filename << filesize << "Byte";

			 // filename must be full absolute path name
			if ( _sendFile(QString(filename)) != 0 ) {
				qDebug() << "FileServerThread failed to send a file" << filename;
			}
		}
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
