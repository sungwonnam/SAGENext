#include "sn_pointerui_sendthread.h"
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
*/
#include <QFileInfo>

#include "sn_pointerui.h"

SN_PointerUI_DataThread::SN_PointerUI_DataThread(const QHostAddress &hostaddr, quint16 port, QObject *parent)

    : QThread(parent)

    , _hostaddr(hostaddr)
    , _port(port)

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


	QObject::connect(&_dataSock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));
}

void SN_PointerUI_DataThread::handleSocketError(QAbstractSocket::SocketError error) {
	qDebug() << "SendThread socket error" << error;
	endThread();
}

SN_PointerUI_DataThread::~SN_PointerUI_DataThread() {
	_dataSock.abort();
	_dataSock.close();
}

void SN_PointerUI_DataThread::endThread() {
	_dataSock.abort();
	_dataSock.close();
//	exit();
}


void SN_PointerUI_DataThread::prepareMediaList(const QList<QUrl> urls) {
	_mutex.lock();

	foreach(const QUrl url, urls) {
		enqueueMedia(url);
	}

	_mutex.unlock();

	emit fileReadyToSend();
}

void SN_PointerUI_DataThread::enqueueMedia(const QUrl url) {
	if (_dataSock.state() != QAbstractSocket::ConnectedState) {
		qDebug() << "SendThread::sendMedia(). socket is not connected to the file server";
		return;
	}

//	char header[EXTUI_MSG_SIZE];
	int mediatype = 0;

	QString urlStr = url.toString().trimmed();

//	qDebug() << "encoded url" << url.toEncoded(); // ByteArray
//	qDebug() << "sendMedia() : urlstr is" << urlStr;

	MediaItemInfo item;

	if (urlStr.contains(QRegExp("^http", Qt::CaseInsensitive)) ) {
//		::sprintf(header, "%d %s %lld", (int)SAGENext::MEDIA_TYPE_WEBURL, qPrintable(urlStr), (qint64)0);
//		_dataSock.write(header, sizeof(header));

		item.mediatype = (int)SAGENext::MEDIA_TYPE_WEBURL;
		::strcpy(item.filename, qPrintable(urlStr));

	}
	else if (urlStr.contains(QRegExp("^file://", Qt::CaseSensitive)) ) {
		QFileInfo fi(url.toLocalFile());
		if (!fi.isFile() || fi.size() == 0) {
			qDebug() << "SendThread::sendMedia : filesize 0 for" << fi.absoluteFilePath();
			return;
		}
		qDebug() << "sendMedia() : filePath" << fi.filePath() << "fileName" << fi.fileName();

		if (fi.suffix().contains(_rxImage)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_IMAGE;
		}
		else if (fi.suffix().contains(_rxVideo)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_LOCAL_VIDEO; // because the file is going to be copied at the wall
		}
		else if (fi.suffix().contains(_rxPdf)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_PDF;
		}
		else if (fi.suffix().contains(_rxPlugin)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_PLUGIN;
		}

		QString noSpaceFilename = fi.fileName().replace(QChar(' '), QChar('_'), Qt::CaseInsensitive);

		item.mediatype = mediatype;
		::strcpy(item.filename, qPrintable(noSpaceFilename));
		item.filesize = fi.size();
		::strcpy(item.filepath, qPrintable(fi.absoluteFilePath()));

		/*

		::sprintf(header, "%d %s %lld", mediatype, qPrintable(noSpaceFilename), fi.size());

		_dataSock.write(header, sizeof(header));

		QFile file(fi.absoluteFilePath());
		file.open(QIODevice::ReadOnly);
		if ( _dataSock.write(file.readAll().constData(), file.size()) < 0) {
			qDebug() << "SendThread::sendMedia() : error while sending the file";
		}
		file.close();
		*/
	}
	_sendList.push_back(item);
}

void SN_PointerUI_DataThread::run() {

	QTcpSocket socket;

	socket.connectToHost(_hostaddr, _port); // read write mode

	QObject::connect(&socket, SIGNAL(connected()), this, SIGNAL(connected()));
	QObject::connect(&socket, SIGNAL(readyRead()), this, SLOT(recvData()));

	exec();
}

void SN_PointerUI_DataThread::sendData(QTcpSocket &sock) {
	_mutex.lock();

	// lock is acquired again at this point

	char header[EXTUI_MSG_SIZE];

	QList<MediaItemInfo>::const_iterator it;

	for (it=_sendList.constBegin(); it!=_sendList.constEnd(); it++) {
		MediaItemInfo item = (*it);

		// send header
		::sprintf(header, "%d %s %lld", item.mediatype, item.filename, item.filesize);
		sock.write(header, sizeof(header));


		// send data
		QFile file(QString(item.filepath));
		file.open(QIODevice::ReadOnly);
		if ( sock.write(file.readAll().constData(), file.size()) < 0) {
			qDebug() << "SendThread::sendData() : error while sending the file";
		}
		else {
			qDebug() << QString(item.filepath) << "sent !!";
		}
		file.close();
	}

	_sendList.clear();

	_mutex.unlock();
}

void SN_PointerUI_DataThread::recvData() {

}

void SN_PointerUI_DataThread::receiveMedia(const QString &medianame, int mediafilesize) {

}
