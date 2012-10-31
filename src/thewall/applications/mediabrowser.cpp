#include "mediabrowser.h"
#include "mediastorage.h"
#include "../sagenextlauncher.h"
#include "../common/commonitem.h"


//QReadWriteLock SN_MediaBrowser::mediaHashRWLock;
//QHash<QString,QPixmap> SN_MediaBrowser::mediaHash;


SN_MediaItem::SN_MediaItem(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent)
    : SN_PixmapButton(parent)
    , _mediaType(mtype)
    , _filename(filename)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    if (!pixmap.isNull()) {
        setPrimaryPixmap(pixmap);
    }
    else {
        qDebug() << "SN_MediaItem::SN_MediaItem() : Null pixmap";
    }

    /*
    _medianameDisplay = new SN_SimpleTextWidget(12, QColor(Qt::white), QColor(Qt::transparent), this);

    QFileInfo f(_filename);
    _medianameDisplay->setText(f.fileName());
    _medianameDisplay->setX(parent->x());
    */
}








SN_MediaBrowser::SN_MediaBrowser(SN_Launcher *launcher, quint64 globalappid, const QSettings *s, SN_MediaStorage* mediaStorage, QGraphicsItem *parent, Qt::WindowFlags wflags)

    : SN_BaseWidget(globalappid, s, parent, wflags)

    , _launcher(launcher)
    , _mediaStorage(mediaStorage)
//    , _rootWindowLayout(new QGraphicsLinearLayout(Qt::Vertical))
//    , _thumbnailWindowLayout(0)
    , _goBackToRootWindowBtn(0)
    , _goBackToParentDirBtn(0)
    , _leftBtn(0)
    , _rightBtn(0)
    , _numItemsHorizontal(8)
    , _numItemsVertical(8)
    , _currPage(0)
    , _isRootWindow(true)
{
    setWidgetType(SN_BaseWidget::Widget_GUI);

    _goBackToRootWindowBtn = new SN_PixmapButton(":/resources/mediaBrowserRoot.png", 128, QString(), this);
    _goBackToRootWindowBtn->setPos(-1 * _goBackToRootWindowBtn->size().width(), 0);
    _goBackToRootWindowBtn->hide();
    QObject::connect(_goBackToRootWindowBtn, SIGNAL(clicked()), this, SLOT(displayRootWindow()));


    _goBackToParentDirBtn = new SN_PixmapButton(":/resources/dotdotslash_128.png", _settings->value("gui/mediathumbnailwidth", 256).toInt(), QString(), this);
    _goBackToParentDirBtn->hide();
    QObject::connect(_goBackToParentDirBtn, SIGNAL(clicked()), this, SLOT(getParentDir()));



    _leftBtn = new SN_PixmapButton(":/resources/arrow-left-64x64.png", _settings->value("gui/mediathumbnailwidth", 256).toInt(), QString(), this);
    _rightBtn = new SN_PixmapButton(":/resources/arrow-right-64x64.png", _settings->value("gui/mediathumbnailwidth", 256).toInt(), QString(), this);
    QObject::connect(_leftBtn, SIGNAL(clicked()), this, SLOT(prevPage()));
    QObject::connect(_rightBtn, SIGNAL(clicked()), this, SLOT(nextPage()));
    _leftBtn->hide();
    _rightBtn->hide();




    _createRootIcons();

    QGraphicsLinearLayout *ll = new QGraphicsLinearLayout(Qt::Vertical);
    ll->addItem(_getRootMediaIconsLayout());
    ll->addItem(_getRootAppIconsLayout());
    ll->setItemSpacing(0, 10);

    setLayout(ll);
}

SN_MediaBrowser::~SN_MediaBrowser() {
//    qDebug() << "SN_MediaBrowser::~SN_MediaBrowser()" << childItems();
}


QGraphicsLinearLayout* SN_MediaBrowser::_getRootMediaIconsLayout() {
    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setSpacing(64);

    foreach (SN_PixmapButton* icon, _rootMediaIcons)
        layout->addItem(icon);

    return layout;
}

QGraphicsLinearLayout* SN_MediaBrowser::_getRootAppIconsLayout() {
    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setSpacing(64);

    foreach (SN_PixmapButton* icon, _rootAppIcons) {
        layout->addItem(icon);
    }
    return layout;
}

void SN_MediaBrowser::_createRootIcons() {
    // video
    SN_PixmapButton *videobutton = new SN_PixmapButton(":/resources/video.png", 128, QString(), this);
    QObject::connect(videobutton, SIGNAL(clicked()), this, SLOT(videoIconClicked()));

    // image
    SN_PixmapButton *imagebutton = new SN_PixmapButton(":/resources/image.png", 128, QString(), this);
    QObject::connect(imagebutton, SIGNAL(clicked()), this, SLOT(imageIconClicked()));

    // pdf
    SN_PixmapButton *pdfbutton = new SN_PixmapButton(":/resources/pdf.png", 128, QString(), this);
    QObject::connect(pdfbutton, SIGNAL(clicked()), this, SLOT(pdfIconClicked()));

    // plugins
    SN_PixmapButton* plugin = new SN_PixmapButton(":/resources/dir_plugin_128.png", 128, QString(), this);
    QObject::connect(plugin, SIGNAL(clicked()), this, SLOT(pluginIconClicked()));

    _rootMediaIcons.push_back(videobutton);
    _rootMediaIcons.push_back(imagebutton);
    _rootMediaIcons.push_back(pdfbutton);
    _rootMediaIcons.push_back(plugin);




    // get the list of plugins from the SN_Launcher
    // and get the icon for each plugin (the icon has to be provided by plugin)

    // But for now (for the SC12)
    // Attach them manually
    SN_MediaItem* webbrowser = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://www.evl.uic.edu", QPixmap(":/resources/webkit_128x128.png"), this);

    // clicking an app icon will launch the application directly
    QObject::connect(webbrowser, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));

    // do the same for
    // google maps, google docs, MandelBrot
    SN_MediaItem* googlemap = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://maps.google.com", QPixmap(":/resources/googleMaps_128.png"), this);
    QObject::connect(googlemap, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));

    SN_MediaItem* mandelbrot = new SN_MediaItem(SAGENext::MEDIA_TYPE_PLUGIN, QDir::homePath()+"/.sagenext/media/plugins/libMandelbrotExamplePlugin.so", QPixmap(":/resources/mandelbrot_128.png"),this);
    QObject::connect(mandelbrot, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));

    _rootAppIcons.push_back(webbrowser);
    _rootAppIcons.push_back(googlemap);
    _rootAppIcons.push_back(mandelbrot);
}

void SN_MediaBrowser::resizeEvent(QGraphicsSceneResizeEvent *event) {
    // recalculate the number of items displayed
    event->newSize();

    _settings->value("gui/mediathumbnailwidth", 256).toInt();

    // update numItemsHorizontal, numItemsVertical

}

/*
bool SN_MediaBrowser::insertNewMediaToHash(const QString &key, QPixmap &pixmap) {
    if (_mediaStorage) {
        return insertNewMediaToHash(key, pixmap);
    }
    return false;
}
*/


/*
void SN_MediaBrowser::attachItems() {
    attachItemsInCurrDirectory();
    // detach all media items
    foreach (QGraphicsItem *item, childItems()) {
        // cast item both as folder and a media item
        SN_MediaItem *mitem = dynamic_cast<SN_MediaItem *>(item);

        if (mitem) {
            delete mitem;
        }
    }

    QHashIterator<QString, QPixmap> i(_currItemsDisplayed);
    int itemCount = 0;
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());
        // TODO: Change logic to change it so it changes dir when a folder is clicked vs. thumbnail clicked
        QFileInfo p(i.key());
        if(p.isFile()) {
            SN_MediaItem *item = new SN_MediaItem(i.key(), pm, this);

            if ( ! QObject::connect(item, SIGNAL(thumbnailClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString))) ) {
                qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
            }

            // TODO: The xPos of each thumbnail is hardcoded...MUST CHANGE THIS
            if(item->getMediaType() == SAGENext::MEDIA_TYPE_IMAGE) {
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
            else if (item->getMediaType() == SAGENext::MEDIA_TYPE_PDF) {
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
            else if(item->getMediaType() == SAGENext::MEDIA_TYPE_PLUGIN) {
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
            else if(item->getMediaType() == SAGENext::MEDIA_TYPE_LOCAL_VIDEO){
                itemCount++;
                item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
            }
        }
        else if(p.isDir()) {
            FolderItem *item = new FolderItem(i.key(), pm, this);

            if ( ! QObject::connect(item, SIGNAL(folderClicked(QString)), this, SLOT(changeDirectory(QString))) ) {
                qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
            }
            // Logic here for folder count
            itemCount++;
            item->moveBy(pm.size().width() * itemCount,0);
        }
    }
    */

/*
    SN_MediaBrowser::mediaHashRWLock.lockForRead();
    mediaHash = _mediaStorage->getMediaHashForRead();
    QHashIterator<QString, QPixmap> i(mediaHash);
    int itemCount = 0;
    int itemCount = 0;
    int itemCount = 0;
    int itemCount = 0;
    while (i.hasNext()) {
        i.next();
        QPixmap pm = SN_MediaBrowser::mediaHash.value(i.key());

        MediaItem *item = new MediaItem(i.key(), pm, this);

        if ( ! QObject::connect(item, SIGNAL(thumbnailClicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString))) ) {
            qCritical("\n%s::%s() : signal thumbnailClicked is not connected to the slot launchMedia\n", metaObject()->className(), __FUNCTION__);
        }

        // TODO: The xPos of each thumbnail is hardcoded...MUST CHANGE THIS
        if(item->getMediaType() == SAGENext::MEDIA_TYPE_IMAGE) {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 0);
        }
        else if (item->getMediaType() == SAGENext::MEDIA_TYPE_PDF) {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 64);
        }
        else if(item->getMediaType() == SAGENext::MEDIA_TYPE_PLUGIN) {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 128);
        }
        else {
            itemCount++;
            item->moveBy(_settings->value("gui/mediathumbnailwidth",256).toInt() * itemCount, 192);
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

//}

/*!
  Slot called when MediaStorage has new media.
  MediaBrowser will then update itself to account for new media.
  TODO: Specify by user if there are multiple MediaBrowsers?
 */
void SN_MediaBrowser::mediaStorageHasNewMedia(){
    qDebug() << "MediaBrowser has been informed of changes to MediaStorage";
}

void SN_MediaBrowser::launchMedia(SAGENext::MEDIA_TYPE mtype, const QString &filename) {
    Q_ASSERT(_launcher);

    //
    // this is folder
    //
    if (mtype == SAGENext::MEDIA_TYPE_UNKNOWN) {

        //
        // call changeDirectory() with new directory
        //

        _populateMediaItems(QDir(filename));
        return;
    }

    //
    // Where the widget should be opened ?
    //
    qDebug() << "SN_MediaBrowser::launchMedia() : " << filename;
    _launcher->launch(mtype, filename /* , scenePos */);
}

void SN_MediaBrowser::_populateMediaItems(const QDir &dir) {
    if (!_mediaStorage) return;

    _currentDir = dir;

    const QMap<QString, MediaMetaData*> &itemsInCurrDir = _mediaStorage->getMediaListInDir(dir);

    //
    // Hide all the root icons
    //
    foreach(SN_PixmapButton* icon, _rootMediaIcons)
        icon->hide();
    foreach(SN_PixmapButton* icon, _rootAppIcons)
        icon->hide();

    //
    // If the dir has some media item
    //
    if ( ! itemsInCurrDir.empty()) {

        // clear the previous list
        // delete the container only.
        // The actual objects are not deleted.
        foreach (SN_MediaItem* item, _currMediaItems) {
            item->close();
            delete item; // comment this out if WA_DeleteOnClose is defined in SN_MediaItem
        }
        _currMediaItems.clear();

        //
        // re-populate the _currItemDisplayed
        //
        QMap<QString, MediaMetaData*>::const_iterator iter = itemsInCurrDir.begin();

        // for each item in the dir
        for (; iter!=itemsInCurrDir.end(); iter++) {
            //
            // create new SN_MediaItem object
            //
            SN_MediaItem *mitem = new SN_MediaItem(iter.value()->type, iter.key(), iter.value()->pixmap, this); // is a child of SN_MediaBrowser
            mitem->hide();

            // if it's folder
            if (iter.value()->type == SAGENext::MEDIA_TYPE_UNKNOWN) {
                QDir dir = QDir(iter.key());
                mitem->setLabel(dir.dirName());
            }

            QObject::connect(mitem, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));

            _currMediaItems.push_back(mitem);
        }

//        qDebug() << "SN_MediaBrowser::_populateMediaItems() : " << itemsInCurrDir.size() << "items in" << dir;

        //
        // toParentDirectory button
        //

    }

    else {
        // dir is empty
        return;
    }

    _isRootWindow = false;

    //
    // display the thumbnails
    //
    updateThumbnailPanel();
}

void SN_MediaBrowser::displayRootWindow() {

    // delete the list of currently displayed
    // actual items are not deleted.
    foreach (SN_MediaItem* item, _currMediaItems) {
        item->close();
        delete item; // comment this out if WA_DeleteOnClose is defined in SN_MediaItem
    }
    _currMediaItems.clear();

    _goBackToRootWindowBtn->hide();
    _goBackToParentDirBtn->hide();
    _leftBtn->hide();
    _rightBtn->hide();


    
    // previous layout will be deleted.
    setLayout(0);

    //
    // show all the root icons
    //
    foreach(SN_PixmapButton* icon, _rootMediaIcons) {
        icon->show();
    }
    foreach(SN_PixmapButton* icon, _rootAppIcons) {
        icon->show();
    }

    QGraphicsLinearLayout* ll = new QGraphicsLinearLayout(Qt::Vertical);
    ll->addItem(_getRootMediaIconsLayout());
    ll->addItem(_getRootAppIconsLayout());

    // set the rootWindowLayout
    setLayout(ll);

//    qDebug() << "SN_MediaBrowser::displayRootWindow()" << layout()->geometry() << layout()->contentsRect();

    adjustSize();

    // reset page number for thumbnail display
    _currPage = 0;

    _isRootWindow = true;
}

void SN_MediaBrowser::updateThumbnailPanel() {
    if (_isRootWindow) return;
    _isRootWindow = false;
    
    if (_goBackToRootWindowBtn) {
        _goBackToRootWindowBtn->show();
    }

    // unset the rootWindowLayout
    // This will delete the layout item the widget had set.
    setLayout(0);

    //
    // keep recreating layout object
    // because displayRootWindow() called setLayout(0)
    //
    QGraphicsLinearLayout* ll = new QGraphicsLinearLayout(Qt::Vertical);
    QGraphicsLinearLayout *buttonlayout = 0;
    QGraphicsGridLayout *gridlayout = new QGraphicsGridLayout;


    //
    // Assume _currMediaItems is sorted
    //
    if ( !_currMediaItems.empty()) {

        int i=0,j=0;
        int numItemsPerPages = _numItemsHorizontal * _numItemsVertical;
        int numPages = 1;
        if (numItemsPerPages > 0) {
            numPages = ceil((qreal)_currMediaItems.size() / (qreal)numItemsPerPages);
        }

        if (numPages > 1) {
            // attach arrow bar on the bottom
            buttonlayout = new QGraphicsLinearLayout(Qt::Horizontal);
            buttonlayout->addItem(_leftBtn);
            buttonlayout->addItem(_rightBtn);
            _leftBtn->show();
            _rightBtn->show();
        }

        QList<SN_MediaItem*>::iterator it = _currMediaItems.begin();
        for (;it!=_currMediaItems.end();it++) {
            (*it)->hide(); // hide all the item initially
        }
        it = _currMediaItems.begin(); // reset the iterator

        // currPage starts with 0
        // skip some items based on _currPage * numItemsPerPages
        if (_currPage > 0) {
//            qDebug() << "_currPage" << _currPage << "skipping" << _currPage * numItemsPerPages << "items";
            it += (_currPage * numItemsPerPages);
        }

        SN_MediaItem *mitem= 0;
        for (; it!=_currMediaItems.end(); it++) {
            mitem = (*it);
//            qDebug() << "adding item at" << i << j;
            gridlayout->addItem(mitem, i, j);
            mitem->show(); // Only this item should be visible

            // finished adding items for this page
            if (gridlayout->count() == numItemsPerPages)
                break;

            // update row, column index
            ++j;
            if (j == _numItemsHorizontal) {
                ++i;
                j = 0;
            }
        }
//        qDebug() << "updateThumbnail() : " << gridlayout->count();
    }

    //
    // cd..  button to go to parent directory
    //
    _goBackToParentDirBtn->show();
    if (gridlayout->columnCount() == _numItemsHorizontal)
        gridlayout->addItem(_goBackToParentDirBtn, gridlayout->rowCount(), 0);
    else {
        gridlayout->addItem(_goBackToParentDirBtn, gridlayout->rowCount()-1, gridlayout->columnCount());
    }

    
    ll->addItem(gridlayout);
    if (buttonlayout)
        ll->addItem(buttonlayout);

    setLayout(ll);
    
    adjustSize();
}

void SN_MediaBrowser::prevPage() {
    if (_currPage == 0) return;
    --_currPage;
    updateThumbnailPanel();
}

void SN_MediaBrowser::nextPage() {
    int numItemsPerPages = _numItemsHorizontal * _numItemsVertical;
    int numPages = ceil((qreal)_currMediaItems.size() / (qreal)numItemsPerPages);
    if (_currPage == numPages - 1) return;
    ++_currPage;
    updateThumbnailPanel();
}

void SN_MediaBrowser::getParentDir() {
    if (_currentDir.cdUp()) {
        _populateMediaItems(_currentDir);
    }
}

void SN_MediaBrowser::sort(int sortmode) {
    // do sort
    
    updateThumbnailPanel();
}

void SN_MediaBrowser::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

//    painter->setBrush(Qt::gray);
//    painter->drawRect(boundingRect());
}
