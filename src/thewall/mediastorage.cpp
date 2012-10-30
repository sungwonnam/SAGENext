#include "mediastorage.h"

#include <QSettings>
#include "applications/mediabrowser.h"

QReadWriteLock SN_MediaStorage::MediaListRWLock;
QList<SN_MediaItem *> SN_MediaStorage::MediaList;

SN_MediaStorage::SN_MediaStorage(const QSettings *s, QObject *parent)
    : QObject(parent)
    , _settings(s)
{
	int nummedia = _initMediaList();
	qWarning() << "SN_MediaStorage : initialized" << nummedia << "items";
}


int SN_MediaStorage::_initMediaList() {
	int numread = 0;
    QDirIterator iter(QDir::homePath() + "/.sagenext/", QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();
        if(iter.fileInfo().isFile()) {
            numread++;

            SN_MediaItem* item =  0;
            QPixmap p = _createThumbnail(SAGENext::MEDIA_TYPE_UNKNOWN, iter.filePath());
            if (p.isNull()) {
                qDebug() << "SN_MediaStorage::_initMediaList() : failed to create thumbnail for" << iter.filePath();
                item = new SN_MediaItem(_findMediaType(iter.filePath()), iter.filePath(), QPixmap(":/resources/x_circle_gray.png"));
            }
            else {
                item = new SN_MediaItem(_findMediaType(iter.filePath()), iter.filePath(), p);
            }
            addNewMedia(item);
        }
        // build a list of directories too
        else if(iter.fileInfo().isDir()) {
            numread++;

            SN_MediaItem* item = new SN_MediaItem(SAGENext::MEDIA_TYPE_UNKNOWN, iter.filePath(), QPixmap(":/resources/dir.png"));
            addNewMedia(item);
        }
    }


	// iterate over $(HOME)/.sagenext/media and build hash

	// also iterate over $(HOME)/.sagenext/sessions and build hash for the saved sessions too

	return numread;
}


bool SN_MediaStorage::addNewMedia(SN_MediaItem* mediaitem) {

    SN_MediaStorage::MediaListRWLock.lockForWrite();
    // Add entry to list, save image as pixmap
    SN_MediaStorage::MediaList.push_back(mediaitem);
    SN_MediaStorage::MediaListRWLock.unlock();

    emit SN_MediaStorage::newMediaAdded();

    return true;
}

bool SN_MediaStorage::addNewMedia(SAGENext::MEDIA_TYPE mtype, const QString &filepath) {

    //
    // Check if this media is already exist in the storage
    //

    SN_MediaItem *newitem = new SN_MediaItem(mtype, filepath, _createThumbnail(mtype, filepath));

    QObject::connect(newitem, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SIGNAL(mediaItemClicked(SAGENext::MEDIA_TYPE,QString)));


    return addNewMedia(newitem);
}

bool SN_MediaStorage::checkForMediaInList(const QString &path) {

    bool result = false;

    SN_MediaStorage::MediaListRWLock.lockForRead();

    foreach (SN_MediaItem *item, SN_MediaStorage::MediaList) {

        // check if there's SN_MediaItem with its filename == path
        // If so, then return false
    }

    SN_MediaStorage::MediaListRWLock.unlock();

    qDebug() << "SN_MediaStorage::checkForMediaInHash() : " << path << " " << result;
    return result;
}


QList<SN_MediaItem *> * SN_MediaStorage::getMediaListInDir(const QDir &dir) {
    // check the dir and build the list of SN_MediaItem
    // return the list
    // The list will be owned by SN_MediaBrowser so don't delete the list

    qDebug() << "SN_MediaStorage::getMediaListInDir()" << dir;
    return 0;
}

SAGENext::MEDIA_TYPE SN_MediaStorage::_findMediaType(const QString &filepath) {
    // use regular expression

    // if it's directory then return SAGENext::MEDIA_TYPE_UNKNOWN
}







QPixmap SN_MediaStorage::_createThumbnail(SAGENext::MEDIA_TYPE mtype, const QString &filename) {

    if (mtype == SAGENext::MEDIA_TYPE_UNKNOWN) {
        QRegExp rxVideo("\\.(avi|mov|mpg|mpeg|mp4|mkv|flv|wmv)$", Qt::CaseInsensitive, QRegExp::RegExp);
        QRegExp rxImage("\\.(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
        QRegExp rxPdf("\\.(pdf)$", Qt::CaseInsensitive, QRegExp::RegExp);
        QRegExp rxPlugin;
        rxPlugin.setCaseSensitivity(Qt::CaseInsensitive);
        rxPlugin.setPatternSyntax(QRegExp::RegExp);
    #if defined(Q_OS_LINUX)
        rxPlugin.setPattern("\\.so$");
    #elif defined(Q_OS_WIN32)
        rxPlugin.setPattern("\\.dll$");
    #elif defined(Q_OS_DARWIN)
        rxPlugin.setPattern("\\.dylib$");
    #endif

        if (filename.contains(rxVideo)) {
            return _readVideo(filename);
        }
        else if (filename.contains(rxImage)) {
            return _readImage(filename);
        }
        else if (filename.contains(rxPdf)) {
            return _readPDF(filename);
        }
        else if (filename.contains(rxPlugin)) {
            // do nothing
        }
        else {
            // error
        }
    }
    else if (mtype == SAGENext::MEDIA_TYPE_IMAGE) {
        return _readImage(filename);
    }
    else if(mtype == SAGENext::MEDIA_TYPE_LOCAL_VIDEO) {
        return _readVideo(filename);
    }
    else if (mtype== SAGENext::MEDIA_TYPE_PDF) {
        return _readPDF(filename);
    }
}









QPixmap SN_MediaStorage::_readImage(const QString &filename) {
    qDebug() << "SN_MediaStorage::_readImage() : " << filename;
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

    else {
        qWarning("%s::%s() : Couldn't create the thumbnail for %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
        return pixmap;
    }
}

QPixmap SN_MediaStorage::_readPDF(const QString &filename) {
    qDebug() << "SN_MediaStorage::_readPDF() : " << filename;
    QPixmap pixmap;

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
        return pixmap;
    else
        return pixmap.scaled(
                _settings->value("gui/mediathumbnailwidth", 256).toInt()
                ,_settings->value("gui/mediathumbnailwidth",256).toInt()
                ,Qt::IgnoreAspectRatio
                ,Qt::FastTransformation
                );
}

QPixmap SN_MediaStorage::_readVideo(const QString &filename) {
    qDebug() << "SN_MediaStorage::_readVideo() : " << filename;

    // Generate video thumbnail
    QString program = "mplayer";
    QStringList arguments;
    arguments << "-vo" << "jpeg" << "-quiet" << "-nosound" << "-frames" << "1" << "-ss" << "5" << filename;

    QProcess *myProcess = new QProcess();
    myProcess->start(program, arguments);
    while( myProcess->waitForFinished() ){
        qDebug() << "SN_MediaStorage::_readVideo() : Block Waiting for thumb generation for mov";
    }

    // Apparently running the above command  will create an image called '00000001.jpg' in the build directory
    QFileInfo thumbLocation = QFileInfo("$${BUILD_DIR}");
    QPixmap pixmap(thumbLocation.absolutePath() + "/00000001.jpg");

    if( pixmap.isNull() )
        return pixmap;

    else
        return pixmap.scaled(
                    _settings->value("gui/mediathumbnailwidth", 256).toInt()
                    ,_settings->value("gui/mediathumbnailwidth",256).toInt()
                    ,Qt::IgnoreAspectRatio
                    ,Qt::FastTransformation
                    );
}


