#ifndef SN_PBOEXAMPLE_H
#define SN_PBOEXAMPLE_H

#include "base/basewidget.h"
#include <QtOpenGL>
#include <sys/resource.h>
#include <QThread>

#include "../common/imagedoublebuffer.h"



class SN_PBOtexture : public SN_BaseWidget
{
	Q_OBJECT
public:
	explicit SN_PBOtexture(const QSize &imagesize, qreal framerate, const QGLFormat &format, const quint64 appid,  const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
	~SN_PBOtexture();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	void timerEvent(QTimerEvent *);

private:
	QGLPixelBuffer *_pbo;

	QImage *_image;

	QGLFormat _glformat;

	GLuint _textureid;
	GLuint _dynamictextureid;

	int _updateTimer;

	bool _init;

	qreal _rotTri;

public slots:
	void init();
	void updateTexture();
};





class SN_FBOtexture : public SN_BaseWidget
{
	Q_OBJECT
public:
	SN_FBOtexture(const QSize &imagesize, qreal framerate,  const quint64 appid,  const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
	~SN_FBOtexture();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	void timerEvent(QTimerEvent *e);

private:
	int _timerid;

	bool _init;

	QSize _imgsize;

	QImage *_image;

	QGLFramebufferObject *_fbo;

public slots:
	void init();
	void updateTexture();
};





class GLPainter;

class SN_GLBufferExample : public SN_BaseWidget
{
	Q_OBJECT
public:
    explicit SN_GLBufferExample(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);
	~SN_GLBufferExample();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);



protected:
	void timerEvent(QTimerEvent *);

private:
	int _timerid;

	QGLContext *_context;

	QImage *_image;

	QGLBuffer **_glbuffer;

	GLDoubleBuffer *_doubleBuffer;

	GLuint _textureid;

	bool _init;

	int _bufferIndex;

	QSize _imgsize;
	int _byteperpixel;



	GLPainter *_glPainter;
	QThread _glThread;

	struct timeval lats;
	struct timeval late;

	struct rusage ru_start;
	struct rusage ru_end;

public slots:
	void init();
//	void swapAppBuffer();
	void updateTextureBind(GLuint tid);
};





class GLPainter : public QThread
{
	Q_OBJECT
public:
	explicit GLPainter(const QSize &imagesize, QImage::Format imageformat, GLuint textureid, QGLWidget *glwidget, QObject *parent = 0);

	~GLPainter();

protected:
	void run();

private:
	QGLWidget *_glwidget;
	QGLWidget *_myGlWidget;
	bool _end;

//	GLDoubleBuffer *_doublebuffer;

	GLuint _textureid;

	QSize _imgsize;
	QImage::Format _imgformat;

signals:
	void updated(GLuint textureid);

public slots:
//	void start();
	void stopThread();
};

#endif // SN_PBOEXAMPLE_H
