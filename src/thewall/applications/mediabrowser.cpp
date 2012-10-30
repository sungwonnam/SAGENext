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
    , _rootWindowLayout(0)
    , _goBackToRootWindowBtn(0)
{
    setWidgetType(SN_BaseWidget::Widget_GUI);

    // complete this !
//    _goBackToRootWindowBtn = new SN_PixmapButton();
    QObject::connect(_goBackToRootWindowBtn, SIGNAL(clicked(int)), this, SLOT(displayRootWindow()));

    _rootWindowLayout = new QGraphicsLinearLayout(Qt::Vertical);
    _rootWindowLayout->setItemSpacing(0, 10);

    _rootWindowLayout->addItem(_attachRootIconsForMedia());
    _rootWindowLayout->addItem(_attachRootIconsForApps());

    displayRootWindow();
}

SN_MediaBrowser::~SN_MediaBrowser() {
//    qDebug() << "SN_MediaBrowser::~SN_MediaBrowser()" << childItems();
}


QGraphicsLinearLayout* SN_MediaBrowser::_attachRootIconsForMedia() {
    // video
    SN_PixmapButton *videobutton = new SN_PixmapButton(":/resources/video.png", 128, QString(), this);
    QObject::connect(videobutton, SIGNAL(clicked(int)), this, SLOT(videoIconClicked()));

    videobutton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // image
    SN_PixmapButton *imagebutton = new SN_PixmapButton(":/resources/image.png", 128, QString(), this);
    QObject::connect(imagebutton, SIGNAL(clicked(int)), this, SLOT(imageIconClicked()));

    // pdf
    SN_PixmapButton *pdfbutton = new SN_PixmapButton(":/resources/pdf.png", 128, QString(), this);
    QObject::connect(pdfbutton, SIGNAL(clicked(int)), this, SLOT(pdfIconClicked()));

    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setSpacing(64);
    layout->addItem(videobutton);
    layout->addItem(imagebutton);
    layout->addItem(pdfbutton);

    _rootIcons.push_back(videobutton);
    _rootIcons.push_back(imagebutton);
    _rootIcons.push_back(pdfbutton);

    return layout;
}

QGraphicsLinearLayout* SN_MediaBrowser::_attachRootIconsForApps() {
    // get the list of plugins from the SN_Launcher
    // and get the icon for each plugin (the icon has to be provided by plugin)


    // But for now (for the SC12)
    // Attach them manually
    SN_MediaItem* webbrowser = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://www.evl.uic.edu", QPixmap(":/resources/webkit_128x128.png"), this);

    // clicking an app icon will launch the application directly
    QObject::connect(webbrowser, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));


    // do the same for
    // google maps, google docs, MandelBrot
    SN_MediaItem* googlemap = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://maps.google.com", QPixmap(":/resources/webkit_128x128.png"), this);
    QObject::connect(googlemap, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));


    SN_MediaItem* mandelbrot = new SN_MediaItem(SAGENext::MEDIA_TYPE_PLUGIN, QDir::homePath()+"/.sagenext/media/plugins/libMandelbrotExamplePlugin.so", QPixmap(":/resources/group_data_128.png"),this);
    QObject::connect(mandelbrot, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));


    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setSpacing(64);
    layout->addItem(webbrowser);
    layout->addItem(googlemap);
    layout->addItem(mandelbrot);

    _rootIcons.push_back(webbrowser);
    _rootIcons.push_back(googlemap);
    _rootIcons.push_back(mandelbrot);

    return layout;
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

        return changeDirectory(filename);
    }

    //
    // Where the widget should be opened ?
    //
    qDebug() << "SN_MediaBrowser::launchMedia() : " << filename;
    _launcher->launch(mtype, filename /* , scenePos */);
}

void SN_MediaBrowser::displayRootWindow() {

    // delete the list of currently displayed
    // actual items are not deleted.
    foreach (SN_MediaItem* item, _currItemsDisplayed) {
        item->close();
        delete item; // comment this out if WA_DeleteOnClose is defined in SN_MediaItem
    }
    _currItemsDisplayed.clear();

    
    if (_goBackToRootWindowBtn) {
        _goBackToRootWindowBtn->hide();
    }
    
    
    //
    // show all the root icons
    //
    foreach(SN_PixmapButton* icon, _rootIcons) {
        icon->show();
    }

    // set the rootWindowLayout
    setLayout(_rootWindowLayout);

    adjustSize();
}

void SN_MediaBrowser::changeDirectory(const QString &dir) {
    qDebug() << "SN_MediaBrowser::changeDirectory() " << dir;

    //
    // sets the current list of items to be displayed
    //
    QMap<const QString, MediaMetaData*> &itemsInCurrDir = _mediaStorage->getMediaListInDir(dir);
    if ( ! itemsInCurrDir.empty()) {
        // clear previous SN_MediaItems
        _currItemsDisplayed.clear();
        
        QMap<const QString, MediaMetaData*>::iterator iter = itemsInCurrDir.begin();
        
        // for each item in the dir
        for (iter; iter!=itemsInCurrDir.end(); iter++) {
            //
            // create new SN_MediaItem object
            //
            SN_MediaItem *mitem = new SN_MediaItem(iter.value()->type, iter.key(), iter.value()->pixmap, this); // is a child of SN_MediaBrowser
            
            QObject::connect(mitem, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));
            
            _currItemsDisplayed.push_back(mitem);
        }

        qDebug() << "SN_MediaBrowser::changeDirectory() : " << itemsInCurrDir.size() << "items in" << dir;
    }

    


    //
    // Hide all the root icons
    //
    foreach(SN_PixmapButton* icon, _rootIcons)
        icon->hide();
    
    QGraphicsLinearLayout* hlayout = 0;
    if (_goBackToRootWindowBtn) {
        _goBackToRootWindowBtn->show();
        hlayout = new QGraphicsLinearLayout(Qt::Horizontal);
        hlayout->addItem(_goBackToRootWindowBtn);
    }

    // unset the rootWindowLayout
    setLayout(0);

    //
    // Given the # of items and the size of the thumbnail
    // determine the size of the panel.
    //
    // Organize items in a grid
    //

    // Also attach an icon (to go back to the rootWindow) on the left

    QGraphicsGridLayout *gridlayout = new QGraphicsGridLayout;

    // this is just temporary. Use grid layout
    QGraphicsLinearLayout *ll = new QGraphicsLinearLayout(Qt::Horizontal);
    ll->setSpacing(64);
    ll->setContentsMargins(32, 32, 32, 32);

    if ( !_currItemsDisplayed.empty()) {
        foreach(SN_MediaItem *item, _currItemsDisplayed) {
//            gridlayout->addItem(item);
            ll->addItem(item);
        }
    }
    
    if (hlayout) {
        hlayout->addItem(ll);
        setLayout(hlayout);
    }
    else {
        setLayout(ll);
    }
    
    adjustSize();
}


void SN_MediaBrowser::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    painter->setBrush(Qt::gray);
    painter->drawRect(boundingRect());
}
