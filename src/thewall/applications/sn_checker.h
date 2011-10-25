#ifndef SN_CHECKER_H
#define SN_CHECKER_H

#include "base/basewidget.h"
#include <QTimer>
#include <QtOpenGL>

#include <sys/resource.h>

class SN_Checker : public SN_BaseWidget
{
	Q_OBJECT
public:
    explicit SN_Checker(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
	~SN_Checker();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	void timerEvent(QTimerEvent *);

private:
	QImage *_image;
	
	GLuint _gltexture;

	GLuint pboIds[2];

	bool _init;

//	GLuint _dynamic;

//	QGLBuffer *_glbuffer;

//	QGLPixelBuffer *_pbuffer;

	int _timerid;

	qreal _frate;

//	QTimer _timer;



	QFuture<void> _future;
	QFutureWatcher<void> _fwatcher;

	struct timeval lats;
	struct timeval late;

	struct rusage ru_start;
	struct rusage ru_end;

	void _doInit();
	void _doRecvPixel();

public slots:
//	void doUpdate();

	void recvPixel();
};



//class RecvPixel : public QThread
//{
//	Q_OBJECT
//public:
//	RecvPixel(QObject *parent=0);
//};

#endif // SN_CHECKER_H
