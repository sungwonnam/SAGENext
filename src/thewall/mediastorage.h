#ifndef MEDIASTORAGE_H
#define MEDIASTORAGE_H

#include <QtGui>
#include <QtCore>
#include <QObject>

#include <poppler-qt4.h>

class QSettings;

class SN_MediaStorage : public QObject
{
    Q_OBJECT
public:
    explicit SN_MediaStorage(const QSettings *s, QObject *parent = 0);

    bool insertNewMediaToHash(const QString &key);
    bool insertNewFolderToHash(const QString &key);
    bool checkForMediaInHash(const QString &key);

	inline QHash<QString, QPixmap> & getMediaHashForRead() const {return mediaHash;}

private:
	const QSettings *_settings;

    static QHash<QString, QPixmap> mediaHash;

    static QReadWriteLock mediaHashRWLock;
    QPixmap readImage(const QString &filename);
    QPixmap readPDF(const QString &filename);
    QPixmap readVideo(const QString &filename);

	/**
	  Read media directory recursively and build initial media hash
	  */
	int initMediaHash();

signals:
    void newMediaAdded();
public slots:

};

#endif // MEDIASTORAGE_H
