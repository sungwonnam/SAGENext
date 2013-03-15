#include "sn_mediabrowser.h"
#include "mediastorage.h"
#include "../sagenextlauncher.h"
#include "../common/commonitem.h"


SN_MediaItem::SN_MediaItem(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPixmap &pixmap, const QSize &thumbsize, QGraphicsItem *parent)
    : SN_PixmapButton(parent)
    , _mediaType(mtype)
    , _filename(filename)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    if (!pixmap.isNull()) {
        setPrimaryPixmap(pixmap, thumbsize);
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
void SN_MediaItem::click() {
	emit clicked(_mediaType, _filename, mapToScene(boundingRect().topLeft()).toPoint());
}








SN_MediaBrowser::SN_MediaBrowser(SN_Launcher *launcher, quint64 globalappid, const QSettings *s, SN_MediaStorage* mediaStorage, QGraphicsItem *parent, Qt::WindowFlags wflags)

    : SN_BaseWidget(globalappid, s, parent, wflags)

    , _launcher(launcher)
    , _mediaStorage(mediaStorage)
    , _goBackToRootWindowBtn(0)
    , _closeButton(0)
    , _goBackToParentDirBtn(0)
    , _leftBtn(0)
    , _rightBtn(0)
    , _numItemsHorizontal(s->value("gui/numthumbnailx", 8).toInt())
    , _numItemsVertical(s->value("gui/numthumbnaily", 4).toInt())
    , _currPage(0)
    , _isRootWindow(true)
{
    setWidgetType(SN_BaseWidget::Widget_GUI);

    // Assumes square thumbnail
    _thumbSize = QSize(_settings->value("gui/mediathumbnailwidth", 128).toInt(), 0); // height is 0. So it's  an empty QSize


    _goBackToRootWindowBtn = new SN_PixmapButton(":/mediabrowser/resources/icon_home_256.png", _thumbSize, QString(), this);
    _goBackToRootWindowBtn->hide();
    QObject::connect(_goBackToRootWindowBtn, SIGNAL(clicked()), this, SLOT(displayRootWindow()));

    /*
    _closeButton = new SN_PixmapButton(":/mediabrowser/resources/icon_close.png", _thumbSize, QString(), this);
    _closeButton->hide();
    QObject::connect(_closeButton, SIGNAL(clicked()), this, SLOT(close()));
    */

    _goBackToParentDirBtn = new SN_PixmapButton(":/resources/black_arrow_left_48x48.png", _thumbSize, QString(), this);
    _goBackToParentDirBtn->hide();
    QObject::connect(_goBackToParentDirBtn, SIGNAL(clicked()), this, SLOT(getParentDir()));



    _leftBtn = new SN_PixmapButton(":/resources/arrow-left-64x64.png", _thumbSize, QString(), this);
    _rightBtn = new SN_PixmapButton(":/resources/arrow-right-64x64.png", _thumbSize, QString(), this);
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
    layout->setSpacing(_thumbSize.width() / 4);

    foreach (SN_PixmapButton* icon, _rootMediaIcons)
        layout->addItem(icon);

    return layout;
}

QGraphicsLinearLayout* SN_MediaBrowser::_getRootAppIconsLayout() {
    // do linear layout or something to arrange them
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setSpacing(_thumbSize.width() / 4);

    foreach (SN_PixmapButton* icon, _rootAppIcons) {
        layout->addItem(icon);
    }
    return layout;
}

void SN_MediaBrowser::_createRootIcons() {
    // video
    SN_PixmapButton *videobutton = new SN_PixmapButton(":/mediabrowser/resources/icon_movies_256.png", _thumbSize, QString(), this);
    QObject::connect(videobutton, SIGNAL(clicked()), this, SLOT(videoIconClicked()));

    // image
    SN_PixmapButton *imagebutton = new SN_PixmapButton(":/mediabrowser/resources/icon_images_256.png", _thumbSize, QString(), this);
    QObject::connect(imagebutton, SIGNAL(clicked()), this, SLOT(imageIconClicked()));

    // pdf
    SN_PixmapButton *pdfbutton = new SN_PixmapButton(":/mediabrowser/resources/icon_pdf_256.png", _thumbSize, QString(), this);
    QObject::connect(pdfbutton, SIGNAL(clicked()), this, SLOT(pdfIconClicked()));

    // plugins
    SN_PixmapButton* plugin = new SN_PixmapButton(":/mediabrowser/resources/icon_plugin_256.png", _thumbSize, QString(), this);
    QObject::connect(plugin, SIGNAL(clicked()), this, SLOT(pluginIconClicked()));

    _rootMediaIcons.push_back(videobutton);
    _rootMediaIcons.push_back(imagebutton);
    _rootMediaIcons.push_back(pdfbutton);
    _rootMediaIcons.push_back(plugin);




    // get the list of plugins from the SN_Launcher
    // and get the icon for each plugin (the icon has to be provided by plugin)

    // But for now (for the SC12)
    // Attach them manually
    SN_MediaItem* webbrowser = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://www.evl.uic.edu", QPixmap(":/mediabrowser/resources/icon_internet2_256.png"), _thumbSize, this);

    // clicking an app icon will launch the application directly
    QObject::connect(webbrowser, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString,QPoint)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString,QPoint)));

    // do the same for
    // google maps, google docs, MandelBrot
    SN_MediaItem* googlemap = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://maps.google.com", QPixmap(":/mediabrowser/resources/icon_maps_256.png"), _thumbSize, this);
    QObject::connect(googlemap, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString,QPoint)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString,QPoint)));


    SN_MediaItem* webgl = new SN_MediaItem(SAGENext::MEDIA_TYPE_WEBURL, "http://webglmol.sourceforge.jp/glmol/viewer.html", QPixmap(":/mediabrowser/resources/icon_webgl_256.png"), _thumbSize,this);
    QObject::connect(webgl, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString,QPoint)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString,QPoint)));


//    SN_MediaItem* mandelbrot = new SN_MediaItem(SAGENext::MEDIA_TYPE_PLUGIN, QDir::homePath()+"/.sagenext/media/plugins/libMandelbrotExamplePlugin.so", QPixmap(":/resources/mandelbrot_128.jpg"),this);
//    QObject::connect(mandelbrot, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString)));

    _rootAppIcons.push_back(webbrowser);
    _rootAppIcons.push_back(googlemap);
    _rootAppIcons.push_back(webgl);
}

/*
void SN_MediaBrowser::resizeEvent(QGraphicsSceneResizeEvent *) {
    _goBackToRootWindowBtn->setPos(32, size().height() - _goBackToRootWindowBtn->size().height()-32);
    if (_closeButton) _closeButton->setPos(_goBackToRootWindowBtn->geometry().x(), _goBackToRootWindowBtn->geometry().y() - _closeButton->size().height());
}
*/


/*!
  Slot called when MediaStorage has new media.
  MediaBrowser will then update itself to account for new media.
  TODO: Specify by user if there are multiple MediaBrowsers?
 */
void SN_MediaBrowser::mediaStorageHasNewMedia(){
    qDebug() << "MediaBrowser has been informed of changes to MediaStorage";
}

void SN_MediaBrowser::launchMedia(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPoint &scenepos) {
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
    _launcher->launch(mtype, filename, scenepos);
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

			if ( iter.value()->pixmap.isNull()) continue;

            //
            // create new SN_MediaItem object
            //
            SN_MediaItem *mitem = new SN_MediaItem(iter.value()->type, iter.key(), iter.value()->pixmap, _thumbSize, this); // is a child of SN_MediaBrowser
			if (mitem) {
            	mitem->hide();

            	// if it's folder
            	if (iter.value()->type == SAGENext::MEDIA_TYPE_UNKNOWN) {
                	QDir dir = QDir(iter.key());
                	mitem->setLabel(dir.dirName());
            	}

            	QObject::connect(mitem, SIGNAL(clicked(SAGENext::MEDIA_TYPE,QString,QPoint)), this, SLOT(launchMedia(SAGENext::MEDIA_TYPE,QString,QPoint)));

            	_currMediaItems.push_back(mitem);
			}
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
    if (_closeButton) _closeButton->hide();
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
    ll->setSpacing(_thumbSize.width() / 2);
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
    
    _goBackToRootWindowBtn->show();
    if (_closeButton) _closeButton->show();

    // unset the rootWindowLayout
    // This will delete the layout item the widget had set.
    setLayout(0);

    //
    // keep recreating layout object
    // because displayRootWindow() called setLayout(0)
    //

    // contains gridlayout and buttonlayout
    QGraphicsLinearLayout* right = new QGraphicsLinearLayout(Qt::Vertical);

    QGraphicsLinearLayout *buttonlayout = 0;
    QGraphicsGridLayout *gridlayout = new QGraphicsGridLayout;

    right->addItem(gridlayout);

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

            right->addItem(buttonlayout);
            right->setAlignment(buttonlayout, Qt::AlignCenter | Qt::AlignBottom);
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
            if (mitem) mitem->show(); // Only this item should be visible

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
    else {
        qDebug() << "SN_MediaBrowser::updateThumbnail() : nothing to display";
    }

    //
    // cd..  button to go to parent directory
    //
    _goBackToParentDirBtn->show();
    if (gridlayout->columnCount() == _numItemsHorizontal)
        gridlayout->addItem(_goBackToParentDirBtn, gridlayout->rowCount(), 0);
    else {
        int row = (gridlayout->rowCount()) ? gridlayout->rowCount() - 1 : 0;
        gridlayout->addItem(_goBackToParentDirBtn, row, gridlayout->columnCount());
    }

    // contains _goBacktoRootWindow icon and right layout
    QGraphicsLinearLayout* mainlayout = new QGraphicsLinearLayout(Qt::Horizontal);
    mainlayout->setSpacing(16);
    mainlayout->addItem(_goBackToRootWindowBtn);
    mainlayout->setAlignment(_goBackToRootWindowBtn, Qt::AlignBottom);
    mainlayout->addItem(right);

    setLayout(mainlayout);
    
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
