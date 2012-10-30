#include "mediastorage.h"

#include "applications/mediabrowser.h"

#include <poppler-qt4.h>

QReadWriteLock SN_MediaStorage::MediaListRWLock;
QMap<QString, MediaMetaData*> SN_MediaStorage::GlobalMediaList;

SN_MediaStorage::SN_MediaStorage(const QSettings *s, QObject *parent)
    : QObject(parent)
    , _settings(s)
{
	int nummedia = _initMediaList();
	qWarning() << "SN_MediaStorage : initialized" << nummedia << "items";
}

SN_MediaStorage::~SN_MediaStorage() {
    QMap<QString, MediaMetaData*>::iterator iter;
    for (iter=SN_MediaStorage::GlobalMediaList.begin(); iter!=SN_MediaStorage::GlobalMediaList.end(); iter++) {
        delete iter.value();
    }
}

int SN_MediaStorage::_initMediaList() {
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

	int numread = 0;
    QDirIterator iter(QDir::homePath() + "/.sagenext/media", QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories);
    while (iter.hasNext()) {
        iter.next();
        if(iter.fileInfo().isFile()) {
            numread++;

            MediaMetaData *mediameta = 0;
            if (iter.filePath().contains(rxVideo)) {
                mediameta = new MediaMetaData;
                mediameta->pixmap = _readVideo(iter.filePath());
                mediameta->type = SAGENext::MEDIA_TYPE_LOCAL_VIDEO;
            }
            else if (iter.filePath().contains(rxImage)) {
                mediameta = new MediaMetaData;
                mediameta->pixmap = _readImage(iter.filePath());
                mediameta->type = SAGENext::MEDIA_TYPE_IMAGE;
            }
            else if (iter.filePath().contains(rxPdf)) {
                mediameta = new MediaMetaData;
                mediameta->pixmap = _readPDF(iter.filePath());
                mediameta->type = SAGENext::MEDIA_TYPE_PDF;
            }
            else if (iter.filePath().contains(rxPlugin)) {
                // do nothing for now
            }
            else {
                // error
            }

            if (mediameta) {
                SN_MediaStorage::GlobalMediaList.insert(iter.filePath(), mediameta);
            }
        }
        // build a list of directories too
        else if(iter.fileInfo().isDir()) {
            numread++;

            MediaMetaData *meta = new MediaMetaData;
            meta->type = SAGENext::MEDIA_TYPE_UNKNOWN;
            meta->pixmap = QPixmap(":/resources/dir.png");
            SN_MediaStorage::GlobalMediaList.insert(iter.filePath(), meta);
        }
    }


	// iterate over $(HOME)/.sagenext/media and build hash

	// also iterate over $(HOME)/.sagenext/sessions and build hash for the saved sessions too

	return numread;
}

void SN_MediaStorage::addNewMedia(SAGENext::MEDIA_TYPE mtype, const QString &filepath) {

    //
    // Check if this media is already exist in the storage
    //
    if ( SN_MediaStorage::GlobalMediaList.find(filepath) == SN_MediaStorage::GlobalMediaList.end()) {
        qDebug() << "SN_MediaStorage::addNewMedia() : " << filepath << "already exist in the list";
        return;
    }

    MediaMetaData* meta = new MediaMetaData;
    meta->type = mtype;
    if (mtype == SAGENext::MEDIA_TYPE_IMAGE) {
        meta->pixmap = _readImage(filepath);
    }
    else if (mtype == SAGENext::MEDIA_TYPE_LOCAL_VIDEO) {
        meta->pixmap = _readVideo(filepath);
    }
    else if (mtype == SAGENext::MEDIA_TYPE_PDF) {
        meta->pixmap = _readPDF(filepath);
    }

    SN_MediaStorage::MediaListRWLock.lockForWrite();
    SN_MediaStorage::GlobalMediaList.insert(filepath, meta);
    SN_MediaStorage::MediaListRWLock.unlock();
}


QMap<QString,MediaMetaData*> SN_MediaStorage::getMediaListInDir(const QDir &dir) {
    // check the dir recursively and build the list of SN_MediaItem
    // return the list
    // The list will be owned by SN_MediaBrowser so don't delete the list

    qDebug() << "SN_MediaStorage::getMediaListInDir()" << dir;

    QMap<QString, MediaMetaData*> itemsInDir;

    QMap<QString, MediaMetaData*>::iterator iter = SN_MediaStorage::GlobalMediaList.begin();
    
    SN_MediaStorage::MediaListRWLock.lockForRead();
    
    // for each item in the global list
    for (; iter!=SN_MediaStorage::GlobalMediaList.end(); iter++) {
        
        // if the item is in the dir
        if ( iter.key().startsWith(dir.path(), Qt::CaseSensitive) ) {
            qDebug() << "SN_MediaStorage::getMediaListInDir() : " << iter.key();
            itemsInDir.insert(iter.key(), iter.value());
        }
    }

    SN_MediaStorage::MediaListRWLock.unlock();
    
    return itemsInDir;
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
    else if (mtype == SAGENext::MEDIA_TYPE_LOCAL_VIDEO) {
        return _readVideo(filename);
    }
    else if (mtype == SAGENext::MEDIA_TYPE_PDF) {
        return _readPDF(filename);
    }
    return QPixmap();
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
//        qDebug() << "SN_MediaStorage::_readVideo() : Block Waiting for thumb generation for mov";
    }

    // Apparently running the above command  will create an image called '00000001.jpg' in the build directory
    QFileInfo thumbLocation = QFileInfo("$${BUILD_DIR}");
    QPixmap pixmap(thumbLocation.absolutePath() + "/00000001.jpg");

    if( pixmap.isNull() ) {
        qDebug() << "SN_MediaStorage::_readVideo() : failed to create thumbnail for" << filename;
        return pixmap;
    }

    else
        return pixmap.scaled(
                    _settings->value("gui/mediathumbnailwidth", 256).toInt()
                    ,_settings->value("gui/mediathumbnailwidth",256).toInt()
                    ,Qt::IgnoreAspectRatio
                    ,Qt::FastTransformation
                    );
}


