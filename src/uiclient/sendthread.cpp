#include "sendthread.h"
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
*/
#include <QFileInfo>

#include "externalguimain.h"

SendThread::SendThread(QObject *parent)
    : QThread(parent)
{
	_rxImage.setCaseSensitivity(Qt::CaseInsensitive);
	_rxVideo.setCaseSensitivity(Qt::CaseInsensitive);
	_rxPdf.setCaseSensitivity(Qt::CaseInsensitive);
	_rxPlugin.setCaseSensitivity(Qt::CaseInsensitive);

	_rxImage.setPatternSyntax(QRegExp::RegExp);
	_rxVideo.setPatternSyntax(QRegExp::RegExp);
	_rxPdf.setPatternSyntax(QRegExp::RegExp);
	_rxPlugin.setPatternSyntax(QRegExp::RegExp);

	_rxImage.setPattern("(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$");
	_rxVideo.setPattern("(avi|mov|mpg|mpeg|mp4|mkv|flv|wmv)$");
	_rxPdf.setPattern("(pdf)$");
	_rxPlugin.setPattern("(so|dll|dylib)$");


	connect(&_dataSock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));
}

void SendThread::handleSocketError(QAbstractSocket::SocketError error) {
	qDebug() << "SendThread socket error" << error;
	endThread();
}

SendThread::~SendThread() {
	_dataSock.abort();
	_dataSock.close();
}

void SendThread::endThread() {
	_dataSock.abort();
	_dataSock.close();
	exit();
}

bool SendThread::setSockFD(int sock) {
	return _dataSock.setSocketDescriptor(sock);
}

void SendThread::sendMediaList(const QList<QUrl> urls) {
	foreach(const QUrl url, urls) {
		sendMedia(url);
	}
}

void SendThread::sendMedia(const QUrl url) {
	if (_dataSock.state() != QAbstractSocket::ConnectedState) {
		qDebug() << "SendThread::sendMedia(). socket is not connected to the file server";
		return;
	}

	char header[EXTUI_MSG_SIZE];
	int mediatype = 0;

	QString urlStr = url.toString().trimmed();

//	qDebug() << "encoded url" << url.toEncoded(); // ByteArray
//	qDebug() << "sendMedia() : urlstr is" << urlStr;

	if (urlStr.contains(QRegExp("^http", Qt::CaseInsensitive)) ) {
		::sprintf(header, "%d %s %lld", (int)MEDIA_TYPE_WEBURL, qPrintable(urlStr), (qint64)0);
		_dataSock.write(header, sizeof(header));
	}
	else if (urlStr.contains(QRegExp("^file://", Qt::CaseSensitive)) ) {
		QFileInfo fi(urlStr.remove("file://"));
		if (!fi.isFile() || fi.size() == 0) {
			qDebug() << "SendThread::sendMedia : filesize 0 for" << fi.absoluteFilePath();
			return;
		}
		qDebug() << "sendMedia() : filePath" << fi.filePath() << "fileName" << fi.fileName();

		if (fi.suffix().contains(_rxImage)) {
			mediatype = (int)MEDIA_TYPE_IMAGE;
		}
		else if (fi.suffix().contains(_rxVideo)) {
			mediatype = (int)MEDIA_TYPE_LOCAL_VIDEO; // because the file is going to be copied at the wall
		}
		else if (fi.suffix().contains(_rxPdf)) {
			mediatype = (int)MEDIA_TYPE_PDF;
		}
		else if (fi.suffix().contains(_rxPlugin)) {
			mediatype = (int)MEDIA_TYPE_PLUGIN;
		}

		QString noSpaceFilename = fi.fileName().replace(QChar(' '), QChar('_'), Qt::CaseInsensitive);
		::sprintf(header, "%d %s %lld", mediatype, qPrintable(noSpaceFilename), fi.size());

		_dataSock.write(header, sizeof(header));

		QFile file(fi.absoluteFilePath());
		file.open(QIODevice::ReadOnly);
		if ( _dataSock.write(file.readAll().constData(), file.size()) < 0) {
			qDebug() << "SendThread::sendMedia() : error while sending the file";
		}
		file.close();
	}
}

void SendThread::run() {
	exec();
	qDebug() << "SentThread exit from event loop";
}
