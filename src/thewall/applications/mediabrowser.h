#ifndef MEDIABROWSER_H
#define MEDIABROWSER_H

#include "base/basewidget.h"
#include "../common/commondefinitions.h"

#include <QtGui>
#include <QtCore>

class SN_Launcher;
class SN_MediaStorage;

/**
  pointer click will trigger SN_Launcher::launch()
  */
class MediaItem : public QGraphicsWidget
{
	Q_OBJECT

public:
    MediaItem(const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent=0);

private:
	SAGENext::MEDIA_TYPE _mediaType;

    QString _filename;

	QGraphicsPixmapItem *_thumbnail;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
	void thumbnailClicked(SAGENext::MEDIA_TYPE mediatype, const QString &filename);
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


	/**
	  This is to distinguish user application from other items in the scene.
	  User applications have UserType + BASEWIDGET_USER

	  Items reside between UserType + BASEWIDGET_NONUSER <= x < UserType + BASEWIDGET_USER are items that inherits SN_BaseWidget but should not be regarded as a user application.
	  meaning that the widget shouldn't be affected by the scheduler.
	  */
	enum { Type = QGraphicsItem::UserType + BASEWIDGET_NONUSER };
    virtual int type() const { return Type;}



    /**
      This slot can be access by SAGENextLauncher when a user uploaded a new media file
      There can be multiple mediaBrowsers so use rwlock to access hash.
      */
    static void insertNewMediaToHash(const QString &key, QPixmap &pixmap);

    inline SN_Launcher * launcher() {return _launcher;}

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    static QReadWriteLock mediaHashRWLock;
    static QHash<QString, QPixmap> mediaHash;

    QScrollBar *_HScrollBar;

    QScrollBar *_VScrollBar;

    SN_Launcher *_launcher;

    SN_MediaStorage* _mediaStorage;

    /**
      how many thumbnails
      */
    int _numRow;
    int _numCol;

    /**
      current directory where mediabrowser is in
      */
    QDir _currentDirectory;

    /**
      draw thumbnails of media in the currentDirectory
      */
    void attachItems();

public slots:
    void mediaStorageHasNewMedia();

	void launchMedia(SAGENext::MEDIA_TYPE, const QString &filename);

};

#endif // MEDIABROWSER_H
