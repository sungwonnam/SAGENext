#ifndef MEDIASTORAGE_H
#define MEDIASTORAGE_H

#include <QtGui>
#include <QtCore>

#include <poppler-qt4.h>

#include "common/commondefinitions.h"

class SN_MediaItem;

class SN_MediaStorage : public QObject
{
    Q_OBJECT
public:
    explicit SN_MediaStorage(const QSettings *s, QObject *parent = 0);

    static QList<SN_MediaItem *> MediaList;

    static QReadWriteLock MediaListRWLock;

    /*!
     * \brief insertNewMediaToHash
     * \param filepath is an absolute path to the media
     * \return
     * This function will emit newMediaAdded()
     */
    bool addNewMedia(SN_MediaItem *mediaitem);

    bool addNewMedia(SAGENext::MEDIA_TYPE mtype, const QString &filepath);

    bool checkForMediaInList(const QString &path);

	inline QList<SN_MediaItem *> getMediaListForRead() const {return MediaList;}

    /*!
     * \brief getMediaListInDir
     * \param dir
     * \return the list of media items in a directory dir
     */
    QList<SN_MediaItem *> * getMediaListInDir(const QDir &dir);

private:
	const QSettings *_settings;

    QPixmap _createThumbnail(SAGENext::MEDIA_TYPE mtype, const QString &filename);


    SAGENext::MEDIA_TYPE _findMediaType(const QString &filepath);


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
