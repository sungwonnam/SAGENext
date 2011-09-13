#ifndef APPINFO_H
#define APPINFO_H

#include "../../common/commondefinitions.h"

#include <QtCore>
#include <QHostAddress>

#include <QDir>

/**
  * AppInfo class maintains app specific information
  * meida type, source (disk or network)
  * and real time performance data
  */
class AppInfo
{
public:
	AppInfo();

	/**
	  * assumes filename is absolute path
	  */
	AppInfo(int width, int height, int bpp = 24);
	AppInfo(int width, int height, int bpp, const QString filename, const QString srcip);

//	inline QMimeData & getMimeData() { return mimedata;}

	void setFileInfo(const QString &absoluteFilePath);
	void setWebUrl(const QString & url);

	inline void setNativeSize(const QSize &s) { _nativeSize = s;}
	inline QSize nativeSize() const {return _nativeSize;}

	void setSrcAddr(const QString &ip);

	/**
	  this sets _nativeSize as well
	  */
	void setFrameSize(int width, int height, int bpp);
	//inline void setFrameSize(int bytecount) { frameSize = bytecount; }

	inline void setMediaType(MEDIA_TYPE t) {mtype = t;}
	inline MEDIA_TYPE getMediaType() const { return mtype; }

	inline void setDrawingThreadCpu(int c) {drawingThreadCpu=c;}
	inline void setNetworkUserBufferLength(int l) {networkUserBufferLength=l;}

	inline void setWebUrl(const QUrl &url) {webUrl = url;}

	inline void setExecutableName(const QString &str) {executableName = str;}
	inline void setCmdArgs(const QStringList &strlist) {cmdArgs = strlist;}


	inline QString getFilename() const { return fileinfo.absoluteFilePath(); }
	inline quint32 getFrameBytecount() const { return frameSize; }
//	inline quint32 getOrgWidth() const { return orgWidth; }
//	inline quint32 getOrgHeight() const { return orgHeight; }
	inline quint16 getBitPerPixel() const { return bitPerPixel; }

	inline int getDrawingThreadCpu() const {return drawingThreadCpu;}
	inline int getNetworkUserBufferLength() const {return networkUserBufferLength;}

	inline QUrl getWebUrl() const {return webUrl;}

	inline QString getExecutableName() const {return executableName;}
	inline QStringList getCmdArgs() const {return cmdArgs;}

	/**
	  * saved when minimized
	  */
	inline void setRecentBoundingRect(const QRectF &r) {recentBoundingRect = r;}
	inline void setRecentScale(qreal s) {recentScale = s;}

	/**
	  * used when restored
	  */
	inline QRectF getRecentBoundingRect() const { return recentBoundingRect; }
	inline qreal getRecentScale() const {return recentScale;}

	quint16 ratioToTheScene(qreal sceneSize) const;

private:
	MEDIA_TYPE mtype;

	/**
	  * file information. If it's web then isNull
	  */
	QFileInfo fileinfo;

	/*!
	  application name
	  */
	QString executableName;

	/*!
	  cmd line arguments
	  */
	QStringList cmdArgs;

	QUrl webUrl;

	QSize _nativeSize; /**< native resolution */

	/**
	  * sender's address
	  */
	QHostAddress srcaddr;

	quint16 bitPerPixel;

	/**
	  * Byte count of single frame
	  */
	quint32 frameSize;

	/**
	  * pos and size relative to the SCENE before minimized
	  */
	QRectF recentBoundingRect;

	/**
	  * scale before minimized
	  */
	qreal recentScale;

	/**
	  * On which cpu, drawing thread is
	  */
	int drawingThreadCpu;

	int networkUserBufferLength;

};

#endif // APPINFO_H
