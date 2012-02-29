#ifndef EXTERNALGUIMAIN_H
#define EXTERNALGUIMAIN_H

#include <QtGui>
//#include <QHostAddress>
//#include <QAbstractSocket>
#include <QSettings>
#include <QTcpSocket>

#include "sn_pointerui_msgthread.h"
#include "sn_pointerui_sendthread.h"


#include "../thewall/common/commondefinitions.h"

namespace Ui {
class SN_PointerUI;
class SN_PointerUI_ConnDialog;
}


/**
  Provides drag & drop feature
  */
class SN_PointerUI_DropFrame : public QLabel {
	Q_OBJECT
public:
	explicit SN_PointerUI_DropFrame(QWidget *parent = 0);

protected:
	void dragEnterEvent(QDragEnterEvent *);
	void dropEvent(QDropEvent *);

signals:
	void mediaDropped(QList<QUrl> mediaurls);
};



/**
  sageNextPointer MainWindow
  */
class SN_PointerUI : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit SN_PointerUI(QWidget *parent = 0);
	~SN_PointerUI();
	
protected:
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *e);
	
	/**
	  Override this to provide mouse right click
	 */
	void contextMenuEvent(QContextMenuEvent *event);
	
	
	void sendMouseMove(const QPoint globalPos, Qt::MouseButtons btns = Qt::NoButton);

	/**
	  This will trigger pointer->setAppUnderPointer()
	  */
	void sendMousePress(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton);

	/**
	  This is NOT the mouseRelease that results pointerClick()
	  This is to know the point where mouse draggin has finished
	  */
	void sendMouseRelease(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton);

	/**
	  mouse press followed by mouse release triggers this.
	  However, if mouse has moved (while button pressed) greater than 3 manhattan length, then it's not a click
	  */
	void sendMouseClick(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton | Qt::NoButton);

	void sendMouseDblClick(const QPoint globalPos, Qt::MouseButtons btns = Qt::LeftButton | Qt::NoButton);

	void sendMouseWheel(const QPoint globalPos, int delta);

private:
	Ui::SN_PointerUI *ui;

	QSettings *_settings;

        /**
          unique ID of me used by UiServer to differentiate multiple ui clients.
          Note that uiclientid is unique ONLY within the wall represented by sockfd.
          It is absolutely valid and likely that multiple message threads have same uiclientid value.

          This should be map data structure to support multiple wall connections
          */
	quint32 _uiclientid;

	/**
	  receives this from UiServer upon connection.
	  It is defined in "general/fileserverport'
	  */
	int fileTransferPort;

        /**
          * SHIFT + CMD + ALT + m
          */
	QAction *ungrabMouseAction;


	/**
      The socket for the message channel.
	  All the mouse events is sent throught this socket.
      */
	QTcpSocket _tcpMsgSock;


	/**
	  The socket for file transferring (sendThread)
	  */
	QTcpSocket _tcpDataSock;


	/**
       When I send my coord to the wall, I'll multiple this to map my local coord to the wall coord.
	  */
	qreal scaleToWallX;
	qreal scaleToWallY;

        /**
          * when I receive app geometry from the wall
          */
//	qreal scaleFromWallX;
//	qreal scaleFromWallY;


		/**
		  Width, Height of the wall in pixel
		  */
	QSizeF wallSize;

        /**
          * non-modal file dialog
          */
	QFileDialog *fdialog;


        /**
          The file transfer thread. _tcpDataSock is used.
          */
	SN_PointerUI_DataThread *sendThread;

		/**
		  to keep track mouse dragging start and end position
		  */
	QPoint mousePressedPos;
		
		/**
		  true if pointer sharing is on
		  */
	bool isMouseCapturing;

		/**
		  This is QLabel that accept dropEvent
		  */
	SN_PointerUI_DropFrame *mediaDropFrame;

	QString _wallAddress;

	QString _pointerName;

	QString _pointerColor;

//	QString _myIpAddress;

	QString _vncUsername;

	QString _vncPasswd;

	QString _sharingEdge;
		
		/**
		  For mac users, start macCapture process written by Ratko to intercept mac's mouse events
		  11 :start capturing 
		  12 : no capturing (mouse is back to my desktop)
		  MOVE(1) x(0~1) y(0~1)
          CLICK(2) LEFT(1) PRESS
		  CLICK(2) LEFT(1) RELEASE
		  CLICK(2) RIGHT(2) PRESS
		  CLICK(2) RIGHT(2) RELEASE
		  WHEEL(3) 
		  DOUBLE_CLICK(5) int
		  **/
	QProcess *macCapture;
		
		/**
		  This is for macCapture
		  to figure out mouse button state when using macCapture
		  0 nothing
		  1 left button pressed
		  2 right button pressed
		  */
	int mouseBtnPressed;
		
		/**
		  This is for macCapture
		  to keep track current mouse position when click/dblclick/wheel happens
		  */
	QPoint currentGlobalPos;


		/**
		  Queue invoking MessageThread::sendMsg()
		  */
//	void queueMsgToWall(const QByteArray &msg);
		
//	void connectToWall(const char * ipaddr, quint16 port);

	void sendFilesToWall(const QList<QUrl> &);

	/*!
	  send a file to wall. Sending consists of two operations. Sending a header followed by the actual file.
	  */
	void sendFileToWall(const QUrl &);

	/*!
	  The filepath is the absoulte path name of the file reside at the wall.
	  */
	void recvFileFromWall(const QString &filepath, qint64 filesize);

public slots:
	/*!
	  Upon connection, receive wall size, uiclientid and sets scaleToWallX/Y
	  */
	void initialize(quint32 uiclientid, int wallwidth, int wallheight, int ftpPort);

	/*!
	  send message to the wall. messages are usually pointer operations.
	  The size of the message from uiclient to wall is EXTUI_SMALL_MSG_SIZE = 128 Byte. This is defined in src/thewall/common/commondefinitions.h
	  */
	void sendMessage(const QByteArray &msg);

	/*!
	  read message sent from the wall.
	  The size of the message from wall to uiclient is EXTUI_MSG_SIZE = 1280 Byte. This is defined in src/thewall/common/commondefinitions.h
	  */
	void readMessage();

	/*!
	  This function runs SN_PointerUI::sendFiles() in a separate thread using QtConcurrent::run()
	  */
	void runSendFileThread(const QList<QUrl> &);

	void runRecvFileThread(const QString &filepath, qint64 filesize);


private slots:
        /*!
          CMD + N triggers the connection dialog.
		  Upon accepting the dialog, it attempts to connect to the wall with the _tcpMsgSock socket.

		  If there already exist the message channel. The _tcpMsgSock will be closed.
          */
	void on_actionNew_Connection_triggered();


        /**
          * This is for Windows OS temporarily
          */
	void on_hookMouseBtn_clicked();

	void hookMouse();

	void unhookMouse();


        /**
          shortcut : CTRL(CMD) + O
		  The action (ui->actionOpen_Media) is defined in the sn_pointer.ui
          */
	void on_actionOpen_Media_triggered();

        /**
          * when fileDialog returns this function is invoked.
          * This functions will invoke MessageThread::registerApp() slot
          */
	void readLocalFiles(QStringList);
		
		//void sendFile(const QString &f, int mediatype);
		
		/**
		  respond to macCapture (read from stdout, translate msg, send to the wall)
		  This slot is connected to QProcess::readyReadStandardOutput() signal
		  */
	void sendMouseEventsToWall();
		

	/*!
	  VNC sharing.
	  The action (ui->actionShare_Desktop) is defined in the sn_pointerui.ui
	  */
	void on_actionShare_desktop_triggered();


	/*!
	  send text to the current application.
	  It assumes that a user 'clicked' a GUI item ,to which the user wants to send text, of the application.
	  The action (ui->actionSend_text is defined in the sn_pointerui.ui
	  */
	void on_actionSend_text_triggered();
};








class SN_PointerUI_ConnDialog : public QDialog {
	Q_OBJECT
public:
	SN_PointerUI_ConnDialog(QSettings *s, QWidget *parent=0);
	~SN_PointerUI_ConnDialog();

	inline QString address() const {return addr;}
	inline int port() const {return portnum;}
	//inline QString myAddress() const {return myaddr;}
	inline QString pointerName() const {return pName;}
	inline QString pointerColor() const {return pColor;}
	inline QString vncUsername() const {return vncusername;}
	inline QString vncPasswd() const {return vncpass;}
	inline QString sharingEdge() const {return psharingEdge;}

private:
	Ui::SN_PointerUI_ConnDialog *ui;
	QSettings *_settings;

        /**
          wall address
          */
	QString addr;

        /**
          wall port
          */
	quint16 portnum;

	QString vncusername;

	QString vncpass;

        /**
          pointer name
          */
	QString pName;

	QString pColor;

	QString psharingEdge;

private slots:
	void on_buttonBox_rejected();
	void on_buttonBox_accepted();
	void on_pointerColorButton_clicked();
};

















class SN_PointerUI_StrDialog : public QDialog
{
	Q_OBJECT
public:
	SN_PointerUI_StrDialog(QWidget *parent=0);

	inline QString text() {return _text;}

private:
	QLineEdit *_lineedit;
	QPushButton *_okbutton;
	QPushButton *_cancelbutton;

	QString _text;

public slots:
	void setText();
};

#endif // EXTERNALGUIMAIN_H
