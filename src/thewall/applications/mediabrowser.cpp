#include "mediabrowser.h"
#include "mediastorage.h"
#include "../sagenextlauncher.h"


QReadWriteLock SN_MediaBrowser::mediaHashRWLock;
QHash<QString,QPixmap> SN_MediaBrowser::mediaHash;





MediaItem::MediaItem(const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _filename(filename)
    , _thumbnail(0)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    _thumbnail = new QGraphicsPixmapItem(pixmap, this);

    Q_ASSERT(_thumbnail);

    //
    // because MediaItem should receive mouse event not the thumbnail
    //
    _thumbnail->setFlag(QGraphicsItem::ItemStacksBehindParent);
    setMediaType();
    resize(_thumbnail->boundingRect().size());
}

void MediaItem::mousePressEvent(QGraphicsSceneMouseEvent *) {
}

void MediaItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    /*
    SN_MediaBrowser *browser = static_cast<SN_MediaBrowser *>(parentWidget());
    Q_ASSERT(browser);
    Q_ASSERT(browser->launcher());
    browser->launcher()->launch(SAGENext::MEDIA_TYPE_IMAGE, _filename);
    */
    emit thumbnailClicked(_mediaType, _filename);
}

void MediaItem::setMediaType() {
    // prob not the best way to do this
    if(_filename.endsWith(".pdf")) {
        _mediaType = SAGENext::MEDIA_TYPE_PDF;
    }
    else if(_filename.endsWith(".so")) {
        _mediaType = SAGENext::MEDIA_TYPE_PLUGIN;
    }
    else if(_filename.endsWith(".mp4") || _filename.endsWith(".avi")) {
        _mediaType = SAGENext::MEDIA_TYPE_VIDEO;
    }
    else { // Else it's a picture
        _mediaType = SAGENext::MEDIA_TYPE_IMAGE;
    }
}

int MediaItem::getMediaType() {
    return _mediaType;
}





SN_MediaBrowser::SN_MediaBrowser(SN_Launcher *launcher, quint64 globalappid, const QSettings *s, SN_MediaStorage* mediaStorage, QGraphicsItem *parent, Qt::WindowFlags wflags)

    : SN_BaseWidget(globalappid, s, parent, wflags)
    , _HScrollBar(new QScrollBar(Qt::Horizontal))
    , _VScrollBar(new QScrollBar(Qt::Vertical))
    , _launcher(launcher)
    , _mediaStorage(mediaStorage)
    , _numRow(4)
{
    setWidgetType(SN_BaseWidget::Widget_GUI);
    qDebug("%s::%s() : called.", metaObject()->className(), __FUNCTION__);

    _currentDirectory.setPath(QDir::homePath().append("/.sagenext"));

    connect( _mediaStorage, SIGNAL(newMediaAdded()), this, SLOT(mediaStorageHasNewMedia()) );

    attachItemsInCurrDirectory();
    attachItems();

    _numCol = mediaHash.size();
    //resize to number of media items
    resize(256 * 10 ,  4 * 256);

    setContentsMargins(1,1,1,1);
}

SN_MediaBrowser::~SN_MediaBrowser() {

}

void SN_MediaBrowser::insertNewMediaToHash(const QString &key, QPixmap &pixmap) {
    SN_MediaBrowser::mediaHashRWLock.lockForWrite();
    SN_MediaBrowser::mediaHash.insert(key, pixmap);
    SN_MediaBrowser::mediaHashRWLock.unlock();
}

void SN_MediaBrowser::attachItemsInCurrDirectory() {
    QFileInfoList list = _currentDirectory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    QListIterator<QFileInfo> i(list);
    QString path;

    currentItemsDisplayed.clear();

    SN_MediaBrowser::mediaHashRWLock.lockForRead();
    mediaHash = _mediaStorage->getMediaHashForRead();

    while (i.hasNext()) {
        path = i.next().absoluteFilePath();
        currentItemsDisplayed.insert(path, mediaHash.value(path));
    }
    SN_MediaBrowser::mediaHashRWLock.unlock();

}

void SN_MediaBrowser::attachItems() {

    // detach all media items
    foreach (QGraphicsItem *item, childItems()) {
        MediaItem *mitem = dynamic_cast<MediaItem *>(item);
        if (mitem) {
            delete mitem;
        }
    }

    QHashIterator<QString, QPixmap> i(currentItemsDisplayed);
    int imageCount = 0;
    int pluginCount = 0;
    int videoCount = 0;
    int pdfCount = 0;
    int folderCount = 0;
    while (i.hasNext()) {
        i.next();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());

        MediaItem *item = new MediaItem(i.key(), pm, this);

        // TODO: Change logic to change it so it changes dir when a folder is clicked vs. thumbnail clicked
        if ( ! QObject::connect(item, SIGNAL(thumbnailClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString))) ) {
            qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
        }

        // TODO: The xPos of each thumbnail is hardcoded...MUST CHANGE THIS
        if(item->getMediaType() == SAGENext::MEDIA_TYPE_IMAGE) {
            imageCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * imageCount, 0);
        }
        else if (item->getMediaType() == SAGENext::MEDIA_TYPE_PDF) {
            pdfCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * pdfCount, 64);
        }
        else if(item->getMediaType() == SAGENext::MEDIA_TYPE_PLUGIN) {
            pluginCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * pluginCount, 128);
        }
        else if(item->getMediaType() == SAGENext::MEDIA_TYPE_LOCAL_VIDEO){
            videoCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * videoCount, 192);
        }
        // Logic here for folder count
        else {
            folderCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * videoCount, 192);
        }
    }

/*
    SN_MediaBrowser::mediaHashRWLock.lockForRead();
    mediaHash = _mediaStorage->getMediaHashForRead();
    QHashIterator<QString, QPixmap> i(mediaHash);
    int imageCount = 0;
    int pluginCount = 0;
    int videoCount = 0;
    int pdfCount = 0;
    while (i.hasNext()) {
        i.next();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());

        MediaItem *item = new MediaItem(i.key(), pm, this);

        if ( ! QObject::connect(item, SIGNAL(thumbnailClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString))) ) {
            qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
        }

        // TODO: The xPos of each thumbnail is hardcoded...MUST CHANGE THIS
        if(item->getMediaType() == SAGENext::MEDIA_TYPE_IMAGE) {
            imageCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * imageCount, 0);
        }
        else if (item->getMediaType() == SAGENext::MEDIA_TYPE_PDF) {
            pdfCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * pdfCount, 64);
        }
        else if(item->getMediaType() == SAGENext::MEDIA_TYPE_PLUGIN) {
            pluginCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * pluginCount, 128);
        }
        else {
            videoCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * videoCount, 192);
        }
    }

//    foreach (QFileInfo fileinfo, _currentDirectory.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDot)) {

//        if (fileinfo.isDir()) {
//            // directory icon
//        }
//        else if (fileinfo.isFile()) {
//            QPixmap pm = MediaBrowser::mediaHash.value(fileinfo.absoluteFilePath());
//            MediaItem *item = new MediaItem(fileinfo.absoluteFilePath(), pm.scaled(256, 256), this);
////            item->moveBy(x, y);
//        }

//    }

    SN_MediaBrowser::mediaHashRWLock.unlock();
*/

}

void SN_MediaBrowser::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    // paint will just draw windows border for users to move/resize window

    // To be able to do that, resize() must be called with correct value

    SN_BaseWidget::paint(painter, option, widget);

}

/*!
  Slot called when MediaStorage has new media.
  MediaBrowser will then update itself to account for new media.
  TODO: Specify by user if there are multiple MediaBrowsers?
 */
void SN_MediaBrowser::mediaStorageHasNewMedia(){
    qDebug() << "MediaBrowser has been informed of changes to MediaStorage";
    attachItems();
}

void SN_MediaBrowser::launchMedia(SAGENext::MEDIA_TYPE mtype, const QString &filename) {
    Q_ASSERT(_launcher);
    _launcher->launch(mtype, filename);
}
