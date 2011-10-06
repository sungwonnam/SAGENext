#include "mediastorage.h"

QReadWriteLock MediaStorage::mediaHashRWLock;
QHash<QString,QPixmap> MediaStorage::mediaHash;

MediaStorage::MediaStorage(QObject *parent) :
    QObject(parent)
{
}

bool MediaStorage::insertNewMediaToHash(const QString &key) {
    qDebug() << "MediaStorage::insertNewMediaToHash() : " << key;

    MediaStorage::mediaHashRWLock.lockForWrite();
    // Add entry to list, save image as pixmap
    MediaStorage::mediaHash.insert(key, readImage(key));
    MediaStorage::mediaHashRWLock.unlock();

    emit MediaStorage::newMediaAdded();

    return true;
}

bool MediaStorage::checkForMediaInHash(const QString &key) {


    bool result = false;

    MediaStorage::mediaHashRWLock.lockForWrite();
    result = MediaStorage::mediaHash.contains(key);
    MediaStorage::mediaHashRWLock.unlock();

    qDebug() << "MediaStorage::checkForMediaInHash() : " << key << " " << result;
    return result;
}

QPixmap MediaStorage::readImage(const QString &filename) {
        QImage image;
        QPixmap pixmap;
        if (image.load(filename)) {
                pixmap.convertFromImage(image);
                return pixmap;
        }
        else qWarning("%s::%s() : QImage::load(%s) failed !",metaObject()->className(), __FUNCTION__, qPrintable(filename));

        return false;
}

QHash<QString,QPixmap> MediaStorage::getMediaHash(){
    QHash<QString,QPixmap> mediaHashOut;
    MediaStorage::mediaHashRWLock.lockForWrite();
    mediaHashOut = MediaStorage::mediaHash;
    MediaStorage::mediaHashRWLock.unlock();

    return mediaHashOut;
}
