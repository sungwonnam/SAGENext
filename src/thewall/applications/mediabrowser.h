#ifndef MEDIABROWSER_H
#define MEDIABROWSER_H

#include "base/basewidget.h"

#include <QtGui>
#include <QtCore>

class SN_Launcher;
class SN_MediaStorage;

/**
  pointer click will trigger SAGENextLaunch::launch()
  */
class MediaItem : public QGraphicsPixmapItem {
public:
    MediaItem(const QString &filename, const QPixmap &pixmap, QGraphicsItem *parent=0);

private:
    QString _filename;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

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
	  User applications have UserType + 12

	  Items reside between UserType < x < UserType + 12 are items that inherits SN_BaseWidget but not a user application.
	  */
	enum { Type = QGraphicsItem::UserType + 11 };
    virtual int type() const { return Type;}



    /**
      This slot can be access by SAGENextLauncher when a user uploaded a new media file
      There can be multiple mediaBrowsers so use rwlock to access hash.
      */
    static int insertNewMediaToHash(const QString &key, QPixmap &pixmap);

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

};

#endif // MEDIABROWSER_H
