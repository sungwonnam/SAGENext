#include "thumbnailthread.h"

#include <QtCore>
#include <QPixmap>
#include <QPixmapCache>

ThumbnailThread::ThumbnailThread(const QString &mediaroot, const QSettings *s, QObject *parent) :
	QThread(parent),
	wallSetting(s)
{
	mediaRootDir.setFilter(QDir::NoSymLinks | QDir::NoDotAndDotDot);
	mediaRootDir.setSorting(QDir::Name | QDir::DirsFirst);
	mediaRootDir.setPath(mediaroot);

	start();
}

void ThumbnailThread::run() {

	QFileSystemWatcher fsWatcher;
	connect(&fsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(runCachingThread(QString)));


	if (! mediaRootDir.exists() ) {
		qCritical() << metaObject()->className() << " : No such dir exist." << wallSetting->value("general/mediadir").toString();
		return;
	}

	QStringList dirsWatched;
	dirsWatched << mediaRootDir.absolutePath();

	// recursively iterate over media directory, add directories to watcher
	QFileInfoList filist = mediaRootDir.entryInfoList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot,   QDir::Name | QDir::DirsFirst);

	// for each directories in mediaDir
	foreach (QFileInfo fi, filist) {
		if (!fi.exists()  ||  !fi.isDir()) continue;
		dirsWatched << fi.dir().absolutePath();
	}

	/**
	  do initial caching
	  **/
	cachingFunction(mediaRootDir.absolutePath());


	// starts event loop. connection b/w fsWatcher and doCaching will take effect after this
	exec();
}


void ThumbnailThread::runCachingThread(const QString &path) {

	// launch cacheing thread
	QtConcurrent::run(this, &ThumbnailThread::cachingFunction, path);
}


void ThumbnailThread::cachingFunction(const QString &path) {
	QDir dir;
	dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::Name | QDir::DirsLast);
	dir.setPath(path);

	foreach (QFileInfo fi, dir.entryInfoList()) {
		if (!fi.exists()  ||  fi.isSymLink()) continue;

		// Recursive
		if (fi.isDir())
			cachingFunction(fi.absolutePath());
		else {
			QString filepath = fi.absoluteFilePath();

			QPixmap pixmap(filepath);
			if (pixmap.isNull()) continue;

			// this is application-wide cache for pixmaps
			QPixmapCache::insert(filepath, pixmap.scaledToWidth(150));
		}
	}
}
