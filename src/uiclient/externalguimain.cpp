#include "externalguimain.h"
#include "ui_externalguimain.h"
#include "ui_connectiondialog.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>

#include <QNetworkInterface>


ExternalGUIMain::ExternalGUIMain(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ExternalGUIMain)
    , _settings(0)
    , uiclientid(-1)
    , fileTransferPort(0)
    , ungrabMouseAction(0)
    , msgsock(0)
    , scaleToWallX(0.0)
    , scaleToWallY(0.0)
    , msgThread(0)
    , sendThread(0)
    , mediaDropFrame(0)
{
	ui->setupUi(this);

	_settings = new QSettings("sagenextpointer.ini", QSettings::IniFormat, this);

	//	wallAddr.clear();

	/* file dialog */
	//        fdialog = new QFileDialog(this, "Open Media Files", QDir::homePath(), "Images (*.tif *.tiff *.svg *.bmp *.png *.jpg *.jpeg *.gif *.xpm);;Videos (*.mov *.avi *.mpg *.mp4);;Any (*)");
	fdialog = new QFileDialog(this, "Open Media Files", QDir::homePath(), "*");
	fdialog->setModal(false);
	fdialog->setVisible(false);
	connect(fdialog, SIGNAL(filesSelected(QStringList)), this, SLOT(readFiles(QStringList)));


	/* mouse ungrab action */
	ungrabMouseAction = new QAction(this);
	//	ungrabMouseAction->setShortcut( QKeySequence(Qt::Key_CapsLock + Qt::Key_Tab));
	ungrabMouseAction->setShortcut( QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::ALT + Qt::Key_M));
	connect(ungrabMouseAction, SIGNAL(triggered()), this, SLOT(ungrabMouse()));
	addAction(ungrabMouseAction);

	//	timer = new QTimer(this);
	//	timer->setInterval(500); // msec
	//	connect(timer, SIGNAL(timeout()), this, SLOT(updateScene()));
	//	timer->start();

	mediaDropFrame = new DropFrame(this);
	ui->verticalLayout->addWidget(mediaDropFrame);
}

ExternalGUIMain::~ExternalGUIMain()
{
        delete ui;
        delete fdialog;
//	delete gview;

        if (hasMouseTracking()) releaseMouse();

//	if ( msgThread && msgThread->isRunning())
//		msgThread->terminate();
}

// triggered by CMD (CTRL) + n
void ExternalGUIMain::on_actionNew_Connection_triggered()
{
	// open modal dialog to enter IP address and port
	ConnectionDialog cd(_settings);
	cd.exec();
	if ( cd.result() == QDialog::Rejected) return;

	_pointerName = cd.pointerName();
	_myIpAddress = cd.myAddress();
	_vncUsername = cd.vncUsername();
	_vncPasswd = cd.vncPasswd();

	// if there is existing connection, close it first
	if ( msgsock != 0 ) {
		::shutdown(msgsock, SHUT_RDWR);
		::close(msgsock);
//		msgThread->endThread();
		msgThread->wait();
		msgsock = 0;
	}


	msgsock = ::socket(AF_INET, SOCK_STREAM, 0);
	if ( msgsock == -1 ) {
		perror("socket");
		return;
	}

	int optval = 1;
	if ( setsockopt(msgsock, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_REUSEADDR failed", __FUNCTION__);
	}
	if ( setsockopt(msgsock, SOL_SOCKET, SO_KEEPALIVE, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt SO_KEEPALIVE failed", __FUNCTION__);
	}
	if ( setsockopt(msgsock, IPPROTO_TCP, TCP_NODELAY, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		qWarning("%s() : setsockopt TCP_NODELAY failed", __FUNCTION__);
	}

	sockaddr_in walladdr;
	memset(&walladdr, 0, sizeof(walladdr));
	walladdr.sin_family = AF_INET;
	walladdr.sin_port = htons( cd.port() );
	inet_pton(AF_INET,  qPrintable(cd.address()),  &walladdr.sin_addr.s_addr);
//	walladdr.sin_addr.s_addr = htonl(wallAddr.toIPv4Address());
//	walladdr.sin_addr.s_addr = inet_addr(ipaddr);

	if ( ::connect(msgsock, (const struct sockaddr *)&walladdr, sizeof(walladdr)) != 0 ) {
		qCritical("ExternalGUIMain::%s() : connect error", __FUNCTION__);
		return;
	}
	qDebug("ExternalGUIMain::%s() : connected to %s:%d", __FUNCTION__, qPrintable(cd.address()), cd.port());


        /*!
          receive scene size and my uiclientid
          */
	QByteArray buf(EXTUI_MSG_SIZE, '\0');
	if ( ::recv(msgsock, buf.data(), buf.size(), MSG_WAITALL) <= 0 ) {
		qDebug() << "recv error while receiving scene size";
		perror("recv");
		::shutdown(msgsock, SHUT_RDWR);
		::close(msgsock);
		return;
	}

	int x = 0;
	int y = 0;
	sscanf(buf.constData(), "%llu %d %d %d", &uiclientid, &x, &y, &fileTransferPort);
	wallSize.rwidth() = x;
	wallSize.rheight() = y;
        //qDebug("ExternalGUIMain::%s() : my uiClientId is %llu, wall resolution is %d x %d", __FUNCTION__, uiclientid, x,y);

	QDesktopWidget *d = QApplication::desktop();
	qDebug("My uiclientid is %llu, The wall size is %d x %d, my primary screen size %d x %d", uiclientid, x, y, d->screenGeometry().width(), d->screenGeometry().height());

        /*!
          When I send my mouse movement to the wall, it's pos should be scaled with these values
          */
	scaleToWallX = wallSize.width() / (qreal)(d->screenGeometry().width());
	scaleToWallY = wallSize.height() / (qreal)(d->screenGeometry().height());
	qDebug("The scaleToWall %.2f x %.2f", scaleToWallX, scaleToWallY);

        /*
          Below isn't needed. I will not receive app layout
          */
//	QGraphicsScene *scene = new QGraphicsScene(this);
//	scene->setBackgroundBrush(Qt::darkGray);
//	ui->graphicsView->setScene(scene);
//	ui->graphicsView->setSceneRect(0, 0, ui->graphicsView->size().width(), ui->graphicsView->size().height());

//	/*!
//	  When I received scene's item info from the wall, I should apply this scale to display items on my scene
//	  */
//	scaleFromWallX = ui->graphicsView->width() / wallSize.width();
//	scaleFromWallY = ui->graphicsView->height() / wallSize.height();
//	qDebug("The scaleFromWall %.6f x %.6f", scaleFromWallX, scaleFromWallY);


	/*!
	  One msg thread per wall connection
	*/
	msgThread = new MessageThread(msgsock, uiclientid, _myIpAddress);
	connect(msgThread, SIGNAL(finished()), msgThread, SLOT(deleteLater()));
	//	connect(msgThread, SIGNAL(finished()), scene, SLOT(deleteLater()));
	connect(msgThread, SIGNAL(finished()), this, SLOT(ungrabMouse()));
	//        connect(msgThread, SIGNAL(newAppLayout(QByteArray)), this, SLOT(updateScene(QByteArray)));
	msgThread->start();



        /**
          file transfer thread
          */
//        sendThread = new SendThread(cd.address(), fileTransferPort);
//        sendThread->start();
}


//void ExternalGUIMain::updateScene(const QByteArray layout) {
//	// for each message thread
////	QMetaObject::invokeMethod(msgThread, "requestAppLayout", Qt::QueuedConnection);

//	if ( QString::compare(ui->showLayoutButton->text(), "Show Layout", Qt::CaseInsensitive) == 0 ) {
//		// User didn't trigger the button. I don't want to receive app layout info
//		return;
//	}

//	int msgtype;
//	unsigned int numapps = 0;
//	sscanf(layout.constData(), "%d %u", &msgtype, &numapps);

//	QGraphicsScene *scene = ui->graphicsView->scene();
//	if (!scene) return;

//	QList<QByteArray> data = layout.split(' ');

//	unsigned int count = 0; int i=2;
//	while ( count < numapps) {
//		if ( i+5 >= data.size() ) {
//			qDebug() << "updateScene : data received isn't large enough to hold " << numapps << " apps";
//			break;
//		}
//		quint64 gaid = data[i].toULongLong();
//		int x = data[i+1].toInt();
//		int y = data[i+2].toInt();
//		qreal z = data[i+3].toDouble();
//		int width = data[i+4].toInt();
//		int height = data[i+5].toInt();

//		// add/modify rect item to/in the scene
//		QGraphicsRectItem *item = 0;
//		if ( (item = itemWithGlobalAppId(scene, gaid)) ) {
//			// modify
//			item->setRect(x*scaleFromWallX, y*scaleFromWallY, width*scaleFromWallX, height*scaleFromWallY);
//			item->setZValue(z);
//		}
//		else {
//			// add
//			QGraphicsRectItem *addeditem = scene->addRect(x*scaleFromWallX, y*scaleFromWallY, width*scaleFromWallX, height*scaleFromWallY, QPen(), QBrush(Qt::lightGray));
//			addeditem->setZValue(z);
//			addeditem->setData(0, gaid);
//			addeditem->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
//		}

//		i += 6;
//		++count;
//	}

//	/** how to remove closed app  ??? */
//}

QGraphicsRectItem * ExternalGUIMain::itemWithGlobalAppId(QGraphicsScene *scene, quint64 gaid) {
        if (!scene) return 0;

        bool ok = false;
        quint64 storedId = 0;

        QList<QGraphicsItem *> i = scene->items();
        foreach( QGraphicsItem *item, i ) {
                storedId = item->data(0).toULongLong(&ok);
                if(ok && storedId == gaid) {
                        return (static_cast<QGraphicsRectItem *>(item));
                }
        }
        return 0;
}

//void ExternalGUIMain::resizeEvent(QResizeEvent *e) {
//        Q_UNUSED(e);
//	ui->graphicsView->setSceneRect( QRectF(QPointF(0,0), ui->graphicsView->size()) );
//	qDebug() << "resizeEvent : scene rect is now " << ui->graphicsView->sceneRect();

//	if (ui->graphicsView->scene() && !wallSize.isNull() && !wallSize.isEmpty() && wallSize.isValid()) {

//		scaleFromWallX = ui->graphicsView->width() / wallSize.width();
//		scaleFromWallY = ui->graphicsView->height() / wallSize.height();
//		qDebug() << "scaleFromWall " << scaleFromWallX << "x" << scaleFromWallY;
//	}

//}



//void ExternalGUIMain::on_showLayoutButton_clicked()
//{
//	// send message to the wall TOGGLE_APP_LAYOUT
//	/* if I'm receiving layout info this will make it stop
//	   */
//	if (msgThread && msgThread->isRunning()) {
//		QByteArray msg(EXTUI_MSG_SIZE, 0);
//		sprintf(msg.data(), "%d %llu", TOGGLE_APP_LAYOUT, uiclientid);
//		QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));

//		// change button label
//		if ( QString::compare(ui->showLayoutButton->text(), "Show Layout", Qt::CaseInsensitive) == 0 ) {
//			ui->showLayoutButton->setText("Hide Layout");
//			/* now scene shows app layout */
//		}
//		else {
//			ui->showLayoutButton->setText("Show Layout");
//			/* scene no longer shows app layout. clear the current scene */
//			if (ui->graphicsView->scene())
//				ui->graphicsView->scene()->clear();
//		}
//	}
//	else {
//		qDebug("%s::%s() : no msgThread", metaObject()->className(), __FUNCTION__);
//	}

//}

void ExternalGUIMain::on_vncButton_clicked()
{
	// send msg to UiServer so that local sageapp (vncviewer) can be started

	QByteArray msg(EXTUI_MSG_SIZE, 0);

	/*
	QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
	foreach (QNetworkInterface net, netInterfaces) {
		if ( net.isValid() &&  net.flags() & QNetworkInterface::IsRunning  &&  net.flags() & !QNetworkInterface::IsLoopBack) {
			// fins ip address
			break;
		}
	}
	*/

	// msgtype, uiclientid, senderIP, display #, vnc passwd, framerate
	sprintf(msg.data(), "%d %llu %s %d %s %s %d", VNC_SHARING, uiclientid, qPrintable(_myIpAddress), 0, qPrintable(_vncUsername), qPrintable(_vncPasswd), 24);

	if (msgThread && msgThread->isRunning()) {
		QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
	}
	else {
		qWarning() << "Sharing screen: please connect to a SAGENext first";
	}
}


void ExternalGUIMain::on_pointerButton_clicked()
{
	if ( hasMouseTracking() ) return;

	// draw cursor on the wall
	QByteArray msg(EXTUI_MSG_SIZE, 0);

	// msgtype, uiclientid, pointer name, Red, Green, Blue
	sprintf(msg.data(), "%d %llu %s %d %d %d", POINTER_SHARE, uiclientid, qPrintable(_pointerName), 255, 128, 0);

	if (msgThread && msgThread->isRunning()) {
		grabMouse();
		setMouseTracking(true); // the widget receives mouse move events even if no buttons are pressed
		qDebug() << "grabMouse";
		QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
	}
	else {
		qWarning() << "Sharing pointer: please connect to a SAGENext first";
	}
}

void ExternalGUIMain::ungrabMouse() {
	if ( ! hasMouseTracking() ) return;

	setMouseTracking(false);
	releaseMouse();
	qDebug() << "mouse released";

	// remove cursor on the wall
	// draw cursor on the wall
	QByteArray msg(EXTUI_MSG_SIZE, 0);

	// msgtype, uiclientid
	sprintf(msg.data(), "%d %llu", POINTER_UNSHARE, uiclientid);

	if (msgThread && msgThread->isRunning())
		QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
}

void ExternalGUIMain::mouseMoveEvent(QMouseEvent *e) {
        // setMouseTracking(true) to generate this event even when button isn't pressed
        if ( hasMouseTracking() ) {

                int x,y;
                x = scaleToWallX * e->globalX();
                y = scaleToWallY * e->globalY();
                // send mmouse position to the wall's UiServer
                QByteArray msg(EXTUI_MSG_SIZE, 0);

                if ( e->buttons() & Qt::LeftButton) {
                        // dragging.
                        // msgtype, uiclientid, x, y
                        sprintf(msg.data(), "%d %llu %d %d", POINTER_DRAGGING, uiclientid, x, y);
                }
                else if (e->buttons() & Qt::NoButton) {
                        // just move pointer
//			qDebug() << "move " << e->globalPos();
                }
                else {
                        // msgtype, uiclientid, x, y
                        sprintf(msg.data(), "%d %llu %d %d", POINTER_MOVING, uiclientid, x, y);
                }

                if (msgThread && msgThread->isRunning())
                        QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
        }
        else {
                QMainWindow::mouseMoveEvent(e);
        }
}

void ExternalGUIMain::mousePressEvent(QMouseEvent *e) {

        mousePressedPos = e->globalPos();

        if ( hasMouseTracking() ) {
//		qDebug() << "mouse press" << e->button() << e->globalPos();

                int x,y;
                x = scaleToWallX * e->globalX();
                y = scaleToWallY * e->globalY();
                // send mmouse position to the wall's UiServer
                QByteArray msg(EXTUI_MSG_SIZE, 0);

                if (e->buttons() & Qt::LeftButton) {
                        qDebug() << "left press" << e->globalPos();
                        // msgtype, uiclientid, x, y
                        sprintf(msg.data(), "%d %llu %d %d", POINTER_PRESS, uiclientid, x, y);
                }
                else if (e->buttons() & Qt::RightButton) {
                        qDebug() << "right press";
                        sprintf(msg.data(), "%d %llu %d %d", POINTER_RIGHTPRESS, uiclientid, x, y);
                }

                if (msgThread && msgThread->isRunning())
                        QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
        }
        else {
                QMainWindow::mousePressEvent(e);
        }
}

void ExternalGUIMain::mouseReleaseEvent(QMouseEvent *e) {
        if ( hasMouseTracking() ) {
                int ml = (e->globalPos() - mousePressedPos).manhattanLength();
//		qDebug() << "release" << e->button() << e->globalPos() << ml;

                int x,y;
                x = scaleToWallX * e->globalX();
                y = scaleToWallY * e->globalY();
                // send mmouse position to the wall's UiServer
                QByteArray msg(EXTUI_MSG_SIZE, 0);

                if ( ml > 3 ) {
                        // NO CLICK
                        //sprintf(msg.data(), "%d %llu %d %d", POINTER_RELEASE, uiclientid, x, y);
                }
                else {
                        // effective CLICK
                        if (e->button() == Qt::LeftButton) {
                                qDebug() << "left release " << e->globalPos() << ml;
                                // msgtype, uiclientid, x, y
                                sprintf(msg.data(), "%d %llu %d %d", POINTER_CLICK, uiclientid, x, y);
                        }
                        else if (e->button() == Qt::RightButton) {
                                qDebug() << "right release" << e->globalPos() << ml;
                                sprintf(msg.data(), "%d %llu %d %d", POINTER_RIGHTCLICK, uiclientid, x, y);
                        }
                        if (msgThread && msgThread->isRunning())
                                QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
                }
        }
        else {
                QMainWindow::mouseReleaseEvent(e);
        }
}

void ExternalGUIMain::mouseDoubleClickEvent(QMouseEvent *e) {
        if ( hasMouseTracking() ) {
                // maximize/restore the app
//		qDebug() << "double click";
                int x,y;
                x = scaleToWallX * e->globalX();
                y = scaleToWallY * e->globalY();
                QByteArray msg(EXTUI_MSG_SIZE, 0);
                sprintf(msg.data(), "%d %llu %d %d", POINTER_DOUBLECLICK, uiclientid, x, y);

                if (msgThread && msgThread->isRunning())
                        QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
        }
        else {
                QMainWindow::mouseDoubleClickEvent(e);
        }
}

void ExternalGUIMain::wheelEvent(QWheelEvent *e) {
        if ( hasMouseTracking() ) {
                int numDegrees = e->delta() / 8;
                int numTicks = numDegrees / 15;

                int x = scaleToWallX * e->globalX();
                int y = scaleToWallY * e->globalY();


                if ( e->buttons() & Qt::MidButton ) {
                        qDebug() << "mid button";
                }
                if ( e->buttons() & Qt::NoButton ) {
                        qDebug() << "no button";
                }
                if (e->buttons() & Qt::LeftButton) {
                        qDebug() << "left";
                }
                if (e->buttons() & Qt::RightButton) {
                        qDebug() << "right";
                }
                if (e->buttons() & Qt::MiddleButton) {
                        qDebug() << "middle";
                }

                // resize the app
                QByteArray msg(EXTUI_MSG_SIZE, 0);
                sprintf(msg.data(), "%d %llu %d %d %d", POINTER_WHEEL, uiclientid, x, y, numTicks);
                qDebug() << "WHEEL" << e->buttons() << e->globalPos() << numTicks;

                if (msgThread && msgThread->isRunning())
                        QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg));
        }
        else {
                QMainWindow::wheelEvent(e);
        }
}




// CTRL - O to open file dialog
void ExternalGUIMain::on_actionOpen_Media_triggered()
{
	if (fdialog) fdialog->show();
	else {
		qDebug() << "fdialog is null";
	}
}

void ExternalGUIMain::readFiles(QStringList filenames) {
    if ( filenames.empty() ) {
        return;
    }
    else {
        //qDebug("MainWindow::%s() : %d", __FUNCTION__, filenames.size());
    }

	if (!msgThread || !msgThread->isRunning()) {
		qWarning() << "Please connect to a SAGENext first";
		return;
	}

    QRegExp rxVideo("\\.(avi|mov|mpg|mp4|mkv|mpg|flv|wmv|mpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
    QRegExp rxImage("\\.(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);


    for ( int i=0; i<filenames.size(); i++ ) {

        QString filename = filenames.at(i);
        //qDebug("%s::%s() : %d, %s", metaObject()->className(), __FUNCTION__, i, qPrintable(filename));

        //QFileInfo fi(filename);
        if ( filename.contains(rxVideo) ) {
            qDebug("ExternalGUIMain::%s() : Prepare sending the video file %s", __FUNCTION__, qPrintable(filename));
            QMetaObject::invokeMethod(msgThread, "registerApp", Qt::QueuedConnection, Q_ARG(int, MEDIA_TYPE_LOCAL_VIDEO), Q_ARG(QString, filename));
        }
        else if (filename.contains(rxImage)) {
            qDebug("ExternalGUIMain::%s() : Prepare sending the image file %s", __FUNCTION__, qPrintable(filename));
            QMetaObject::invokeMethod(msgThread, "registerApp", Qt::QueuedConnection, Q_ARG(int, MEDIA_TYPE_IMAGE), Q_ARG(QString, filename));
        }
        else {
            qCritical("ExternalGUIMain::%s() : Unrecognized file format", __FUNCTION__);
        }
    }
}




DropFrame::DropFrame(QWidget *parent) :
        QFrame(parent)
{
        setAcceptDrops(true);
}

void DropFrame::dragEnterEvent(QDragEnterEvent *e) {
        if (e->mimeData()->hasImage())
                e->acceptProposedAction();
}

void DropFrame::dropEvent(QDropEvent *e) {
// file transfer
}




ConnectionDialog::ConnectionDialog(QSettings *s, QWidget *parent)
        :QDialog(parent)
        ,ui(new Ui::connectionDialog)
        ,_settings(s)
        ,portnum(0)
{
        ui->setupUi(this);

        addr.clear();

        ui->ipaddr->setInputMask("000.000.000.000;_");
        ui->myaddrLineEdit->setInputMask("000.000.000.000;_");
        ui->port->setInputMask("00000;_");

        ui->ipaddr->setText( _settings->value("walladdr", "127.0.0.1").toString() );
        ui->myaddrLineEdit->setText( _settings->value("myaddr", "127.0.0.1").toString() );
        ui->port->setText( _settings->value("wallport", 30003).toString() );
		ui->vncUsername->setText(_settings->value("vncusername", "user").toString());
        ui->vncpasswd->setText(_settings->value("vncpasswd", "dummy").toString());
        ui->pointerNameLineEdit->setText( _settings->value("pointername", "pointer").toString());

//	ui->ipaddr->setText("127.0.0.1");
//	ui->port->setText("30003");
}

ConnectionDialog::~ConnectionDialog() {
        delete ui;
}

void ConnectionDialog::on_buttonBox_accepted()
{
        addr = ui->ipaddr->text();
        portnum = ui->port->text().toInt();
        myaddr = ui->myaddrLineEdit->text();
        pName = ui->pointerNameLineEdit->text();
		vncusername = ui->vncUsername->text();
        vncpass = ui->vncpasswd->text();


        _settings->setValue("walladdr", addr);
        _settings->setValue("wallport", portnum);
        _settings->setValue("myaddr", myaddr);
        _settings->setValue("pointername", pName);
		_settings->setValue("vncusername", vncusername);
        _settings->setValue("vncpasswd", vncpass);

		if (vncUsername().isEmpty()) {
			vncUsername() = "user";
		}

        accept();
//	done(0);
}

void ConnectionDialog::on_buttonBox_rejected()
{
        reject();
}



