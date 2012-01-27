#ifndef SN_CHECKER_H
#define SN_CHECKER_H

#include "base/railawarewidget.h"
#include <QTimer>
#include <QFuture>
#include <QFutureWatcher>

#define GL_GLEXT_PROTOTYPES
#include <GL/glu.h>

#include <sys/resource.h>

class SN_Checker : public SN_RailawareWidget
{
	Q_OBJECT
public:
    explicit SN_Checker(bool usepbo, const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
	~SN_Checker();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:

private:
	bool _end;

	/*!
	  if !_useOpenGL
	  */
	QImage *_image;

	GLuint _textureid;

	/*!
	  PBO buffer id
	  */
	GLuint _pboIds[2];

	/*!
	  The mapped double buffer for mapped pbo
	  */
	void * _pbobufarray[2];


	bool _init;

	/*!
	  frame rate
	  */
	qreal _frate;

	bool _usePbo;

	GLenum _pixelFormat;

	QFuture<void> _future;
	QFutureWatcher<void> _fwatcher;

	struct timeval lats;
	struct timeval late;

	struct rusage ru_start;
	struct rusage ru_end;


	bool __firstFrame;

	/*!
	  which buffer index is writable ?
	  1 - _pboBufIdx is used to display on the screen.
	  */
	int _pboBufIdx;

	/*!
	  Is one of the buffer idx is ready to write?
	  */
	bool __bufferMapped;

	bool _initPboMutex();
	pthread_mutex_t *_pbomutex;
	pthread_cond_t *_pbobufferready;


	/*!
	  Writes pixel data to the mapped buffer
	  */
	void _doRecvPixel();

signals:
	void frameReady();

public slots:
	/*!
	  init pbo double buffer
	  */
	void _doInit();

	/*!
	  Worker thread calling _doRecvPixel()
	  */
	void recvPixel();

	/*!
	  swap double buffer.
	  schedule repaint by calling update().
	  map new buffer.
	  signal thread
	  */
	void schedulePboUpdate();

	void scheduleUpdate();

	void endThread();
};



//class RecvPixel : public QThread
//{
//	Q_OBJECT
//public:
//	RecvPixel(QObject *parent=0);
//};

#endif // SN_CHECKER_H
