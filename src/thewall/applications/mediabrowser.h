#ifndef MEDIABROWSER_H
#define MEDIABROWSER_H

#include "base/basewidget.h"

#include <QtGui>
#include <QtCore>

class SAGENextLauncher;


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
class MediaBrowser : public BaseWidget
{
    Q_OBJECT
public:
    explicit MediaBrowser(SAGENextLauncher *launcher, quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = Qt::Window);
    ~MediaBrowser();

    /**
      This slot can be access by SAGENextLauncher when a user uploaded a new media file
      There can be multiple mediaBrowsers so use rwlock to access hash.
      */
    static int insertNewMediaToHash(const QString &key, QPixmap &pixmap);

    inline SAGENextLauncher * launcher() {return _launcher;}

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    static QReadWriteLock mediaHashRWLock;
    static QHash<QString, QPixmap> mediaHash;

    QScrollBar *_HScrollBar;

    QScrollBar *_VScrollBar;

    SAGENextLauncher *_launcher;

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


};

#endif // MEDIABROWSER_H
