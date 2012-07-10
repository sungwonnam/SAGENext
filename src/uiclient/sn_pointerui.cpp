#include "sn_pointerui.h"

#include "ui_sn_pointerui.h"

#include "sn_pointerui_conndialog.h"
#include "sn_pointerui_vncdialog.h"

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
#include <QSysInfo>


SN_PointerUI::SN_PointerUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SN_PointerUI)
    , _settings(0)
    , _uiclientid(0)
    , fileTransferPort(0)
    , ungrabMouseAction(0)
    , scaleToWallX(0.0)
    , scaleToWallY(0.0)
//    , sendThread(0)
	, isMouseCapturing(false)
    , mediaDropFrame(0)
	, _wallPort(0)
    , _sharingEdge(QString("right"))
	, macCapture(0)
	, _winCapture(0)
	, _winCaptureServer(0)

	, _iodeviceForMouseHook(0)

	, mouseBtnPressed(0)
    , _wasDblClick(false)
	, _prevClickTime(0)

{
	ui->setupUi(this);

//    setAttribute(Qt::WA_DeleteOnClose, true);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
	ui->hookMouseBtn->hide();
#endif

	ui->mainToolBar->addAction(ui->actionNew_Connection);
	ui->mainToolBar->addAction(ui->actionOpen_Media);
	ui->mainToolBar->addAction(ui->actionShare_desktop);
	ui->mainToolBar->addAction(ui->actionSend_text);


	_settings = new QSettings("EVL", "SAGENextPointer", this);

    //
    // handle msg socket error
    //
    QObject::connect(&_tcpMsgSock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleSocketError(QAbstractSocket::SocketError)));

    //
    // handle msg socket state change
    //
    QObject::connect(&_tcpMsgSock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(handleSocketStateChange(QAbstractSocket::SocketState)));

	//
	// to receive a message from SAGENext
	//
	QObject::connect(&_tcpMsgSock, SIGNAL(readyRead()), this, SLOT(readMessage()));
//	QObject::connect(&_tcpMsgSock, SIGNAL(disconnected()), &
//	QObject::connect(&_tcpDataSock, SIGNAL(readyRead()), this, SLOT(recvFileFromWall()));

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
	QObject::connect(fdialog, SIGNAL(filesSelected(QStringList)), this, SLOT(readLocalFiles(QStringList)));


	
	//
	// in SAGE image
	//
	QPixmap splashsmall(":/images/splash_small.png");
	_inSAGEsplash.setPixmap(splashsmall);
	_inSAGEsplash.setGeometry(0, 0, splashsmall.width(), splashsmall.height());
//	_inSAGEsplash.setFrameShape(QFrame::NoFrame);
	_inSAGEsplash.hide();




    //
    // now read the settings for _wallAddress and _wallPort
    //
    QString savedWallAddr = _settings->value("walladdr", "").toString();
    quint16 savedWallPort = _settings->value("wallport", 0).toUInt();

    if (!savedWallAddr.isEmpty()  &&  savedWallPort) {
        //
        // try connecting to the server
        //
        _wallAddress = savedWallAddr;
        _wallPort = savedWallPort;

        _pointerName = _settings->value("pointername", "pointerName").toString();
        _pointerColor = _settings->value("pointercolor", "#ff0000").toString();
        _sharingEdge = _settings->value("sharingedge", "right").toString();

        _tcpMsgSock.connectToHost(_wallAddress, _wallPort);
    }
    else {
        if ( ! QMetaObject::invokeMethod(this, "on_actionNew_Connection_triggered", Qt::QueuedConnection) ) {
            qDebug() << "invokeMethod() : on_actionNew_Connection_triggered failed";
        }
    }
}

SN_PointerUI::~SN_PointerUI()
{
    _tcpMsgSock.disconnect(); // disconnect all signal/slot connections

	delete ui;
	if (fdialog) delete fdialog;

	if (hasMouseTracking()) releaseMouse();

    if (_tcpMsgSock.state() == QAbstractSocket::ConnectingState)
        _tcpMsgSock.abort();
	_tcpMsgSock.close();

     if (_tcpDataSock.state() == QAbstractSocket::ConnectingState)
         _tcpDataSock.abort();
	_tcpDataSock.close();
	
	if (macCapture) {
		macCapture->kill();
//		delete macCapture; // doesn't need because macCapture is a child Qt object
		macCapture->waitForFinished(-1);
	}

	if (_winCapture) {
		_winCapture->kill();
		_winCapture->waitForFinished(-1);
	}

	if (_winCaptureServer) {
		_winCaptureServer->close();
	}

	if (_winCapturePipe.isOpen()) {
		_winCapturePipe.close();
	}

    qDebug() << "Good Bye";
}

void SN_PointerUI::m_deleteMouseHookProcess() {
	if (macCapture) {
        macCapture->kill();
        macCapture->waitForFinished(-1);
        delete macCapture;
        macCapture = 0;
    }

	if (_winCapture) {
		_winCapturePipe.close();

		_winCapture->kill();
		_winCapture->waitForFinished(-1);
		delete _winCapture;
		_winCapture = 0;
	}

	_iodeviceForMouseHook = 0;
}


void SN_PointerUI::handleSocketError(QAbstractSocket::SocketError error) {
    qDebug() << "SN_PointerUI::handleSocketError() :" << error;


    //
    // SAGENext is closed
    //
    if (error == QAbstractSocket::RemoteHostClosedError) {

        if (ui && ui->isConnectedLabel)
            ui->isConnectedLabel->setText("The Wall closed");

       m_deleteMouseHookProcess();

        QMetaObject::invokeMethod(this, "on_actionNew_Connection_triggered", Qt::QueuedConnection);
    }

    //
    // IP addr is valid, but SAGENext isn't available
    //
    else if (error == QAbstractSocket::ConnectionRefusedError) {

        ui->isConnectedLabel->setText("Failed to connect\nIs wall running?");
    }

	else {
		qDebug() << "SN_PointerUI::handleSocketError()" << error;
	}
}

void SN_PointerUI::handleSocketStateChange(QAbstractSocket::SocketState newstate) {
    switch (newstate) {
    case QAbstractSocket::UnconnectedState : {
        ui->isConnectedLabel->setText("Not Connected");

        //
        // Maybe schedule connection in 1 sec here ?
        //
		qDebug() << "SN_PointerUI::handleSocketStateChange() : UnConnectedState";

		m_deleteMouseHookProcess();

        break;
    }
    case QAbstractSocket::HostLookupState : {
        ui->isConnectedLabel->setText("Looking up host...");
        break;
    }
    case QAbstractSocket::ConnectingState : {
        ui->isConnectedLabel->setText("Connecting to...\n" + _wallAddress);
        break;
    }
    case QAbstractSocket::ConnectedState : {
        // the server will send ACK_FROM_WALL upon a client connection
        ui->isConnectedLabel->setText("Connected to Wall\n" + _wallAddress);
        break;
    }
    case QAbstractSocket::BoundState : {
        break;
    }
    case QAbstractSocket::ClosingState : {
		qDebug() << "SN_PointerUI::handleSocketStateChange() : ClosingState";
        break;
    }
    case QAbstractSocket::ListeningState : {
        break;
    }
    }
}

// triggered by CMD (CTRL) + n
void SN_PointerUI::on_actionNew_Connection_triggered()
{
    if (_tcpMsgSock.state() == QAbstractSocket::ConnectingState) {
        _tcpMsgSock.abort();
    }

    //
    // disconnect first
    //
    if (_tcpMsgSock.state() == QAbstractSocket::ConnectedState) {
		_tcpMsgSock.disconnectFromHost(); // will enter UnconnectedState after waiting all data has been written.
        _tcpDataSock.disconnectFromHost();

        QApplication::sendPostedEvents(); // empties the event Q
	}

    //
	// open modal dialog for a user to enter the IP address and port
    //
	SN_PointerUI_ConnDialog cd(_settings);
	cd.exec();
	if ( cd.result() == QDialog::Rejected) return;


    if (!cd.ipaddress().isNull()) {
        _wallAddress = cd.ipaddress().toString();
    }
    else if (!cd.hostname().isEmpty()) {
        _wallAddress = cd.hostname();
    }
    else {
        QMessageBox::warning(this, "Error : Can't connect", "Both Hostname and IP address are empty");
        return;
    }
    _wallPort = cd.port();

	_pointerName = cd.pointerName();
	_pointerColor = cd.pointerColor();
//	_myIpAddress = cd.myAddress();
	_sharingEdge = cd.sharingEdge();

	_tcpMsgSock.connectToHost(_wallAddress, _wallPort);
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

	//
	// Open a dialog for username/password
	//
	SN_PointerUI_VncDialog vncDialog;
	int ret = vncDialog.exec(); // modal dialog

	if (ret == QDialog::Rejected) return;
	
	QString vncUsername = "";
	QString vncPasswd = "";

#ifdef Q_OS_MAC
    //
    // Lion and higher
    //
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        vncUsername = vncDialog.username();
    }
#endif

	vncPasswd = vncDialog.password();


	// send msg to UiServer so that local sageapp (vncviewer) can be started
	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);

	// msgtype, uiclientid, senderIP, display #, vnc passwd, framerate

	sprintf(msg.data(), "%d %d %s %s %d"
            , SAGENext::VNC_SHARING
            , 0
            , (vncUsername.isEmpty()) ? "_" : qPrintable(vncUsername)
            , (vncPasswd.isEmpty()) ? "_" : qPrintable(vncPasswd)
            , 10
            );

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

	sprintf(msg.data(), "%d %s", SAGENext::RESPOND_STRING, qPrintable(text));
//	QTextStream ts(&msg, QIODevice::WriteOnly);
//	ts << (int)SAGENext::RESPOND_STRING << _uiclientid << text;

	sendMessage(msg);
}







/**
  This is called upon receiving ACK_FROM_WALL
  */
void SN_PointerUI::initialize(quint32 uiclientid, int wallwidth, int wallheight, int ftpPort) {

	_uiclientid = uiclientid;
	fileTransferPort = ftpPort;

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
#if defined(Q_OS_MAC)
	if (!macCapture) {
		macCapture = new QProcess(this);
		macCapture->setWorkingDirectory(QCoreApplication::applicationDirPath());

		if ( ! QObject::connect(macCapture, SIGNAL(readyReadStandardOutput()), this, SLOT(readFromMouseHook())) ) {
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
				_iodeviceForMouseHook = macCapture;

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
#elif defined(Q_OS_WIN32)
	if (!_winCaptureServer) {
		_winCaptureServer = new SN_WinCaptureTcpServer(&_winCapturePipe, this);
	}

	if (!_winCaptureServer->isListening()) {
		_winCaptureServer->listen(QHostAddress::LocalHost, 44556);
	}

	while(!_winCaptureServer->isListening()) {} // busy waiting

	int sedge  = 1;
	if (QString::compare("left", _sharingEdge, Qt::CaseInsensitive) == 0) sedge = 1;
	else if (QString::compare("right", _sharingEdge, Qt::CaseInsensitive) == 0) sedge = 2;
	else if (QString::compare("top", _sharingEdge, Qt::CaseInsensitive) == 0) sedge = 3;
	else if (QString::compare("bottom", _sharingEdge, Qt::CaseInsensitive) == 0) sedge = 4;

	QObject::connect(&_winCapturePipe, SIGNAL(readyRead()), this, SLOT(readFromMouseHook()));
	if (!_winCapture) {
		_winCapture = new QProcess(this);
		_winCapture->setWorkingDirectory(QCoreApplication::applicationDirPath());
		_winCapture->start("winCapture" + QString(QDir::separator()) + "winCapture " + QString::number(d->screenGeometry().width()) + " " + QString::number(d->screenGeometry().height()) + " " + QString::number(sedge));
		if (! _winCapture->waitForStarted(-1) ) {
			QMessageBox::critical(this, "winCapture Error", "Failed to start winCapture");
		}
		else {
			_iodeviceForMouseHook = &_winCapturePipe;
		}
	}
#else
	//
	// this is to know whether the cursor is on _sharingEdge
	//
//	setMouseTracking(true); // the widget receives mouse move events even if no buttons are pressed
#endif


    //
    //  connect to udp socket
    //
    /*
    _udpSocket.connectToHost(QHostAddress(_wallAddress), _wallPort + 10 + _uiclientid);
    if (! _udpSocket.waitForConnected(-1)) {
        qDebug() << "SN_PointerUI::initialize() : udpSocket connectToHost() failed";
    }
    else {
        qDebug() << "SN_PointerUI::initialize() : connected" << _udpSocket.state();
    }
    */
}


void SN_PointerUI::readFromWinCapture() {
	Q_ASSERT(_winCapturePipe.isOpen());
	Q_ASSERT(_winCapturePipe.isReadable());

	QByteArray msg(64, 0);
	while (_winCapturePipe.readLine(msg.data(), msg.size()) > 0) {
		qDebug() << "readFromWinCapture() : " << msg;
	}
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
			
			_inSAGEsplash.show();

			//
			// change cursor shape
			//
			setCursor(Qt::BlankCursor);


#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
			// do nothing
			ui->isConnectedLabel->setText("Pointer is in SAGENext");
#else
            ui->hookMouseBtn->setText("Pointer is shared");
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
	_inSAGEsplash.hide();
	isMouseCapturing = false;
	unsetCursor();

#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
		// do nothing
#else
	releaseMouse();
//	qDebug() << "mouse released";
    ui->hookMouseBtn->setText("share pointer");
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
//			qDebug() << "sendMessage() :" << msg;
			_tcpMsgSock.flush();
		}
	}
	else {
		qDebug() << "The message channel isn't writable";
	}

    /*
    if (!_udpSocket.isValid()) {
        qDebug() << "udp socket invalid";
        return;
    }
    if (!_udpSocket.isWritable()) {
        qDebug() << "udp socket isn't writable";
        return;
    }
    if ( _udpSocket.write(msg) == -1 ) {
        qDebug() << "_udpSocket.write error";
    }
    else {
        _udpSocket.flush();
        qDebug() << "msg sent" << msg;
    }
    */
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
			_tcpDataSock.connectToHost(_wallAddress, ftpport);
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
	case SAGENext::RESPOND_FILEINFO: {
//		sprintf(msg.data(), "%d %d %s %lld", SAGENext::RESPOND_FILEINFO, mtype, qPrintable(bw->appInfo()->fileInfo().fileName()), filesize);

		SAGENext::MEDIA_TYPE mtype;
		char filepath[512];
		qint64 filesize;
		sscanf(msg.constData(), "%d %d %s %lld", &msgCode, &mtype, filepath, &filesize);

		qDebug() << "RESPOND_FILEINFO" << filepath << filesize;

		runRecvFileThread(QString(filepath), filesize);

		break;
	}
	}
}




void SN_PointerUI::runSendFileThread(const QList<QUrl> &list) {
	if (!_tcpDataSock.isValid()) {
		QMessageBox::warning(this, "Socket invalid", "Connect to a SAGENext file server first");
		return;
	}

	if (!_tcpDataSock.isWritable()) {
		QMessageBox::warning(this, "Can't write to the socket", "The channel isn't writable");
		return;
	}

	//
	// run the sendFilesToWall() in a separate thread
	//
	QtConcurrent::run(this, &SN_PointerUI::sendFilesToWall, list);
}

void SN_PointerUI::runRecvFileThread(const QString &filepath, qint64 filesize) {
	if (!_tcpDataSock.isValid()) {
		QMessageBox::warning(this, "Socket invalid", "Connect to a SAGENext file server first");
		return;
	}
	if (!_tcpDataSock.isReadable()) {
		QMessageBox::warning(this, "Can't read from the socket", "The channel isn't readable");
		return;
	}

	//
	// run the recvFileFromWall() in a separate thread
	//
	QtConcurrent::run(this, &SN_PointerUI::recvFileFromWall, filepath, filesize);
}

void SN_PointerUI::sendFilesToWall(const QList<QUrl> &list) {
	foreach(QUrl url, list) {
		sendFileToWall(url);
	}
}

void SN_PointerUI::sendFileToWall(const QUrl &url) {
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
		::sprintf(header, "%d %d %s %lld", 0, (int)SAGENext::MEDIA_TYPE_WEBURL, qPrintable(urlStr), (qint64)0);
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

		//
		// 0 for uploading to wall
		//
		::sprintf(header, "%d %d %s %lld", 0, mediatype, qPrintable(noSpaceFilename), fi.size());

		//
		// send the header first
		//
		_tcpDataSock.write(header, sizeof(header));


		//
		// followed by the file
		//
		QFile file(fi.absoluteFilePath());
		file.open(QIODevice::ReadOnly);
		if ( _tcpDataSock.write(file.readAll().constData(), file.size()) < 0) {
			qDebug() << "SendThread::sendMedia() : error while sending the file";
		}
		file.close();
	}
}

void SN_PointerUI::recvFileFromWall(const QString &filepath, qint64 filesize) {
	//
	// REQUEST_FILEINFO to UiServer
	//
//	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
//	::sprintf(msg.data(), "%d %llu", SAGENext::REQUEST_FILEINFO, 0); // 0 to request fileinfo of selected (what I recently clicked) application

	//
	// send header to the fileServer
	// 1 for downloading from wall
	//
	char header[EXTUI_MSG_SIZE];
	memset(header, 0, EXTUI_MSG_SIZE);
	::sprintf(header, "%d %d %s %lld", 1, -1, qPrintable(filepath), filesize); // mediatype is dummy here

	//
	// send the header first
	//
	_tcpDataSock.write(header, sizeof(header));

	//
	// extract filename only
	// because the file path is of the wall
	//
	QFileInfo fi(filepath);
	QString filename = fi.fileName();

	qDebug() << "recvFileFromWall" << filepath << filename;

	//
	// now receive a file
	//
	QFile file(filename);

	if ( ! file.open(QIODevice::WriteOnly) ) {
		qDebug("%s::%s() : failed to open the file", metaObject()->className(), __FUNCTION__);
		return;
	}
	qDebug() << "I will receive" << file.fileName() << filesize << "Byte";

	QByteArray buffer(filesize, 0);
	if ( _tcpDataSock.read(buffer.data(), filesize) < 0 ) {
        qCritical("%s::%s() : error while receiving the file.", metaObject()->className(), __FUNCTION__);
        return;
    }
	file.write(buffer);

	if (!file.exists() || file.size() <= 0) {
		qDebug("%s::%s() : %s is not a valid file", metaObject()->className(), __FUNCTION__, qPrintable(file.fileName()));
		return;
	}

	file.close();
}

void SN_PointerUI::readLocalFiles(QStringList filenames) {
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

		sendFileToWall(QUrl::fromLocalFile(filename));

    }
}










/**
  This function is used by Mac OS X and invoked by QProcess::readyReadStandardOutput() signal
  */
void SN_PointerUI::readFromMouseHook() {
	if (!_iodeviceForMouseHook) return;
	
	int msgcode = 0; // capture, release, move, click, wheel,..
	int x = 0.0, y = 0.0; // MOVE (x, y) is event globalPos
	int button  = 0; // left or right
	int state = 0; // pressed or released
	int wheelvalue = 0; // -1 and 1
	
	QByteArray msg(64, 0);
	QTextStream in(&msg, QIODevice::ReadOnly);
	while ( _iodeviceForMouseHook->readLine(msg.data(), msg.size()) > 0 ) {

//		qDebug() << msg;

		in >> msgcode;
		switch(msgcode) {
		
		// CAPTURED
		case 11: {
//			qDebug() << "Start capturing mouse events";
			ui->isConnectedLabel->setText("Pointer is in the SAGENext !");
			hookMouse();
			break;
		}
			
		// RELEASED
		case 12: {
//			qDebug() << "Stop capturing mouse events";
			ui->isConnectedLabel->setText("Move your cursor to the " + _sharingEdge +" edge\nto share your pointer");
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
#ifdef Q_OS_WIN32
				qint64 now = QDateTime::currentMSecsSinceEpoch();
				
				// if this click is occurred pretty quickly (250 msec)
				// then it's dbl click
				if (now - _prevClickTime < 250) {
					sendMouseDblClick(currentGlobalPos);
					_wasDblClick = true;
				}
				else
					sendMousePress(currentGlobalPos); // normal left press
#else
				
				// send left press
				sendMousePress(currentGlobalPos); // setAppUnderApp()
#endif
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
				
				if (_wasDblClick) {
					_wasDblClick = false;
				}
				else {
					
					int ml = (currentGlobalPos - mousePressedPos).manhattanLength();
					
					//
					// pointerClick() won't be generated as a result of mouseDragging
					//
					if ( ml <= 3 ) {
#if defined(Q_OS_WIN32)
						_prevClickTime = QDateTime::currentMSecsSinceEpoch();
#endif
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
			break;
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
//		qDebug() << "POINTER_DRAGGING";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_DRAGGING, _uiclientid, x, y);
	}
	else if (btns & Qt::RightButton) {
//		qDebug() << "POINTER_RIGHTDRAGGING";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTDRAGGING, _uiclientid, x, y);
	}
	else {
		// just move pointer
//		qDebug() << "POINTER_MOVING" << globalPos;
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
//		qDebug() << "POINTER_RIGHTPRESS";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTPRESS, _uiclientid, x, y);
	}
	else {
		//
		// will trigger setAppUnderPointer() which is needed for left mouse dragging
		//
//		qDebug() << "POINTER_PRESS";
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
//		qDebug() << "POINTER_RIGHTRELEASE";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTRELEASE, _uiclientid, x, y);
	}
	else {
		// will pretend droping operation
//		qDebug() << "POINTER_RELEASE";
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
//		qDebug() << "POINTER_RIGHTCLICK";
		sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_RIGHTCLICK, _uiclientid, x, y);
	}
	else {
//		qDebug() << "POINTER_CLICK";
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
	
//	qDebug() << "POINTER_DOUBLECLICK";
	sprintf(msg.data(), "%d %u %d %d", SAGENext::POINTER_DOUBLECLICK, _uiclientid, x, y);
	sendMessage(msg);


/*
	sendMouseClick(globalPos);

	QByteArray msg(EXTUI_SMALL_MSG_SIZE, 0);
	sprintf(msg.data(), "%d %llu", SAGENext::REQUEST_FILEINFO, (quint64)0);
	sendMessage(msg);
	*/
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

            if (_wasDblClick) {
                _wasDblClick = false; // reset the flag
            }
            else {
                qDebug() << "mouseReleaseEvent()" << e->button() << "sending mouse CLICK";
                sendMouseClick(e->globalPos(), e->button() | Qt::NoButton);
            }
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

        _wasDblClick = true;

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





