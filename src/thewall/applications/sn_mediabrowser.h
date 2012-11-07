#ifndef MEDIABROWSER_H
#define MEDIABROWSER_H

#include "base/basewidget.h"
#include "../common/commondefinitions.h"
#include "../common/commonitem.h"

#include <QtGui>
#include <QtCore>

class SN_Launcher;
class SN_MediaStorage;

/*!
 * MediaItem is a child of SN_PixmapButton.
 */
class SN_MediaItem : public SN_PixmapButton
{
	Q_OBJECT

public:
    SN_MediaItem(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPixmap &pixmap, const QSize& thumbsize, QGraphicsItem *parent=0);

    inline SAGENext::MEDIA_TYPE mediaType() {return _mediaType;}

    inline QString absFilePath() const {return _filename;}


    void click();

private:
	SAGENext::MEDIA_TYPE _mediaType;

    /*!
     * \brief _filename is an absolute filepath to the media or directory
     */
    QString _filename;

    SN_SimpleTextWidget* _medianameDisplay;

signals:
    /*!
     * \brief clicked
     * \param filename can be a filepath to the media or plugin
     */
    void clicked(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPoint &scenepos);

};









/**
  MediaBrowser will have MediaItem instances as its children
  */
class SN_MediaBrowser : public SN_BaseWidget
{
    Q_OBJECT
public:
    explicit SN_MediaBrowser(SN_Launcher *launcher, quint64 globalappid, const QSettings *s, SN_MediaStorage* mediaStorage, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = Qt::Window);
    ~SN_MediaBrowser();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    
protected:
    /*!
      reimplementing SN_BaseWidget::mouseDoubleClickEvent() 
      in order to prevent SN_MediaBrowser to be maximized accidently
      */
    inline void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {/* do nothing */}

    /*!
     * \brief resizeEvent
     * \param event
     * If the thumbnail display panel is resized,
     * the number of thumbnails displayed is changed.
     */
//    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    /*!
     * \brief currentMediaItems is the list of SN_MediaItem thumbnails that are being displayed.
     */
    QList<SN_MediaItem *> _currMediaItems;

    SN_Launcher* _launcher;

    SN_MediaStorage* _mediaStorage;

    /*!
     * \brief _rootIcons is a list of icons on the root window
     */
    QList<SN_PixmapButton *> _rootMediaIcons;
    QList<SN_PixmapButton *> _rootAppIcons;

    /*!
      When a media thumbnail panel is displaying (not the rootWindow)
      This icon should be displayed on the left of the panel.
      Upon clicking, rootWindow will show
      */
    SN_PixmapButton* _goBackToRootWindowBtn;

    /*!
     * \brief _closeButton closes mediabrowser upon clicking
     */
    SN_PixmapButton* _closeButton;

    SN_PixmapButton* _goBackToParentDirBtn;

    SN_PixmapButton* _leftBtn;

    SN_PixmapButton* _rightBtn;

    QSize _thumbSize;

    /*!
      How many columns in the grid layout for the thumbnail view
      */
    int _numItemsHorizontal;

    /*!
      How many row in the grid layout for the thumbnail view
      */
    int _numItemsVertical;

    /*!
      The grid thumbnail view may not show all the items in the _currMediaItems
      meaning the thumbnail window can have multiple pages
      */
    int _currPage;

    /*!
      true if I'm displaying root icons (no thumbnail view)
      */
    bool _isRootWindow;

    QDir _currentDir;

    /*!
     * \brief attachRootIcons
     * Attach icons for media (video, image, pdf) on the top of the root window.
     * This function is called once when the SN_MediaBrowser launches.
     */
    QGraphicsLinearLayout* _getRootMediaIconsLayout();

    /*!
     * \brief _attachRootIconsForApps
     * Attach icons for apps (plugins, webbrowser, genovis, ..) on the bottom of the root window.
     * This function needs to retrive plugins added by the SN_Launcher.
     */
    QGraphicsLinearLayout *_getRootAppIconsLayout();

    void _createRootIcons();


    /*!
     * \brief _populateMediaItems
     * \param dir to search for
     * \return the number of items in the dir
     *
     * This function is called when a user clicked media icon on the root window
     */
    void _populateMediaItems(const QDir &dir);

public slots:
    void mediaStorageHasNewMedia();

	void launchMedia(SAGENext::MEDIA_TYPE, const QString &filename, const QPoint &scenepos);

    /*!
     * \brief changeDirectory
     * \param dirName
     *
     * Hide root icons,
     * Display thumbnails of the media in the dir.
     * Show _goBackToRootWindowBtn
     */
    void updateThumbnailPanel();

    /*!
     * \brief displayRootWindow
     * This will display the initial window (root window) of the MediaBrowser
     */
    void displayRootWindow();

    void prevPage();
    void nextPage();


    /*!
     * \brief videoIconClicked
     * If a user clicks the video icon on the root window then
     * populates media items from the media storage.
     *
     * _populateMediaItems() will call updateThumbnailPanel()
     */
    inline void videoIconClicked() {_populateMediaItems(QDir::homePath() + "/.sagenext/media/video");}
    inline void imageIconClicked() {_populateMediaItems(QDir::homePath() + "/.sagenext/media/image");}
    inline void pdfIconClicked() {_populateMediaItems(QDir::homePath() + "/.sagenext/media/pdf");}
    inline void pluginIconClicked() {_populateMediaItems(QDir::homePath() + "/.sagenext/media/plugins");}

    /*!
      This function is called upon clicking the dotdotslash icon in a thumbnail view
      */
    void getParentDir();
    
    /*!
      sort the media item in the _currMediaItem list
      */
    void sort(int sortmode);
};

#endif // MEDIABROWSER_H
