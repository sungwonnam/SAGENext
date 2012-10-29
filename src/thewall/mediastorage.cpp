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

    // List of known non-image file types
    QRegExp rxVideo("avi|mov|mpg|mpeg|mp4|mkv|flv|wmv", Qt::CaseInsensitive); // Is a video with these file extensions?
    QRegExp rxPDF("pdf", Qt::CaseInsensitive);

    // Get the file extension from the filename
    QString ext = key;
    ext.remove(0,key.lastIndexOf("."));

    if( ext.toLower().contains(rxVideo) ){ // Video
        // Add entry to list, save image as pixmap
        SN_MediaStorage::mediaHash.insert(key, readVideo(key));
    }
    else if( ext.toLower().contains(rxPDF) ){ // PDF
        // Add entry to list, save image as pixmap
        SN_MediaStorage::mediaHash.insert(key, readPDF(key));
    }
    else{ // Assume image
        // Add entry to list, save image as pixmap
        SN_MediaStorage::mediaHash.insert(key, readImage(key));
    }

    SN_MediaStorage::mediaHashRWLock.unlock();
    emit SN_MediaStorage::newMediaAdded();
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
    qDebug() << "MediaStorage::readImage() : " << filename;
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

QPixmap SN_MediaStorage::readPDF(const QString &filename) {
    qDebug() << "MediaStorage::readPDF() : " << filename;
    QPixmap pixmap(filename);

    Poppler::Document *_document;
    _document = Poppler::Document::load(filename);
    if (!_document || _document->isLocked()) {
        qCritical("%s::%s() : Couldn't open pdf file %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
    } else {
        Poppler::Page *_currentPage = _document->page(0);
        pixmap = QPixmap::fromImage(_currentPage->renderToImage(200,200));

        _currentPage->~Page();
    }
    _document->~Document();

    if( pixmap.isNull() )
        pixmap = QPixmap(":/resources/close_over.png");

    return pixmap.scaled(
                _settings->value("gui/mediathumbnailwidth", 256).toInt()
                ,_settings->value("gui/mediathumbnailwidth",256).toInt()
                ,Qt::IgnoreAspectRatio
                ,Qt::FastTransformation);
}

QPixmap SN_MediaStorage::readVideo(const QString &filename) {
    qDebug() << "MediaStorage::readVideo() : " << filename;

    // Generate video thumbnail
    QString program = "mplayer";
    QStringList arguments;
    arguments << "-vo" << "jpeg" << "-quiet" << "-nosound" << "-frames" << "1" << "-ss" << "5" << filename;

    QProcess *myProcess = new QProcess();
    myProcess->start(program, arguments);
    while( myProcess->waitForFinished() ){
        qDebug() << "Waiting for thumb generation for mov";
    }

    // Apparently running the above command  will create an image called '00000001.jpg' in the build directory
    QFileInfo thumbLocation = QFileInfo("$${BUILD_DIR}");
    QPixmap pixmap(thumbLocation.absolutePath() + "/00000001.jpg");

    if( pixmap.isNull() )
        pixmap = QPixmap(":/resources/close_over.png");

    return pixmap.scaled(
        _settings->value("gui/mediathumbnailwidth", 256).toInt()
        ,_settings->value("gui/mediathumbnailwidth",256).toInt()
        ,Qt::IgnoreAspectRatio
        ,Qt::FastTransformation);
}


