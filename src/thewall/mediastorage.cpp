#include "mediastorage.h"

#include "applications/sn_mediabrowser.h"

//#include <poppler-qt4.h>

QReadWriteLock SN_MediaStorage::MediaListRWLock;
QMap<QString, MediaMetaData*> SN_MediaStorage::GlobalMediaList;

SN_MediaStorage::SN_MediaStorage(const QSettings *s, QObject *parent)
    : QObject(parent)
    , _settings(s)
    , _thumbSize(QSize(_settings->value("gui/mediathumbnailwidth", 128).toInt(), 0))
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
    QDirIterator iter(QDir::homePath() + "/.sagenext/media", QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (iter.hasNext()) {
        iter.next();
        if(iter.fileInfo().isFile()) {
            numread++;

            MediaMetaData *mediameta = 0;
            if (iter.filePath().contains(rxVideo)) {
                mediameta = new MediaMetaData;
                mediameta->type = SAGENext::MEDIA_TYPE_LOCAL_VIDEO;
            }
            else if (iter.filePath().contains(rxImage)) {
                mediameta = new MediaMetaData;
                mediameta->type = SAGENext::MEDIA_TYPE_IMAGE;
            }
            else if (iter.filePath().contains(rxPdf)) {
                mediameta = new MediaMetaData;
                mediameta->type = SAGENext::MEDIA_TYPE_PDF;
            }
            else if (iter.filePath().contains(rxPlugin)) {
                mediameta = new MediaMetaData;
                mediameta->type = SAGENext::MEDIA_TYPE_PLUGIN;
            }
            else {
                // error
            }


            if (mediameta) {
                mediameta->pixmap = _createThumbnail(iter.filePath(), mediameta->type);
                SN_MediaStorage::GlobalMediaList.insert(iter.filePath(), mediameta);
            }
        }
        // build a list of directories too
        else if(iter.fileInfo().isDir()) {
            numread++;

            MediaMetaData *meta = new MediaMetaData;
            meta->type = SAGENext::MEDIA_TYPE_UNKNOWN;
            meta->pixmap = QPixmap(":/resources/dir.png").scaledToWidth(_thumbSize.width());
            SN_MediaStorage::GlobalMediaList.insert(iter.filePath(), meta);
        }
    }


	// iterate over $(HOME)/.sagenext/media and build hash

	// also iterate over $(HOME)/.sagenext/sessions and build hash for the saved sessions too

	return numread;
}

void SN_MediaStorage::addNewMedia(int mtype, const QString &filepath) {
    //
    // Check if this media is already exist in the storage
    //
    if ( SN_MediaStorage::GlobalMediaList.find(filepath) != SN_MediaStorage::GlobalMediaList.end()) {
        qDebug() << "SN_MediaStorage::addNewMedia() : " << filepath << "already exist in the list";
        return;
    }

    MediaMetaData* meta = new MediaMetaData;
    meta->type = (SAGENext::MEDIA_TYPE)mtype;
    meta->pixmap = _createThumbnail(filepath, (SAGENext::MEDIA_TYPE)mtype);

    SN_MediaStorage::MediaListRWLock.lockForWrite();
    SN_MediaStorage::GlobalMediaList.insert(filepath, meta);
    SN_MediaStorage::MediaListRWLock.unlock();

    emit newMediaAdded();
}


QMap<QString,MediaMetaData*> SN_MediaStorage::getMediaListInDir(const QDir &dir) {
    // check the dir recursively and build the list of SN_MediaItem
    // return the list
    // The list will be owned by SN_MediaBrowser so don't delete the list

//    qDebug() << "SN_MediaStorage::getMediaListInDir()" << dir;

    QMap<QString, MediaMetaData*> itemsInDir;

    QMap<QString, MediaMetaData*>::iterator iter = SN_MediaStorage::GlobalMediaList.begin();
    
    SN_MediaStorage::MediaListRWLock.lockForRead();
    
    // for each item in the global list
    for (; iter!=SN_MediaStorage::GlobalMediaList.end(); iter++) {

        // skip the dir
        if (iter.key() == dir.path()) {
            continue;
        }
        
        // if the item is in the dir
        if ( iter.key().startsWith(dir.path(), Qt::CaseSensitive) ) {
            qDebug() << "SN_MediaStorage::getMediaListInDir() : " << iter.key();
            if (iter.value()->pixmap.isNull()) {
                qDebug() << "\t Has no thumbnail";
            }
            itemsInDir.insert(iter.key(), iter.value());
        }
    }

    SN_MediaStorage::MediaListRWLock.unlock();
    
    return itemsInDir;
}



QPixmap SN_MediaStorage::_createThumbnail(const QString &fpath, SAGENext::MEDIA_TYPE mtype) {
    //
    // Assume the file exists and valid
    //

    QPixmap pixmap;
    bool saveThumbnail = false;

    //
    // check if there's a thumbnail for this file
    //
    QFileInfo fi(fpath);
    QString thumbpath = QDir::homePath() + "/.sagenext/.thumbnails/.thumb." + fi.fileName() + ".jpg";

    if (pixmap.load(thumbpath)) {
        qDebug() << "SN_MediaStorage::_createThumbnail() : loading existing thumbnail : " << thumbpath;
    }

    // Create one
    else {
        qDebug() << "SN_MediaStorage::_createThumbnail() : creating thumbnail for : " << thumbpath;
        if (mtype == SAGENext::MEDIA_TYPE_PDF) {
            saveThumbnail = _readPDF(fpath, pixmap);
        }
        else if (mtype == SAGENext::MEDIA_TYPE_IMAGE) {
            saveThumbnail = _readImage(fpath, pixmap);
        }
        else if (mtype == SAGENext::MEDIA_TYPE_LOCAL_VIDEO) {
            saveThumbnail = _readVideo(fpath, pixmap);
        }
        else if (mtype == SAGENext::MEDIA_TYPE_PLUGIN) {
            saveThumbnail = _readPlugin(fpath, pixmap);
        }
    }

    //
    // Rescale it
    //
    if (!pixmap.isNull()  && pixmap.width() != _thumbSize.width()) {

        pixmap = pixmap.scaled(
                    _settings->value("gui/mediathumbnailwidth", 128).toInt()
                    ,_settings->value("gui/mediathumbnailwidth",128).toInt()
                    ,Qt::IgnoreAspectRatio
                    ,Qt::FastTransformation);

        /*
        if (pixmap.width() >= pixmap.height()  &&  pixmap.width() > _thumbSize.width()) {
            pixmap = pixmap.scaledToWidth(_thumbSize.width());
        }
        else if (pixmap.width() < pixmap.height()  &&  pixmap.height() > _thumbSize.width()) {
            pixmap = pixmap.scaledToHeight(_thumbSize.width());
        }
        */
    }


    //
    // Save to a file
    //
    if (!pixmap.isNull() && saveThumbnail) {
        // Save thumbnail to current directory as a JPG
        if( !pixmap.save(thumbpath, "JPG") )
            qWarning("%s::%s() : Couldn't save the thumbnail to %s",metaObject()->className(), __FUNCTION__, qPrintable(thumbpath));
    }

    return pixmap;
}

bool SN_MediaStorage::_readPlugin(const QString &filename, QPixmap& pixmap) {
    if ( ! pixmap.load(filename + ".picon") ) {
        // load default icon image for a plugin
        pixmap.load(":/resources/plugin_200x200.png");
        return false;
    }
    else {
        return true;
    }
}


bool SN_MediaStorage::_readImage(const QString &filepath,  QPixmap& pixmap) {
    // Create new thumbnail
    if (!pixmap.load(filepath)) {
        qWarning("%s::%s() : Couldn't create the thumbnail for %s",metaObject()->className(), __FUNCTION__, qPrintable(filepath));
        return false;
    }
    else {
        return true;
    }
}

bool SN_MediaStorage::_readPDF(const QString &fpath, QPixmap& pixmap) {
    /*
    Poppler::Document *_document = 0;
    _document = Poppler::Document::load(fpath);

    bool ret = false;

    if (!_document || _document->isLocked()) {
        qCritical("%s::%s() : Couldn't open pdf file %s", metaObject()->className(), __FUNCTION__, qPrintable(fpath));
        ret = false;
    } else {
        Poppler::Page *_currentPage = _document->page(0);
        pixmap = QPixmap::fromImage(_currentPage->renderToImage(200,200), Qt::AutoColor | Qt::OrderedDither);

        if (!pixmap.isNull()) ret = true;

        _currentPage->~Page();
    }

    if (_document) _document->~Document();

    return ret;
    */
    return false;
}

bool SN_MediaStorage::_readVideo(const QString &filepath, QPixmap & pixmap) {
    QString program = "mplayer";
    QStringList arguments;
    // Create a jpeg quietly, no sound, getting a single frame (-frames 1) 5 seconds into the video (-ss 5)
    arguments << "-vo" << "jpeg" << "-quiet" << "-nosound" << "-frames" << "1" << "-ss" << "5" << filepath;

    // Apparently running the above command  will create an image called '00000001.jpg' in the build directory
//    QFileInfo thumbLocation = QFileInfo("$${BUILD_DIR}");
    //
    // This is because QApplicatin's current directory is happen to be the same as $${BUILD_DIR} defined in thewall.pro -- Sungwon
    //


    //
    // run mplayer
    //
    QProcess::execute(program, arguments);
//    QProcess* proc = new QProcess;
//    proc->start(program, arguments, QIODevice::ReadOnly);
//    if (proc->waitForFinished()) {
        if (!pixmap.load("00000001.jpg")) {
            qWarning("%s::%s() : Couldn't create the thumbnail for %s", metaObject()->className(), __FUNCTION__, qPrintable(filepath));
//            delete proc;
            return false;
        }
//    }

//    delete proc;

        //
        // delete 00000001.jpg
        //
        QProcess::execute("rm -f 00000001.jpg");

    return true;
}


