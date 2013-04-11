#ifndef APPINFO_H
#define APPINFO_H

#include "common/sn_commondefinitions.h"

#ifdef QT5
#include <QtWidgets>
#else
#include <QtCore>
#endif

/**
  * AppInfo class maintains app specific information
  * meida type, source (disk or network)
  * and real time performance data
  */
class SN_AppInfo
{
public:
	SN_AppInfo(quint64 gaid);

	/**
	  * assumes filename is absolute path
	  */
	SN_AppInfo(quint64 gaid, int width, int height, int bpp = 24);
	SN_AppInfo(quint64 gaid, int width, int height, int bpp, const QString filename, const QString srcip);

	inline quint64 GID() const {return _GID;}
    inline void setGID(quint64 gaid) {_GID = gaid;}

	inline void setMediaType(SAGENext::MEDIA_TYPE t) {_mtype = t;}
	inline SAGENext::MEDIA_TYPE mediaType() const { return _mtype; }

	inline void setFileInfo(const QString &absoluteFilePath) {_fileinfo.setFile(absoluteFilePath);}
	inline QFileInfo fileInfo() const {return _fileinfo;}
	inline QString mediaFilename() const { return _fileinfo.absoluteFilePath(); }

//    inline void setRemoteFilepath(const QString &rf) {_remoteFilepath = rf;}
//    QString remoteFilepath() const {return _remoteFilepath;}

	/**
	  useful for SageStreamWidget
	  */
	inline void setExecutableName(const QString &str) {_executableName = str;}
	inline QString executableName() const {return _executableName;}

	/**
	  useful for SageStreamWidget
	  */
	inline void setCmdArgs(const QStringList &strlist) {_cmdArgsList = strlist;}
	inline void setCmdArgs(const QString &str) {_cmdArgsString = str;}
	inline QStringList cmdArgsList() const {return _cmdArgsList;}
	inline QString cmdArgsString() const {return _cmdArgsString;}


	/**
	  WebWidget
	  */
	inline void setWebUrl(const QUrl &url) {_webUrl = url;}
	inline QUrl webUrl() const {return _webUrl;}



	inline void setNativeSize(const QSize &s) { _nativeSize = s;}
	inline QSize nativeSize() const {return _nativeSize;}



	/**
	  sender ip address for VNCWidget and SageStreamWidget
	  */
	inline void setSrcAddr(const QString &ip) {_srcaddr = ip;}
	inline const QString & srcAddr() const {return _srcaddr;}

	inline quint16 bitPerPixel() const { return _bitPerPixel; }

	/**
	  This sets _nativeSize and _bitPerPixel as well
	  */
	void setFrameSize(int width, int height, int bpp);
	inline quint32 frameSizeInByte() const { return _frameSize; }


	/**
	  * saved when maximized/minimized for restore later
	  */
	inline void setRecentPos(const QPointF &p) {_recentPos = p;}
	inline QPointF recentPos() const {return _recentPos;}

	inline void setRecentSize(const QSizeF &s) {_recentSize = s;}
	inline QSizeF recentSize() const {return _recentSize;}

	inline void setRecentScale(qreal s) {_recentScale = s;}
	inline qreal recentScale() const {return _recentScale;}

	inline void setVncUsername(const QString &s) {_vncUsername = s;}
	inline void setVncPassword(const QString &s) {_vncPasswd = s;}
	inline QString vncUsername() const {return _vncUsername;}
	inline QString vncPassword() const {return _vncPasswd;}


	inline void setDrawingThreadCpu(int c) {_drawingThreadCpu=c;}
	inline int drawingThreadCpu() const {return _drawingThreadCpu;}

	inline void setNetworkUserBufferLength(int l) {_networkUserBufferLength=l;}
	inline int networkUserBufferLength() const {return _networkUserBufferLength;}

private:
	quint64 _GID;
	
	SAGENext::MEDIA_TYPE _mtype;

	/**
	  * file information. If it's web then isNull
	  */
	QFileInfo _fileinfo;

	/*!
	  SAIL application name for SAGE app
	  */
	QString _executableName;

    /*!
      The absolute filepath of remote media
      */
//    QString _remoteFilepath;

	/*!
	  cmd line arguments for SAIL app
	  */
	QStringList _cmdArgsList;

	/*!
	  cmd line arguments in a single string
	  */
	QString _cmdArgsString;

	/**
	  web URL for WebWidget
	  */
	QUrl _webUrl;

	QSize _nativeSize; /**< native resolution */

	/**
	  * sender's IP address for SageStreamWidget, VNCWidget
	  */
	QString _srcaddr;

	quint16 _bitPerPixel;

	/**
	  * Byte count of single frame
	  */
	quint32 _frameSize;

	/**
	  QGraphicsItem pos
	  */
	QPointF _recentPos;

	/**
	  QGraphicsWidget size for widget type window flag is Qt::WindowFlags
	  */
	QSizeF _recentSize;

	/**
	  * QGraphicsItem scale
	  */
	qreal _recentScale;

	/**
	  * On which cpu, drawing thread is
	  */
	int _drawingThreadCpu;

	int _networkUserBufferLength;

	QString _vncUsername;

	QString _vncPasswd;

};

#endif // APPINFO_H
