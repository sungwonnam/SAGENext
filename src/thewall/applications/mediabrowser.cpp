#include "mediabrowser.h"
#include "../sagenextlauncher.h"


QReadWriteLock MediaBrowser::mediaHashRWLock;
QHash<QString,QPixmap> MediaBrowser::mediaHash;





MediaItem::MediaItem(const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent)
    : QGraphicsPixmapItem(pixmap, parent)
    , _filename(filename)
{
}

void MediaItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
}

void MediaItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    MediaBrowser *browser = static_cast<MediaBrowser *>(parentWidget());
    Q_ASSERT(browser);
    Q_ASSERT(browser->launcher());
    browser->launcher()->launch(MEDIA_TYPE_IMAGE, _filename);
}







MediaBrowser::MediaBrowser(SAGENextLauncher *launcher, quint64 globalappid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)

    : BaseWidget(globalappid, s, parent, wflags)
    , _HScrollBar(new QScrollBar(Qt::Horizontal))
    , _VScrollBar(new QScrollBar(Qt::Vertical))
    , _launcher(launcher)
    , _numRow(10)
    , _numCol(10)
{
    setWidgetType(BaseWidget::Widget_Misc);

    _currentDirectory.setPath(QDir::homePath().append("/.sagenext"));
}

MediaBrowser::~MediaBrowser() {

}

int MediaBrowser::insertNewMediaToHash(const QString &key, QPixmap &pixmap) {
    MediaBrowser::mediaHashRWLock.lockForWrite();
    MediaBrowser::mediaHash.insert(key, pixmap);
    MediaBrowser::mediaHashRWLock.unlock();
}

void MediaBrowser::attachItems() {

    // detach all media items
    foreach (QGraphicsItem *item, childItems()) {
        MediaItem *mitem = dynamic_cast<MediaItem *>(item);
        if (mitem) {
            delete mitem;
        }
    }


    MediaBrowser::mediaHashRWLock.lockForRead();

    foreach (QFileInfo fileinfo, _currentDirectory.entryInfoList(QDir::Files | QDir::Dirs)) {

        if (fileinfo.isDir()) {
            // directory icon
        }
        else if (fileinfo.isFile()) {
            QPixmap pm = MediaBrowser::mediaHash.value(fileinfo.absoluteFilePath());
            MediaItem *item = new MediaItem(fileinfo.absoluteFilePath(), pm.scaled(256, 256), this);
//            item->moveBy(x, y);
        }

    }

    MediaBrowser::mediaHashRWLock.unlock();
}

void MediaBrowser::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    // paint will just draw windows border for users to move/resize window
}
