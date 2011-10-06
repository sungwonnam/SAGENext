#include "mediabrowser.h"
#include "mediastorage.h"
#include "../sagenextlauncher.h"


QReadWriteLock SN_MediaBrowser::mediaHashRWLock;
QHash<QString,QPixmap> SN_MediaBrowser::mediaHash;





MediaItem::MediaItem(const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent)
    : QGraphicsPixmapItem(pixmap, parent)
    , _filename(filename)
{
}

void MediaItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
}

void MediaItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    SN_MediaBrowser *browser = static_cast<SN_MediaBrowser *>(parentWidget());
    Q_ASSERT(browser);
    Q_ASSERT(browser->launcher());
    browser->launcher()->launch(SAGENext::MEDIA_TYPE_IMAGE, _filename);
}








SN_MediaBrowser::SN_MediaBrowser(SN_Launcher *launcher, quint64 globalappid, const QSettings *s, SN_MediaStorage* mediaStorage, QGraphicsItem *parent, Qt::WindowFlags wflags)

    : SN_BaseWidget(globalappid, s, parent, wflags)
    , _HScrollBar(new QScrollBar(Qt::Horizontal))
    , _VScrollBar(new QScrollBar(Qt::Vertical))
    , _launcher(launcher)
    , _mediaStorage(mediaStorage)
    , _numRow(10)
    , _numCol(10)
{
    setWidgetType(SN_BaseWidget::Widget_Misc);

    qDebug("%s::%s() : called.", metaObject()->className(), __FUNCTION__);

    _currentDirectory.setPath(QDir::homePath().append("/.sagenext"));

    connect( _mediaStorage, SIGNAL(newMediaAdded()), this, SLOT(mediaStorageHasNewMedia()) );
}

SN_MediaBrowser::~SN_MediaBrowser() {

}

int SN_MediaBrowser::insertNewMediaToHash(const QString &key, QPixmap &pixmap) {
    SN_MediaBrowser::mediaHashRWLock.lockForWrite();
    SN_MediaBrowser::mediaHash.insert(key, pixmap);
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


    SN_MediaBrowser::mediaHashRWLock.lockForRead();
    mediaHash = _mediaStorage->getMediaHash();
    QHashIterator<QString, QPixmap> i(mediaHash);
    int count = 0;
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());

        MediaItem *item = new MediaItem(i.key(), pm.scaled(256, 256), this);
        item->moveBy(256 * count, 0);
        count++;
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
}

void SN_MediaBrowser::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    // paint will just draw windows border for users to move/resize window
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
