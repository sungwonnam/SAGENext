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
    SN_MediaItem(SAGENext::MEDIA_TYPE mtype, const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent=0);

    inline SAGENext::MEDIA_TYPE mediaType() {return _mediaType;}

    inline QString absFilePath() const {return _filename;}

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
    void clicked(SAGENext::MEDIA_TYPE mtype, const QString &filename);


public slots:
    inline void handlePointerClick() {emit clicked(_mediaType, _filename);}
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
    QList<SN_PixmapButton *> _rootIcons;

    /*!
      This layout contains the rootIcons
      */
    QGraphicsLinearLayout* _rootWindowLayout;

    /*!
      When a media thumbnail panel is displaying (not the rootWindow)
      This icon should be displayed on the left of the panel.
      Upon clicking, rootWindow will show
      */
    SN_PixmapButton* _goBackToRootWindowBtn;

    /**
      draw thumbnails of media in the currentDirectory
      */
//    void attachItems();

    /*!
     * \brief attachRootIcons
     * Attach icons for media (video, image, pdf) on the top of the root window.
     * This function is called once when the SN_MediaBrowser launches.
     */
    QGraphicsLinearLayout* _attachRootIconsForMedia();

    /*!
     * \brief _attachRootIconsForApps
     * Attach icons for apps (plugins, webbrowser, genovis, ..) on the bottom of the root window.
     * This function needs to retrive plugins added by the SN_Launcher.
     */
    QGraphicsLinearLayout *_attachRootIconsForApps();


public slots:
    void mediaStorageHasNewMedia();

	void launchMedia(SAGENext::MEDIA_TYPE, const QString &filename);

    /*!
     * \brief changeDirectory
     * \param dirName
     *
     * Hide root icons,
     * Display thumbnails of the media in the dir
     * Show _goBackToRootWindowBtn
     */
    void changeDirectory(const QString &dir);

    /*!
     * \brief displayRootWindow
     * This will display the initial window (root window) of the MediaBrowser
     */
    void displayRootWindow();


    /*!
     * \brief videoIconClicked
     * A user clickec the video icon on the root window
     */
    inline void videoIconClicked() {changeDirectory(QDir::homePath() + "/.sagenext/media/video");}

    inline void imageIconClicked() {changeDirectory(QDir::homePath() + "/.sagenext/media/image");}

    inline void pdfIconClicked() {changeDirectory(QDir::homePath() + "/.sagenext/media/pdf");}


};

#endif // MEDIABROWSER_H
