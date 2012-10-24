#include "mediastorage.h"

#include <QSettings>

QReadWriteLock SN_MediaStorage::mediaHashRWLock;
QHash<QString, QPixmap> SN_MediaStorage::mediaHash;

SN_MediaStorage::SN_MediaStorage(const QSettings *s, QObject *parent)
    : QObject(parent)
    , _settings(s)
{

	int nummedia = initMediaHash();
	qWarning() << "SN_MediaStorage : initialized" << nummedia << "items";
}

int SN_MediaStorage::initMediaHash() {
	int numread = 0;
    QDirIterator mediaIterator(QDir::homePath() + "/.sagenext/", QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories);
    while (mediaIterator.hasNext()) {
        mediaIterator.next();
        if(mediaIterator.fileInfo().isFile()) {
            numread++;
            insertNewMediaToHash(mediaIterator.filePath());
        }
        // build a list of directories too
        else if(mediaIterator.fileInfo().isDir()) {
            numread++;
            insertNewFolderToHash(mediaIterator.filePath());
        }
    }


	// iterate over $(HOME)/.sagenext/media and build hash

	// also iterate over $(HOME)/.sagenext/sessions and build hash for the saved sessions too

	return numread;
}

bool SN_MediaStorage::insertNewFolderToHash(const QString &key) {
    qDebug() << "MediaStorage::insertNewFolderToHash() : " << key;

    SN_MediaStorage::mediaHashRWLock.lockForWrite();
    // Add entry to list, save folder image as pixmap
    SN_MediaStorage::mediaHash.insert(key, QPixmap(":/resources/dir.png"));
    SN_MediaStorage::mediaHashRWLock.unlock();

    emit SN_MediaStorage::newMediaAdded();

    return true;
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

    // get a image thumbnail for pdf
    if(filename.endsWith(".pdf")) {
        QPixmap pixmap;
        Poppler::Document *_document = Poppler::Document::load(filename);
        Poppler::Page *p = _document->page(0);
        if (p) {
             pixmap = QPixmap::fromImage(p->renderToImage(64, 64));
        }
        delete p;
        delete _document;


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
        else {
            qWarning("%s::%s() : Couldn't create the thumbnail for %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
            return pixmap;
        }
    } else {
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
            else {
                qWarning("%s::%s() : Couldn't create the thumbnail for %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
                return pixmap;
            }
    }
}


