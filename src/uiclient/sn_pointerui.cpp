#include "sn_pointerui.h"

#include "ui_sn_pointerui.h"
#include "ui_sn_pointerui_conndialog.h"

/*
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
*/

//#include <QNetworkInterface>
#include <QHostAddress>


SN_PointerUI::SN_PointerUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SN_PointerUI)
    , _settings(0)
    , _uiclientid(0)
    , fileTransferPort(0)
    , ungrabMouseAction(0)
    , scaleToWallX(0.0)
    , scaleToWallY(0.0)
    , sendThread(0)
	, isMouseCapturing(false)
    , mediaDropFrame(0)
    , _sharingEdge(QString("top"))
	, macCapture(0)
	, mouseBtnPressed(0)

{
	ui->setupUi(this);

	ui->isConnectedLabel->hide();
#ifdef Q_OS_MAC
	ui->hookMouseBtn->hide();
#endif

	ui->mainToolBar->addAction(ui->actionNew_Connection);
	ui->mainToolBar->addAction(ui->actionOpen_Media);
	ui->mainToolBar->addAction(ui->actionShare_desktop);
	ui->mainToolBar->addAction(ui->actionSend_text);


	_settings = new QSettings("sagenextpointer.ini", QSettings::IniFormat, this);


	//
	// to receive a message from SAGENext
	//
	QObject::connect(&_tcpMsgSock, SIGNAL(readyRead()), this, SLOT(readMessage()));
//	QObject::connect(&_tcpMsgSock, SIGNAL(disconnected()), &
	QObject::connect(&_tcpDataSock, SIGNAL(readyRead()), this, SLOT(receiveData()));

	//
	// mouse ungrab action
	//
	ungrabMouseAction = new QAction(this);
	ungrabMouseAction->setShortcut( QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::ALT + Qt::Key_M));
	QObject::connect(ungrabMouseAction, SIGNAL(triggered()), this, SLOT(unhookMouse()));
	addAction(ungrabMouseAction);


	//	timer = new QTimer(this);
	//	timer->setInterval(500); // msec
	//	connect(timer, SIGNAL(timeout()), this, SLOT(updateScene()));
	//	timer->start();


	//
	// Drag & Drop gui
	//
	mediaDropFrame = new SN_PointerUI_DropFrame(this);
	ui->verticalLayout->addWidget(mediaDropFrame);
	int lastwidgetidx = ui->verticalLayout->count() - 1; // find the index of the last (bottom) widget which is dropFrame
	ui->verticalLayout->setStretch(lastwidgetidx, 2);

	QObject::connect(mediaDropFrame, SIGNAL(mediaDropped(QList<QUrl>)), this, SLOT(runSendFileThread(QList<QUrl>)));




	//
	// file dialog (CMD + o)
	//
	//fdialog = new QFileDialog(this, "Open Media Files", QDir::homePath(), "Images (*.tif *.tiff *.svg *.bmp *.png *.jpg *.jpeg *.gif *.xpm);;Videos (*.mov *.avi *.mpg *.mp4);;Any (*)");
	fdialog = new QFileDialog(this, "Open Media Files", QDir::homePath(), "*");
	fdialog->setModal(false);
	fdialog->setVisible(false);
	QObject::connect(fdialog, SIGNAL(filesSelected(QStringList)), this, SLOT(readFiles(QStringList)));
}

SN_PointerUI::~SN_PointerUI()
{
	delete ui;
	if (fdialog) delete fdialog;

	if (hasMouseTracking()) releaseMouse();

	_tcpMsgSock.close();

	_tcpDataSock.close();
	
	if (macCapture) {
		macCapture->kill();
//		delete macCapture; // doesn't need because macCapture is a child Qt object
		macCapture->waitForFinished(-1);
	}
}



// triggered by CMD (CTRL) + n
void SN_PointerUI::on_actionNew_Connection_triggered()
{
	// open modal dialog to enter IP address and port
	SN_PointerUI_ConnDialog cd(_settings);
	cd.exec();
	if ( cd.result() == QDialog::Rejected) return;

	_wallAddress = cd.address();
	_pointerName = cd.pointerName();
	_pointerColor = cd.pointerColor();
//	_myIpAddress = cd.myAddress();
	_vncUsername = cd.vncUsername();
	_vncPasswd = cd.vncPasswd();
	_sharingEdge = cd.sharingEdge();

	if (_tcpMsgSock.state() == QAbstractSocket::ConnectedState) {
		_tcpMsgSock.disconnectFromHost();
	}

	_tcpMsgSock.connectToHost(QHostAddress(_wallAddress), cd.port());
}

void SN_PointerUI::on_actionOpen_Media_triggered()
{
	if (!_tcpMsgSock.isValid()) {
		QMessageBox::warning(this, "No Connection", "Connect to SAGENext first");
		return;
	}

	if (fdialog) fdialog->show();
	else {
		qDebug() << "fdialog is null";
	}
}

void SN_PointerUI::on_actionShare_desktop_triggered()
{
	if (!_tcpMsgSock.isValid()) {
		QMessageBox::warning(this, "No Connection", "Connect to SAGENext first");
		return;
	}

	// send msg to UiServer so that local sageapp (vncviewer) can be started
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);

	// msgtype, uiclientid, senderIP, display #, vnc passwd, framerate
	sprintf(msg.data(), "%d %u %d %s %s %d", SAGENext::VNC_SHARING, _uiclientid, 0, qPrintable(_vncUsername), qPrintable(_vncPasswd), 10);

	sendMessage(msg);
}

void SN_PointerUI::on_actionSend_text_triggered()
{
	if (!_tcpMsgSock.isValid()) {
		QMessageBox::warning(this, "No Connection", "Connect to SAGENext first");
		return;
	}

	SN_PointerUI_StrDialog strdialog;
	strdialog.exec();

	if (strdialog.result() == QDialog::Rejected) return;

	QString text = strdialog.text();
	if (text.isNull() || text.isEmpty()) {
		qDebug() << "text is null or empty";
		return;
	}

	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);

	sprintf(msg.data(), "%d %u %s", SAGENext::RESPOND_STRING, _uiclientid, qPrintable(text));
//	QTextStream ts(&msg, QIODevice::WriteOnly);
//	ts << (int)SAGENext::RESPOND_STRING << _uiclientid << text;

	sendMessage(msg);
}








void SN_PointerUI::initialize(quint32 uiclientid, int wallwidth, int wallheight, int ftpPort) {

	_uiclientid = uiclientid;
	fileTransferPort = ftpPort;

	ui->isConnectedLabel->setText("Connected to the wall");
	ui->isConnectedLabel->show();

	wallSize.rwidth() = wallwidth;
	wallSize.rheight() = wallheight;

	/*!
      When I send my mouse movement to the wall, it's pos should be scaled with these values. ( == mapToWall)
      */
	QDesktopWidget *d = QApplication::desktop();
	scaleToWallX = wallSize.width() / (qreal)(d->screenGeometry().width());
	scaleToWallY = wallSize.height() / (qreal)(d->screenGeometry().height());

	qDebug() << "================================================================================================";
	qDebug("My uiclientid is %u, The wall size is %.1f x %.1f, my primary screen size %d x %d, fileServer port %d", _uiclientid, wallSize.width(), wallSize.height(), d->screenGeometry().width(), d->screenGeometry().height(), fileTransferPort);
	qDebug("The scaleToWall %.2f x %.2f", scaleToWallX, scaleToWallY);
	qDebug() << "================================================================================================";

	//
	// maccapture
	//
#ifdef Q_OS_MAC
	if (!macCapture) {
		macCapture = new QProcess(this);
		macCapture->setWorkingDirectory(QCoreApplication::applicationDirPath());

		if ( ! QObject::connect(macCapture, SIGNAL(readyReadStandardOutput()), this, SLOT(sendMouseEventsToWall())) ) {
			qDebug() << "Can't connect macCapture signal to my slot. macCapture won't be started.";
		}
		else {
//			macCapture->start(QCoreApplication::applicationDirPath().append("/macCapture"));
			macCapture->start("./macCapture");
			if (! macCapture->waitForStarted(-1) ) {
//				qCritical() << "macCapture failed to start !!";
				QMessageBox::critical(this, "macCapture Error", "Failed to start macCapture");
			}
			else {
				QByteArray captureEdge(10, '\0');
				int edge = 3;// left 1, right, top, bottom (in macCapture)
				if ( _sharingEdge == "top" ) edge = 3;
				else if (_sharingEdge == "right") edge = 2;
				else if (_sharingEdge == "left") edge = 1;
				else if (_sharingEdge == "bottom") edge = 4;
				sprintf(captureEdge.data(), "%d %d\n", 14, edge);
				macCapture->write(captureEdge);
			}
		}
	}
#else
	//
	// this is to know whether the cursor is on _sharingEdge
	//
//	setMouseTracking(true); // the widget receives mouse move events even if no buttons are pressed
#endif
}






// for windows and Linux
void SN_PointerUI::on_hookMouseBtn_clicked()
{
	if (!_tcpMsgSock.isValid()) {
		QMessageBox::warning(this, "No Connection", "Connect to SAGENext first");
		return;
	}

	setMouseTracking(true);
	hookMouse();
}

void SN_PointerUI::hookMouse() {
	if (_tcpMsgSock.state() == QAbstractSocket::ConnectedState ) {
//		if (_msgChannelThread->isRunning()) {
			//
			// set the flag
			//
			isMouseCapturing = true;

			//
			// change cursor shape
			//
			setCursor(Qt::BlankCursor);


#ifdef Q_OS_MAC
			// do nothing
			ui->isConnectedLabel->setText("Pointer is in SAGENext");
#else
			ui->isConnectedLabel->setText("[Shift + Ctrl + Alt + m]\nto relase mouse");
			//
			// in Linux and Windows 7, this will work even the mouse curson isn't on this application window.
			//
			grabMouse();
//			qDebug() << "grabMouse";
#endif
			QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
			// msgtype, uiclientid, pointer name, Red, Green, Blue
			sprintf(msg.data(), "%d %u %s %s", SAGENext::POINTER_SHARE, _uiclientid, qPrintable(_pointerName), qPrintable(_pointerColor));
			sendMessage(msg);
//		}
//		else {
//			qWarning() << "Please connect to a SAGENext first";
//		}
	}
	else {
		qDebug() << "Sharing pointer: not connected to the wall";
	}
}

void SN_PointerUI::unhookMouse() {
	isMouseCapturing = false;
	unsetCursor();

#ifdef Q_OS_MAC
		// do nothing
#else
	releaseMouse();
//	qDebug() << "mouse released";
#endif
	// remove cursor on the wall
	if (_tcpMsgSock.state() == QAbstractSocket::ConnectedState) {
		QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
		sprintf(msg.data(), "%d %u", SAGENext::POINTER_UNSHARE, _uiclientid);
		sendMessage(msg);
	}
}


void SN_PointerUI::sendMessage(const QByteArray &msg) {
	if (_tcpMsgSock.isValid() && _tcpMsgSock.isWritable()) {
		if ( _tcpMsgSock.write(msg.constData(), msg.size()) == -1 ) {
			qDebug() << "QTcpSocket::write() error";
		}
		else {
			_tcpMsgSock.flush();
		}
	}
	else {
		qDebug() << "The message channel isn't writable";
	}
}

void SN_PointerUI::readMessage() {
	if (_tcpMsgSock.bytesAvailable() < EXTUI_MSG_SIZE) {
		qDebug() << "SN_PointerUI::readMessage()" << _tcpMsgSock.bytesAvailable() << "Bytes available to read";
		return;
	}

	QByteArray msg(EXTUI_MSG_SIZE, '\0');

	qint64 r = _tcpMsgSock.read(msg.data(), EXTUI_MSG_SIZE);
	Q_UNUSED(r);

	int msgType = 0;
	int msgCode = 0;
	sscanf(msg.constData(), "%d", &msgType);

	switch(msgType) {
	case SAGENext::ACK_FROM_WALL: {
		quint32 uiclientid;
		int wallwidth, wallheight, ftpport;
		sscanf(msg.constData(), "%d %u %d %d %d", &msgCode, &uiclientid, &wallwidth, &wallheight, &ftpport);
		_uiclientid = uiclientid;

		initialize(uiclientid, wallwidth, wallheight, ftpport);

		// Now I can connect to the file server
		if (ftpport > 0) {
			_tcpDataSock.connectToHost(QHostAddress(_wallAddress), ftpport);
			if ( _tcpDataSock.waitForConnected() ) {
				QByteArray msg(EXTUI_MSG_SIZE, 0);
				::sprintf(msg.data(), "%u", _uiclientid);
				_tcpDataSock.write(msg.constData(), msg.size());
			}
			else {
				qDebug() << "Failed to connect to file server";
			}
		}

		break;
	}
	}
}


void SN_PointerUI::receiveData() {

}

void SN_PointerUI::runSendFileThread(const QList<QUrl> &list) {
	if (!_tcpDataSock.isValid()) {
		QMessageBox::warning(this, "No Connection", "Connect to a SAGENext file server first");
		return;
	}

	QtConcurrent::run(this, &SN_PointerUI::sendFiles, list);
}

void SN_PointerUI::sendFiles(const QList<QUrl> &list) {
	foreach(QUrl url, list) {
		sendFile(url);
	}
}

void SN_PointerUI::sendFile(const QUrl &url) {
	char header[EXTUI_MSG_SIZE];
	int mediatype = 0;

	QRegExp _rxImage; _rxImage.setCaseSensitivity(Qt::CaseInsensitive);
	QRegExp _rxVideo; _rxVideo.setCaseSensitivity(Qt::CaseInsensitive);
	QRegExp _rxPdf; _rxPdf.setCaseSensitivity(Qt::CaseInsensitive);
	QRegExp _rxPlugin; _rxPlugin.setCaseSensitivity(Qt::CaseInsensitive);

	_rxImage.setPatternSyntax(QRegExp::RegExp);
	_rxVideo.setPatternSyntax(QRegExp::RegExp);
	_rxPdf.setPatternSyntax(QRegExp::RegExp);
	_rxPlugin.setPatternSyntax(QRegExp::RegExp);

	_rxImage.setPattern("(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$");
	_rxVideo.setPattern("(avi|mov|mpg|mpeg|mp4|mkv|flv|wmv)$");
	_rxPdf.setPattern("(pdf)$");
	_rxPlugin.setPattern("(so|dll|dylib)$");


	QString urlStr = url.toString().trimmed();

//	qDebug() << "encoded url" << url.toEncoded(); // ByteArray
//	qDebug() << "sendMedia() : urlstr is" << urlStr;

	if (urlStr.contains(QRegExp("^http", Qt::CaseInsensitive)) ) {
		::sprintf(header, "%d %s %lld", (int)SAGENext::MEDIA_TYPE_WEBURL, qPrintable(urlStr), (qint64)0);
		_tcpDataSock.write(header, sizeof(header));
	}
	else if (urlStr.contains(QRegExp("^file://", Qt::CaseSensitive)) ) {
		QFileInfo fi(url.toLocalFile());
		if (!fi.isFile() || fi.size() == 0) {
			qDebug() << "SendThread::sendMedia : filesize 0 for" << fi.absoluteFilePath();
			return;
		}
		qDebug() << "sendMedia() : filePath" << fi.filePath() << "fileName" << fi.fileName();

		if (fi.suffix().contains(_rxImage)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_IMAGE;
		}
		else if (fi.suffix().contains(_rxVideo)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_LOCAL_VIDEO; // because the file is going to be copied at the wall
		}
		else if (fi.suffix().contains(_rxPdf)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_PDF;
		}
		else if (fi.suffix().contains(_rxPlugin)) {
			mediatype = (int)SAGENext::MEDIA_TYPE_PLUGIN;
		}

		QString noSpaceFilename = fi.fileName().replace(QChar(' '), QChar('_'), Qt::CaseInsensitive);
		::sprintf(header, "%d %s %lld", mediatype, qPrintable(noSpaceFilename), fi.size());

		_tcpDataSock.write(header, sizeof(header));

		QFile file(fi.absoluteFilePath());
		file.open(QIODevice::ReadOnly);
		if ( _tcpDataSock.write(file.readAll().constData(), file.size()) < 0) {
			qDebug() << "SendThread::sendMedia() : error while sending the file";
		}
		file.close();
	}
}

void SN_PointerUI::readFiles(QStringList filenames) {
    if ( filenames.empty() ) {
        return;
    }
    else {
        //qDebug("MainWindow::%s() : %d", __FUNCTION__, filenames.size());
    }

	if (!_tcpMsgSock.isValid()) {
		qWarning() << "Please connect to a SAGENext first";
		return;
	}

	if (!_tcpDataSock.isValid()) {
		qWarning() << "Not connected to the file server";
		return;
	}

//    QRegExp rxVideo("\\.(avi|mov|mpg|mp4|mkv|mpg|flv|wmv|mpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
//    QRegExp rxImage("\\.(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
//	QRegExp rxPdf("\\.(pdf)$", Qt::CaseInsensitive, QRegExp::RegExp);


    for ( int i=0; i<filenames.size(); i++ ) {

        QString filename = filenames.at(i);
//		qDebug() << "readFile" << QUrl::fromLocalFile(filename);
        //qDebug("%s::%s() : %d, %s", metaObject()->className(), __FUNCTION__, i, qPrintable(filename));
//		QMetaObject::invokeMethod(sendThread, "sendMedia", Qt::QueuedConnection, Q_ARG(QUrl, QUrl::fromLocalFile(filename)));

		sendFile(QUrl::fromLocalFile(filename));

    }
}










/**
  This function is used by Mac OS X and invoked by QProcess::readyReadStandardOutput() signal
  */
void SN_PointerUI::sendMouseEventsToWall() {
	Q_ASSERT(macCapture);

//	QTextStream textin(macCapture);
	
	int msgcode = 0; // capture, release, move, click, wheel,..
	int x = 0.0, y = 0.0; // MOVE (x, y) is event globalPos
	int button  = 0; // left or right
	int state = 0; // pressed or released
	int wheelvalue = 0; // -1 and 1
	
	QByteArray msg(64, 0);
	QTextStream in(&msg, QIODevice::ReadOnly);
	while ( macCapture->readLine(msg.data(), msg.size()) > 0 ) {
		in >> msgcode;
		switch(msgcode) {
		
		// CAPTURED
		case 11: {
			qDebug() << "Start capturing Mac mouse events";
			hookMouse();
			break;
		}
			
		// RELEASED
		case 12: {
			qDebug() << "Stop capturing Mac mouse events";
			unhookMouse();
			break;
		}
			
		// MOVE, topLeft (0,0)  ->  bottomRight (screen width, screen height)
		case 1: {
			in >> x >> y;
			currentGlobalPos = QPoint(x, y);
//			qDebug() << "move from macCapture" << currentGlobalPos;
			if ( mouseBtnPressed == 1 ) {
				// left button dragging
				sendMouseMove(QPoint(x, y), Qt::LeftButton);
			}
			else if (mouseBtnPressed == 2 ) {
				// right button dragging
				sendMouseMove(QPoint(x, y), Qt::RightButton);
			}
			else {
				// just moving
				sendMouseMove(QPoint(x, y), Qt::NoButton);
			}
			break;
		}
		
		// CLICK Left(1) Right(2) Press(1) Release(0)
		case 2: {
			in >> button >> state;
//			qDebug() << "click from macCapture" << currentGlobalPos << button << state;
			
			//
			// left pressed
			//
			if ( button == 1 && state == 1 ) {
				mouseBtnPressed = 1;
				mousePressedPos = currentGlobalPos;

				// send left press
				sendMousePress(currentGlobalPos); // setAppUnderApp()
			}

			//
			// right pressed
			//
			else if ( button == 2 && state == 1) {
				mouseBtnPressed = 2;
				mousePressedPos = currentGlobalPos;

				// send right press (for selection rectangle)
				sendMousePress(currentGlobalPos, Qt::RightButton);
			}

			//
			// left released (left click)
			//
			else if ( button == 1 && state == 0 && mouseBtnPressed == 1) {
				// mouse left click
				mouseBtnPressed = 0;

				int ml = (currentGlobalPos - mousePressedPos).manhattanLength();

				//
				// pointerClick() won't be generated as a result of mouseDragging
				//
				if ( ml <= 3 ) {
					sendMouseClick(currentGlobalPos); // left click
				}

				//
				// pointerRelease() at the end of mouse dragging
				//
				else {

					// send left release (after dragging)
					sendMouseRelease(currentGlobalPos); // left release
				}
			}

			//
			// right released
			//
			else if ( button == 2 && state == 0 && mouseBtnPressed == 2) {
				// mouse right click
				mouseBtnPressed = 0;
				//
				// pointerClick() won't be generated as a result of mouseDragging
				//
				int ml = (currentGlobalPos - mousePressedPos).manhattanLength();
				if ( ml <= 3 ) {

					// send right click
					sendMouseClick(currentGlobalPos, Qt::RightButton); // right Click
				}
				else {

					// send right release (after dragging)
					sendMouseRelease(currentGlobalPos, Qt::RightButton); // right release
				}
			}
			break;
		}
			
		// double click (left only)
		case 5: {
			in >> button >> state;
//			qDebug() << "dbl click from macCapture" << currentGlobalPos;
			sendMouseDblClick(currentGlobalPos);
		}
			
		// WHEEL, scroll up(-1), down(1)
		case 3: {
			in >> wheelvalue;
			sendMouseWheel(currentGlobalPos, wheelvalue * -1);
			break;
		}
		}
	}
}




void SN_PointerUI::sendMouseMove(const QPoint globalPos, Qt::MouseButtons btns /*= Qt::NoButton*/) {
	int x = 0, y = 0;
	x = scaleToWallX * globalPos.x();
	y = scaleToWallY * globalPos.y();
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);

	if ( btns & Qt::LeftButton) {
//		qDebug() << "send left dargging";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_DRAGGING, _uiclientid, x, y);
	}
	else if (btns & Qt::RightButton) {
//		qDebug() << "sendMouseMove() Rightbutton dragging";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTDRAGGING, _uiclientid, x, y);
	}
	else {
		// just move pointer
//		qDebug() << "sendMouseMove() Moving" << globalPos;
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_MOVING, _uiclientid, x, y);
	}
	sendMessage(msg);
}

/**
  both mouse press is needed for dragging operation
  */
void SN_PointerUI::sendMousePress(const QPoint globalPos, Qt::MouseButtons btns /* Qt::LeftButton */) {
	int x=0 , y=0;
	x = scaleToWallX * globalPos.x();
	y = scaleToWallY * globalPos.y();
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
	
	if (btns & Qt::RightButton) {
		//
		// this is needed to setPos of selection rectangle
		//
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTPRESS, _uiclientid, x, y);
	}
	else {
		//
		// will trigger setAppUnderPointer() which is needed for left mouse dragging
		//
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_PRESS, _uiclientid, x, y);
	}
	sendMessage(msg);
}

/**
  Both mouse release are needed to know mouse dragging finish
  */
void SN_PointerUI::sendMouseRelease(const QPoint globalPos, Qt::MouseButtons btns /* = Qt::LeftButton */) {
	int x=0 , y=0;
	x = scaleToWallX * globalPos.x();
	y = scaleToWallY * globalPos.y();
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);

	if (btns & Qt::RightButton) {
		// will finish selection rectangle
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTRELEASE, _uiclientid, x, y);
	}
	else {
		// will pretend droping operation
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RELEASE, _uiclientid, x, y);
	}
	sendMessage(msg);
}

void SN_PointerUI::sendMouseClick(const QPoint globalPos, Qt::MouseButtons btns /* Qt::LeftButton | Qt::NoButton */) {
	int x=0 , y=0;
	x = scaleToWallX * globalPos.x();
	y = scaleToWallY * globalPos.y();
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
	
	if (btns & Qt::RightButton) {
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTCLICK, _uiclientid, x, y);
	}
	else {
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_CLICK, _uiclientid, x, y);
	}
	sendMessage(msg);
}

/**
  Left button only
  */
void SN_PointerUI::sendMouseDblClick(const QPoint globalPos, Qt::MouseButtons /*btns*/ /*= Qt::LeftButton | Qt::NoButton*/) {
	int x=0 , y=0;
	x = scaleToWallX * globalPos.x();
	y = scaleToWallY * globalPos.y();
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
	sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_DOUBLECLICK, _uiclientid, x, y);
	sendMessage(msg);
}

void SN_PointerUI::sendMouseWheel(const QPoint globalPos, int delta) {
	int x=0 , y=0;
	x = scaleToWallX * globalPos.x();
	y = scaleToWallY * globalPos.y();
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
//	qDebug() << "sendMouseWheel" << delta;
	sprintf(msg.data(), "%d %u %d %d %d", SAGENext::POINTER_WHEEL, _uiclientid, x, y, delta);
	sendMessage(msg);
}










void SN_PointerUI::mouseMoveEvent(QMouseEvent *e) {
//	qDebug() << "moveEvent" << e->globalPos();
	// setMouseTracking(true) to generate this event even when button isn't pressed
	if ( isMouseCapturing ) {
		//
		// e->button() is always 0
		// e->buttons() is only meaningful
		//
		sendMouseMove(e->globalPos(), e->buttons());
		e->accept();
	}
	else {
		QMainWindow::mouseMoveEvent(e);
	}
}

void SN_PointerUI::mousePressEvent(QMouseEvent *e) {
	// to keep track draggin start/end
	mousePressedPos = e->globalPos();

	if ( isMouseCapturing ) {
		qDebug() << "mousePressEvent()" << e->button() << "sending mouse PRESS";
		sendMousePress(e->globalPos());
		e->accept();
	}
	else {
		QMainWindow::mousePressEvent(e);
	}
}

void SN_PointerUI::mouseReleaseEvent(QMouseEvent *e) {
	if ( isMouseCapturing ) {
		//
		// below prevents pointerClick() when mouse was dragged
		//
		int ml = (e->globalPos() - mousePressedPos).manhattanLength();
		//		qDebug() << "release" << e->button() << e->globalPos() << ml;
		if ( ml <= 3 ) {
			qDebug() << "mouseReleaseEvent()" << e->button() << "sending mouse CLICK";
			sendMouseClick(e->globalPos(), e->button() | Qt::NoButton);
		}
		else {
			//
			// this is to know when dragging has finished
			//
			qDebug() << "mouseReleaseEvent()" << e->button() << "sending mouse RELEASE";
			sendMouseRelease(e->globalPos(), e->button() | Qt::NoButton);
		}
		e->accept();
	}
	else {
		QMainWindow::mouseReleaseEvent(e);
	}
}

void SN_PointerUI::contextMenuEvent(QContextMenuEvent *e) {
	if (isMouseCapturing) {
		//
		// right press (without following release) will trigger this
		//
//		qDebug() << "contextMenuEvent";
//		sendMousePress(e->globalPos(), Qt::RightButton); // Right press
		e->accept();
	}
}

void SN_PointerUI::mouseDoubleClickEvent(QMouseEvent *e) {
	if ( isMouseCapturing ) {
		qDebug() << "mouseDoubleClickEvent()" << e->button() << "sending mouse DBLCLICK";
		sendMouseDblClick(e->globalPos()); // Left double click
		e->accept();
	}
	else {
		QMainWindow::mouseDoubleClickEvent(e);
	}
}

void SN_PointerUI::wheelEvent(QWheelEvent *e) {
//	int numDegrees = e->delta() / 8;
//	int numTicks = numDegrees / 15;
//	qDebug() << "WHEEL" << e->globalPos() << "delta" << e->delta() << numTicks;
	
	if ( isMouseCapturing ) {
		sendMouseWheel(e->globalPos(), e->delta());
		e->accept();
	}
	else {
		QMainWindow::wheelEvent(e);
	}
}

























SN_PointerUI_DropFrame::SN_PointerUI_DropFrame(QWidget *parent)
    : QLabel(parent)
{
	setAcceptDrops(true);
	setFrameStyle(QFrame::Sunken);
	setFrameShape(QFrame::Box);
	setText("Drop media here");
	setAlignment(Qt::AlignCenter);
}

void SN_PointerUI_DropFrame::dragEnterEvent(QDragEnterEvent *e) {
	e->acceptProposedAction();
}

void SN_PointerUI_DropFrame::dropEvent(QDropEvent *e) {
//	if (! _tcpDataS ) {
//		QMessageBox::warning(this, "No Connection", "Connect to a SAGENext file server first");
//		return;
//	}

//	if (e->mimeData()->hasHtml()) {
////		qDebug() << "Html"; // web address
//		qDebug() << e->mimeData()->urls();
//	}
//	else if (e->mimeData()->hasUrls()) {
////		qDebug() << "Urls"; // all the files
//		QString filename = e->mimeData()->text().remove("file://"); // '\n' at the end of each filename
//		QStringList files = filename.split('\n', QString::SkipEmptyParts);
//		qDebug() << files;
//	}

	if ( e->mimeData()->hasHtml() || e->mimeData()->hasUrls()) {
		emit mediaDropped(e->mimeData()->urls());

		e->acceptProposedAction();
	}
}























SN_PointerUI_ConnDialog::SN_PointerUI_ConnDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SN_PointerUI_ConnDialog)
    , _settings(s)
    , portnum(0)
{
	ui->setupUi(this);

	addr.clear();

	ui->ipaddr->setInputMask("000.000.000.000;_");
	//        ui->myaddrLineEdit->setInputMask("000.000.000.000;_");
	ui->port->setInputMask("00000;_");

	ui->ipaddr->setText( _settings->value("walladdr", "127.0.0.1").toString() );


	/**
		QList<QHostAddress> myiplist = QNetworkInterface::allAddresses();
		for (int i=0; i<myiplist.size(); ++i) {
			if (myiplist.at(i).toIPv4Address()) {
				ui->myAddrCB->addItem(myiplist.at(i).toString(), myiplist.at(i).toIPv4Address()); // QString, quint32
			}
		}
		int currentIdx = ui->myAddrCB->findText( _settings->value("myaddr", "127.0.0.1").toString() );
		ui->myAddrCB->setCurrentIndex(currentIdx);
		**/



	ui->port->setText( _settings->value("wallport", 30003).toString() );
	ui->vncUsername->setText(_settings->value("vncusername", "user").toString());
	ui->vncpasswd->setEchoMode(QLineEdit::Password);
	ui->vncpasswd->setText(_settings->value("vncpasswd", "dummy").toString());
	ui->pointerNameLineEdit->setText( _settings->value("pointername", "pointer").toString());

	//		ui->pointerColorLabel->setText(_settings->value("pointercolor", "#FF0000").toString());
	QColor pc(_settings->value("pointercolor", "#ff0000").toString());
	pColor = pc.name();
	QPixmap pixmap(ui->pointerColorLabel->size());
	pixmap.fill(pc);
	ui->pointerColorLabel->setPixmap(pixmap);

	int idx = ui->sharingEdgeCB->findText(_settings->value("sharingedge", "top").toString());
	if (idx != -1)
		ui->sharingEdgeCB->setCurrentIndex(idx);
	else
		ui->sharingEdgeCB->setCurrentIndex(0);

	adjustSize();

//	ui->ipaddr->setText("127.0.0.1");
//	ui->port->setText("30003");
}

SN_PointerUI_ConnDialog::~SN_PointerUI_ConnDialog() {
	delete ui;
}

void SN_PointerUI_ConnDialog::on_buttonBox_accepted()
{
	addr = ui->ipaddr->text();
	portnum = ui->port->text().toInt();
//	myaddr = ui->myAddrCB->currentText();
	pName = ui->pointerNameLineEdit->text();
	vncusername = ui->vncUsername->text();
	vncpass = ui->vncpasswd->text();
	psharingEdge = ui->sharingEdgeCB->currentText();
	
	_settings->setValue("walladdr", addr);
	_settings->setValue("wallport", portnum);
//	_settings->setValue("myaddr", myaddr);
	_settings->setValue("pointername", pName);
	_settings->setValue("pointercolor", pColor);
	_settings->setValue("vncusername", vncusername);
	_settings->setValue("vncpasswd", vncpass);
	_settings->setValue("sharingedge", psharingEdge);
	
	if (vncusername.isEmpty()) {
		vncusername = "user";
	}
	
	accept();
	//	done(0);
}

void SN_PointerUI_ConnDialog::on_buttonBox_rejected()
{
	reject();
}

void SN_PointerUI_ConnDialog::on_pointerColorButton_clicked()
{
	QColor prevColor;
	prevColor.setNamedColor(_settings->value("pointercolor", "#FF0000").toString());
	QColor newColor = QColorDialog::getColor(prevColor, this, "Pointer Color"); // pops up modal dialog

	QPixmap pixmap(ui->pointerColorLabel->size());
	pixmap.fill(newColor);
//	ui->pointerColorLabel->setText(newColor.name());
	ui->pointerColorLabel->setPixmap(pixmap);

	pColor = newColor.name();
}












SN_PointerUI_StrDialog::SN_PointerUI_StrDialog(QWidget *parent)
    : QDialog(parent)
    , _lineedit(new QLineEdit(this))
    , _okbutton(new QPushButton("Ok", this))
    , _cancelbutton(new QPushButton("Cancel", this))
{
	_lineedit->setMaxLength(EXTUI_SMALL_MSG_SIZE - 1);

	connect(_okbutton, SIGNAL(clicked()), this, SLOT(setText()));
	connect(_cancelbutton, SIGNAL(clicked()), this, SLOT(reject()));



	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(_okbutton);
	layout->addWidget(_cancelbutton);


	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(_lineedit);
	mainLayout->addLayout(layout);

	setLayout(mainLayout);

	adjustSize();
}

void SN_PointerUI_StrDialog::setText() {
	_text = _lineedit->text();
	accept();
}


