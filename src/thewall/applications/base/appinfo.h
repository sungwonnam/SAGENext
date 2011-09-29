#ifndef APPINFO_H
#define APPINFO_H

#include "../../common/commondefinitions.h"

#include <QtCore>

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

	inline void setSrcAddr(const QString &ip) {_srcaddr = ip;}

	/**
	  this sets _nativeSize as well
	  */
	void setFrameSize(int width, int height, int bpp);
	//inline void setFrameSize(int bytecount) { frameSize = bytecount; }

	inline void setMediaType(MEDIA_TYPE t) {mtype = t;}
	inline MEDIA_TYPE mediaType() const { return mtype; }

	inline void setDrawingThreadCpu(int c) {drawingThreadCpu=c;}
	inline void setNetworkUserBufferLength(int l) {networkUserBufferLength=l;}

	inline void setWebUrl(const QUrl &url) {_webUrl = url;}

	inline void setExecutableName(const QString &str) {_executableName = str;}
	inline void setCmdArgs(const QStringList &strlist) {_cmdArgs = strlist;}


	inline QString mediaFilename() const { return fileinfo.absoluteFilePath(); }
	inline quint32 getFrameBytecount() const { return frameSize; }
//	inline quint32 getOrgWidth() const { return orgWidth; }
//	inline quint32 getOrgHeight() const { return orgHeight; }
	inline quint16 getBitPerPixel() const { return bitPerPixel; }

	inline int getDrawingThreadCpu() const {return drawingThreadCpu;}
	inline int getNetworkUserBufferLength() const {return networkUserBufferLength;}

	inline QUrl webUrl() const {return _webUrl;}

	inline QString executableName() const {return _executableName;}
	inline QStringList cmdArgs() const {return _cmdArgs;}

	/**
	  * saved when maximized/minimized for restore later
	  */
	inline void setRecentPos(const QPointF &p) {_recentPos = p;}
	inline QPointF recentPos() const {return _recentPos;}

	inline void setRecentScale(qreal s) {_recentScale = s;}
	inline qreal recentScale() const {return _recentScale;}

	inline void setRecentSize(const QSizeF &s) {_recentSize = s;}
	inline QSizeF recentSize() const {return _recentSize;}


	inline void setVncUsername(const QString &s) {_vncUsername = s;}
	inline void setVncPassword(const QString &s) {_vncPasswd = s;}
	inline QString vncUsername() const {return _vncUsername;}
	inline QString vncPassword() const {return _vncPasswd;}

private:
	MEDIA_TYPE mtype;

	/**
	  * file information. If it's web then isNull
	  */
	QFileInfo fileinfo;

	/*!
	  SAIL application name for SAGE app
	  */
	QString _executableName;

	/*!
	  cmd line arguments for SAIL app
	  */
	QStringList _cmdArgs;

	/**
	  web URL for WebWidget
	  */
	QUrl _webUrl;

	QSize _nativeSize; /**< native resolution */

	/**
	  * sender's IP address for SageStreamWidget, VNCWidget
	  */
	QString _srcaddr;

	quint16 bitPerPixel;

	/**
	  * Byte count of single frame
	  */
	quint32 frameSize;

	/**
	  QGraphicsItem pos
	  */
	QPointF _recentPos;

	/**
	  QGraphicsWidget size
	  */
	QSizeF _recentSize;

	/**
	  * QGraphicsItem scale
	  */
	qreal _recentScale;

	/**
	  * On which cpu, drawing thread is
	  */
	int drawingThreadCpu;

	int networkUserBufferLength;

	QString _vncUsername;

	QString _vncPasswd;

};

#endif // APPINFO_H
