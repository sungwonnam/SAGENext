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

            SN_MediaItem* item = new SN_MediaItem(_findMediaType(iter.filePath()), iter.filePath(), _createThumbnail(iter.filePath()));
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

    SN_MediaItem *newitem = new SN_MediaItem(mtype, filepath, _createThumbnail(filepath));

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

QPixmap SN_MediaStorage::_createThumbnail(const QString &filename) {

    QPixmap pixmap;

    // get a image thumbnail for pdf
    if(filename.endsWith(".pdf")) {
        Poppler::Document *_document = Poppler::Document::load(filename);
        Poppler::Page *p = _document->page(0);
        if (p) {
            pixmap = QPixmap::fromImage(p->renderToImage(64, 64));
        }
        delete p;
        delete _document;
    }
    else {
        pixmap.load(filename);
        /*
        if (image.load(filename)) {
                pixmap.convertFromImage(image);
                return pixmap;
        }
        */
    }

    if (!pixmap.isNull()) {
        return pixmap.scaled(
                _settings->value("gui/mediathumbnailwidth", 256).toInt()
                ,_settings->value("gui/mediathumbnailwidth",256).toInt()
                ,Qt::IgnoreAspectRatio
                ,Qt::FastTransformation);
    }
    else {
        qWarning("%s::%s() : Couldn't create the thumbnail for %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
        // return null pixmap
        return pixmap;
    }
}


