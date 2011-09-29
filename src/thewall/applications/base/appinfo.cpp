#include "appinfo.h"
//#include "common/perfmonitor.h"

AppInfo::AppInfo()
	: fileinfo(QFileInfo())
	, _webUrl(QUrl())
	, _nativeSize(QSize())
	, _srcaddr(QString())
	, bitPerPixel(0)
	, frameSize(0)
	, _recentScale(1)
	, drawingThreadCpu(-1)
	, networkUserBufferLength(65535) /* 64 kB */
{
}

AppInfo::AppInfo(int width, int height, int bpp)
	: fileinfo(QFileInfo())
	, _webUrl(QUrl())
	, _nativeSize(QSize(width, height))
	, _srcaddr(QString())
	, bitPerPixel(bpp)
	, frameSize(width * height * bpp)
	, _recentScale(1)
	, drawingThreadCpu(-1)
	, networkUserBufferLength(65535) /* 64 kB */
{
}

AppInfo::AppInfo(int width, int height, int bpp, QString file, QString srcip) :

		fileinfo(QFileInfo(file)),
		_webUrl(QUrl()),
		_nativeSize(QSize(width, height)),
		_srcaddr(srcip),
		bitPerPixel(bpp),
		frameSize(width * height * bpp),
		_recentScale(1),
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

void AppInfo::setWebUrl(const QString &url) {
	_webUrl.setUrl(url);
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
