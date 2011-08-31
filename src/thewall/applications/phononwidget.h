#ifndef PHONONWIDGET_H
#define PHONONWIDGET_H

/**
  animation can be
  1. from local file on a disk                          [Phonon]
  2. file streaming from remote (ext-UI, other wall)    [Phonon]
  3. raw pixel streaming (sage app, ext-UI, other wall) [QImage/QPixmap, maybe Mplayer as backend]
  **/

/**
  BaseGraphicsWidget() // provides basic widget interaction / management
  <-
  BaseVideoWidget() // video specific capability/requiremnts:
					 1. can have data receiving thread
					 2. needs ability to pause/resume, seek if seekable
					 3. maybe require to inherit OGLWidget ?
	  <- SageStreamWidget() : has to know SAGE protocol, receive raw pixels
		 PhononWidget() : supports file loading, streaming modes
  **/


#include "common/basegraphicswidget.h"
//#include <QtGui>
#include <Phonon>

class PhononWidget : public BaseGraphicsWidget
{
	Q_OBJECT

public:
	/**
	  * local video file on disk
	  */
	PhononWidget(QString filename, const quint64 appid, const QSettings *s, BaseGraphicsWidget *parent = 0, Qt::WindowFlags wFlags = 0);
//	PhononWidget(QAbstractSocket *socket, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
//	PhononWidget(int sockfd, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
//	PhononWidget(QHostAddress *senderAddr, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
	/**
	  * objects are created only in this constructor
	  */
	PhononWidget(const Phonon::MediaSource &mediaSource, const quint64 appid, const QSettings *s, BaseGraphicsWidget *parent = 0, Qt::WindowFlags wFlags = 0);
	~PhononWidget();

//	quint16 getRatioToTheWall() const;

//	void run();

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
	QGraphicsProxyWidget *proxyWidget;
	Phonon::VideoPlayer *_vplayer;
	Phonon::VideoWidget *_vwidget;
	Phonon::MediaObject *_mobject;

	QGraphicsSimpleTextItem *bufferingText;

	void init();

protected slots:
	/**
	  * reimplement because we need to do pause the stream
	  */
	void minimize();
	void restore();

	/**
	  * will respond to MediaObject::bufferStatus(int) signal
	  */
	void bufferingInfo(int);

	/**
	  * will respond to MediaObject::stateChanged() signal
	  */
	void stateChanged(Phonon::State newstate, Phonon::State oldstate);
};

#endif // PHONONWIDGET_H
