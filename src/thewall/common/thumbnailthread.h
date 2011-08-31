#ifndef THUMBNAILTHREAD_H
#define THUMBNAILTHREAD_H

#include <QFileSystemWatcher>
#include <QThread>
#include <QtCore>

class QSettings;

/*!
  A thread generating thumbnail of local media.
  This class stores thumbnail in QPixmapCache (application wide)
  */
class ThumbnailThread : public QThread
{
	Q_OBJECT
public:
	explicit ThumbnailThread(const QString &mediaRoot, const QSettings *s, QObject *parent = 0);

protected:
	/*!
	  Do initial caching,
	  starts event loop
	  */
	void run();

private:
	const QSettings *wallSetting;

	/*!
	  root directory
	  */
	QDir mediaRootDir;

	QFileSystemWatcher fsWatcher;

	/*!
	  This is recursive function.

	  launched by doCaching
	  */
	void cachingFunction(const QString &path);

signals:

public slots:
	/*!
	  Generate pixmap and cache.
	  This function should run as a thread
	  */
	void runCachingThread(const QString &path);

};

#endif // THUMBNAILTHREAD_H
