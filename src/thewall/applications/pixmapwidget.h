#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

#include "base/basewidget.h"

#include <QTcpServer>
//#include <QRunnable>

#include <QFutureWatcher>

#include <QtOpenGL>

class SN_PixmapWidget : public SN_BaseWidget
{
	Q_OBJECT
public:
	/*!
	  * read the image file from local disk
	  */
	SN_PixmapWidget(QString filename, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	/*!
	  * image file is sent through network.
	  * It will fire a receiving thread which is stateless (thread finishes and socket closed once file is delivered)
	  * Receiving should be a separate thread to prevent MainWindow from unresponsive
	  */
	SN_PixmapWidget(qint64 filesize, const QString &senderIP, const QString &recvIP, quint16 recvPort, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	~SN_PixmapWidget();


	/*!
	  single-shot pixel receiving thread. (Either from local file or socket)
	  */
	bool readImage();

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
//	void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
//	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	enum Data_Source {FROM_DISK_FILE, FROM_DISK_DIRECTORY, FROM_SOCKET};
	Data_Source dataSrc;

	/*!
	  Image is stored here from recv thread
	  */
	QImage *_imageTemp;

//	QImage _myImage;

	/*!
	  image is converted to pixmap and drawn from pixmap to make paint() faster
	  */
//	QPixmap *_pixmap;

	QPixmap _drawingPixmap;

	GLuint _gltexture;

	int _imgWidth;
	int _imgHeight;

	/*!
	  socket to accept connection from sender
	  */
	int serverSock;

	QString filename;
	qint64 filesize; // long long int

	/*!
	  image file will be received through below channel
	  */
	QHostAddress recvAddr;
	quint16 recvPort;

	/*!
	  A flag to synchronize receiving thread and drawing thread
	  */
	bool isImageReady;

	/*!
	  read thread monitor
	  */
	QFutureWatcher<bool> *futureWatcher;

	/*!
	  Starts readImage function as a thread.
	  This is called in the constructor
	  */
	void start();

public slots:
	/**
	  After an image has loaded (after the thread finished), this slot is called to do resize() and update()
	  */
	void callUpdate();
};

#endif // PIXMAPWIDGET_H
