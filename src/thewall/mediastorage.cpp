#include "mediastorage.h"

QReadWriteLock SN_MediaStorage::mediaHashRWLock;
QHash<QString,QPixmap> SN_MediaStorage::mediaHash;

SN_MediaStorage::SN_MediaStorage(QObject *parent) :
    QObject(parent)
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
        QImage image;
        QPixmap pixmap;
        if (image.load(filename)) {
                pixmap.convertFromImage(image);
                return pixmap;
        }
        else qWarning("%s::%s() : QImage::load(%s) failed !",metaObject()->className(), __FUNCTION__, qPrintable(filename));

        return false;
}

QHash<QString,QPixmap> SN_MediaStorage::getMediaHash(){
    QHash<QString,QPixmap> mediaHashOut;
    SN_MediaStorage::mediaHashRWLock.lockForWrite();
    mediaHashOut = SN_MediaStorage::mediaHash;
    SN_MediaStorage::mediaHashRWLock.unlock();

    return mediaHashOut;
}
