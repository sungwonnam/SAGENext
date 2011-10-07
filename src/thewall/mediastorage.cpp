#include "mediastorage.h"

#include <QSettings>

QReadWriteLock SN_MediaStorage::mediaHashRWLock;
QHash<QString, QPixmap> SN_MediaStorage::mediaHash;

SN_MediaStorage::SN_MediaStorage(const QSettings *s, QObject *parent)
    : QObject(parent)
    , _settings(s)
{
}

bool SN_MediaStorage::insertNewMediaToHash(const QString &key) {
    qDebug() << "MediaStorage::insertNewMediaToHash() : " << key;

    SN_MediaStorage::mediaHashRWLock.lockForWrite();
    // Add entry to list, save image as pixmap
    SN_MediaStorage::mediaHash.insert(key, readImage(key));
    SN_MediaStorage::mediaHashRWLock.unlock();

    emit SN_MediaStorage::newMediaAdded();

    return true;
}

bool SN_MediaStorage::checkForMediaInHash(const QString &key) {


    bool result = false;

    SN_MediaStorage::mediaHashRWLock.lockForWrite();
    result = SN_MediaStorage::mediaHash.contains(key);
    SN_MediaStorage::mediaHashRWLock.unlock();

    qDebug() << "MediaStorage::checkForMediaInHash() : " << key << " " << result;
    return result;
}

QPixmap SN_MediaStorage::readImage(const QString &filename) {
//        QImage image;
	QPixmap pixmap(filename);
		/*
        if (image.load(filename)) {
                pixmap.convertFromImage(image);
                return pixmap;
        }
		*/
	if ( ! pixmap.isNull() ) {
		//
		// for better drawing performance, thumbnail shouldn't be large
		//
		return pixmap.scaled(
		            _settings->value("gui/mediathumbnailwidth", 256).toInt()
		            ,_settings->value("gui/mediathumbnailwidth",256).toInt()
		            ,Qt::IgnoreAspectRatio
		            ,Qt::FastTransformation);
	}

	else qWarning("%s::%s() : Couldn't create the thumbnail for %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));

	return pixmap;
}

QHash<QString, QPixmap> SN_MediaStorage::getMediaHash(){
    QHash<QString, QPixmap> mediaHashOut;
    SN_MediaStorage::mediaHashRWLock.lockForWrite();
    mediaHashOut = SN_MediaStorage::mediaHash;
    SN_MediaStorage::mediaHashRWLock.unlock();

    return mediaHashOut;
}
