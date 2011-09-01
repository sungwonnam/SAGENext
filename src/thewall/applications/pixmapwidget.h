#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

#include "base/basewidget.h"

#include <QTcpServer>
//#include <QRunnable>

#include <QFutureWatcher>

class PixmapWidget : public BaseWidget
{
	Q_OBJECT
public:
	/*!
	  * read the image file from local disk
	  */
	PixmapWidget(QString filename, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	/*!
	  * image file is sent through network.
	  * It will fire a receiving thread which is stateless (thread finishes and socket closed once file is delivered)
	  * Receiving should be a separate thread to prevent MainWindow from unresponsive
	  */
	PixmapWidget(qint64 filesize, const QString &senderIP, const QString &recvIP, quint16 recvPort, const quint64 appid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = 0);

	~PixmapWidget();


	/*!
	  single-shot pixel receiving thread.
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
	QImage *image;

	/*!
	  image is converted to pixmap and drawn from pixmap to make paint() faster
	  */
	QPixmap *pixmap;

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
	  Starts readImage function as a thread
	  */
	void start();

public slots:
	void callUpdate();
};

#endif // PIXMAPWIDGET_H
