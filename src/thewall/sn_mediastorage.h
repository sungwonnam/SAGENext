#ifndef MEDIASTORAGE_H
#define MEDIASTORAGE_H

#include <QtGui>
#include <QtCore>

#include "common/commondefinitions.h"

typedef struct MediaMetaT {
    SAGENext::MEDIA_TYPE type;
    QPixmap pixmap;
} MediaMetaData;



class SN_MediaItem;



class SN_MediaStorage : public QObject
{
    Q_OBJECT
public:
    explicit SN_MediaStorage(const QSettings *s, QObject *parent = 0);

    ~SN_MediaStorage();
    
    /*!
     * \brief GlobalMediaList contains all the media item
     * Key is absolute file path to the media item (or folder)
     */
    static QMap<QString, MediaMetaData *> GlobalMediaList;

    static QReadWriteLock MediaListRWLock;


    /*!
     * \brief getMediaListInDir
     * \param dir
     * \return the list of media items in a directory dir
     *
     * The list contains a COPY of SN_MediaItem (not the pointer to the object)
     * Because there can be multiple media browsers
     */
    QMap<QString, MediaMetaData *> getMediaListInDir(const QDir &dir);

private:
	const QSettings *_settings;

    QSize _thumbSize;

//    QPixmap _createThumbnail(SAGENext::MEDIA_TYPE mtype, const QString &filename);

    /*!
      creates thumbnail of the media file
      */
    bool _readImage(const QString &filename, QPixmap &pixmap);
    bool _readPDF(const QString &fpath, QPixmap &pixmap);
    bool _readVideo(const QString &filename, QPixmap &pixmap);
    bool _readPlugin(const QString &filename, QPixmap &pixmap);

    QPixmap _createThumbnail(const QString & fpath, SAGENext::MEDIA_TYPE mtype);

	/**
	  Read media directory recursively and build initial media hash
	  */
	int _initMediaList();

signals:
    void newMediaAdded();

//    void mediaItemClicked(SAGENext::MEDIA_TYPE, const QString filepath);

public slots:
    /*!
      \brief This function add a media to the GlobalMediaList
      */
    void addNewMedia(int mtype, const QString &filepath);
};

#endif // MEDIASTORAGE_H
