#include "appinfo.h"
//#include "common/perfmonitor.h"

AppInfo::AppInfo() :

		fileinfo(QFileInfo()),
		webUrl(QUrl()),
		_nativeSize(QSize()),
		srcaddr(QHostAddress(QHostAddress::Null)),
		bitPerPixel(0),
		frameSize(0),
		recentBoundingRect(QRectF()),
		recentScale(1),
		drawingThreadCpu(-1),
		networkUserBufferLength(65535) /* 64 kB */

{
}

AppInfo::AppInfo(int width, int height, int bpp) :

		fileinfo(QFileInfo()),
		webUrl(QUrl()),
		_nativeSize(QSize(width, height)),
		srcaddr(QHostAddress(QHostAddress::Null)),
		bitPerPixel(bpp),
		frameSize(width * height * bpp),
		recentBoundingRect(QRectF()),
		recentScale(1),
		drawingThreadCpu(-1),
		networkUserBufferLength(65535) /* 64 kB */
{
}

AppInfo::AppInfo(int width, int height, int bpp, QString file, QString srcip) :

		fileinfo(QFileInfo(file)),
		webUrl(QUrl()),
		_nativeSize(QSize(width, height)),
		srcaddr(QHostAddress(srcip)),
		bitPerPixel(bpp),
		frameSize(width * height * bpp),
		recentBoundingRect(QRectF()),
		recentScale(1),
		drawingThreadCpu(-1),
		networkUserBufferLength(65535) /* 64 kB */
{
}

void AppInfo::setFileInfo(const QString &absoluteFilePath) {
	fileinfo.setFile( absoluteFilePath );
}

void AppInfo::setFrameSize(int width, int height, int bpp) {
//	orgWidth = width;
//	orgHeight = height;
	_nativeSize.rwidth() = width;
	_nativeSize.rheight() = height;
//	_nativeSize = QSize(width, height);
	bitPerPixel = bpp;

	frameSize = width * height * bpp / 8; // Byte
}

void AppInfo::setSrcAddr(const QString &ip) {
	srcaddr.setAddress(ip);
}

void AppInfo::setWebUrl(const QString &url) {
	webUrl.setUrl(url);
}

quint16 AppInfo::ratioToTheScene(qreal sceneSize) const
{
	qreal ratio = (recentBoundingRect.width() * recentBoundingRect.height()) / sceneSize;
//	qreal ratio = (recentBoundingRect.width() * recentScale * recentBoundingRect.height() * recentScale) / (qreal)sceneSize;
	return (quint16)(ratio * 100.0);
}

//void AppInfo::setPosAndSize(const QPointF &pos, const QSizeF &size) {
//	recentPosAndSize.setTopLeft(pos);
//	recentPosAndSize.setSize(size);
//}

//void AppInfo::setPosAndSize(const QRectF &rect) {
//	setPosAndSize(rect.topLeft(), rect.size());
//}

//void AppInfo::setPosAndSize(const QSizeF &size) {
//	setPosAndSize(QPointF(0,0), size);
//}

//void AppInfo::setPosAndSize(qreal width, qreal height) {
//	setPosAndSize(QPointF(0,0), QSizeF(width, height));
//}
