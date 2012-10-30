#ifndef MEDIASTORAGE_H
#define MEDIASTORAGE_H

#include <QtGui>
#include <QtCore>

#include <poppler-qt4.h>

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
    
    static QMap<const QString, MediaMetaData *> GlobalMediaList;

    static QReadWriteLock MediaListRWLock;

    /*!
      \brief This function add a media to the GlobalMediaList
      */
    void addNewMedia(SAGENext::MEDIA_TYPE mtype, const QString &filepath);

    /*!
     * \brief getMediaListInDir
     * \param dir
     * \return the list of media items in a directory dir
     *
     * The list contains a COPY of SN_MediaItem (not the pointer to the object)
     * Because there can be multiple media browsers
     */
    QMap<const QString, MediaMetaData *> getMediaListInDir(const QDir &dir);

private:
	const QSettings *_settings;

    QPixmap _createThumbnail(SAGENext::MEDIA_TYPE mtype, const QString &filename);

    /*!
      creates thumbnail of the media file
      */
    QPixmap _readImage(const QString &filename);
    QPixmap _readPDF(const QString &filename);
    QPixmap _readVideo(const QString &filename);


	/**
	  Read media directory recursively and build initial media hash
	  */
	int _initMediaList();

signals:
    void newMediaAdded();

    void mediaItemClicked(SAGENext::MEDIA_TYPE, const QString filepath);

};

#endif // MEDIASTORAGE_H
