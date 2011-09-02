#ifndef EXTERNALGUIMAIN_H
#define EXTERNALGUIMAIN_H

#include <QtGui>
#include <QHostAddress>
//#include <QAbstractSocket>
#include <QSettings>

#include "messagethread.h"
#include "sendthread.h"


#define EXTUI_MSG_SIZE 1280

/* types of messages between external UI and the wall */
enum EXTUI_MSG_TYPE { MSG_NULL, REG_FROM_UI, ACK_FROM_WALL, DISCONNECT_FROM_WALL, WALL_IS_CLOSING, TOGGLE_APP_LAYOUT, RESPOND_APP_LAYOUT, VNC_SHARING, POINTER_PRESS, POINTER_RIGHTPRESS, POINTER_RELEASE, POINTER_RIGHTRELEASE, POINTER_CLICK, POINTER_RIGHTCLICK, POINTER_DOUBLECLICK, POINTER_DRAGGING, POINTER_MOVING, POINTER_SHARE, POINTER_WHEEL, POINTER_UNSHARE };

/* transfer file / stream pixel / stream file */
enum EXTUI_TRANSFER_MODE { FILE_TRANSFER, FILE_STREAM, PIXEL_STREAM };

enum MEDIA_TYPE {MEDIA_TYPE_UNKNOWN = 100, MEDIA_TYPE_IMAGE, MEDIA_TYPE_VIDEO, MEDIA_TYPE_LOCAL_VIDEO, MEDIA_TYPE_AUDIO, MEDIA_TYPE_PLUGIN };

namespace Ui {
        class ExternalGUIMain;
        class connectionDialog;
}


class DropFrame : public QFrame {
public:
        explicit DropFrame(QWidget *parent = 0);

protected:
        void dragEnterEvent(QDragEnterEvent *);
        void dropEvent(QDropEvent *);
};



class ExternalGUIMain : public QMainWindow
{
        Q_OBJECT

public:
        explicit ExternalGUIMain(QWidget *parent = 0);
        ~ExternalGUIMain();

protected:
        void resizeEvent(QResizeEvent *);
        void mouseMoveEvent(QMouseEvent *e);
        void mousePressEvent(QMouseEvent *e);
        void mouseReleaseEvent(QMouseEvent *e);
        void mouseDoubleClickEvent(QMouseEvent *e);
        void wheelEvent(QWheelEvent *e);

private:
        Ui::ExternalGUIMain *ui;

        QSettings *_settings;

        /*!
          unique ID of me used by UiServer to differentiate multiple ui clients.
          Note that uiclientid is unique ONLY within the wall represented by sockfd.
          It is absolutely valid and likely that multiple message threads have same uiclientid value.

          This should be map data structure to support multiple wall connections
          */
        quint64 uiclientid;

        int fileTransferPort;

//        QTimer *timer;

        /**
          * SHIFT + CMD + ALT + m
          */
        QAction *ungrabMouseAction;

        /**
          * wall address and port
          */
//	QHostAddress wallAddr;

        //QMap<quint64, QGraphicsScene *> wallSceneMap;

        /**
          socket that connect my msgthread and wall's uimsgthread
          */
        int msgsock;


        /**
          * when I send my geometry to the wall
          */
        qreal scaleToWallX;
        qreal scaleToWallY;

        /**
          * when I receive app geometry from the wall
          */
//	qreal scaleFromWallX;
//	qreal scaleFromWallY;


        QSizeF wallSize;

        /**
          * non-modla file dialog
          */
        QFileDialog *fdialog;

        /**
          * message thread
          */
        MessageThread *msgThread;

        /**
          file transfer thread
          */
        SendThread *sendThread;


        QPoint mousePressedPos;

        DropFrame *mediaDropFrame;

        QString _pointerName;

        QString _myIpAddress;

        QString _vncPasswd;


//	void connectToWall(const char * ipaddr, quint16 port);

private slots:
        /**
          * CMD + N
          */
        void on_actionNew_Connection_triggered();

        void on_vncButton_clicked();

        /**
          * will set mouseTracking(true) and grabMouse()
          */
        void on_pointerButton_clicked();

        void ungrabMouse();



        /**
          * CMD + O
          */
        void on_actionOpen_Media_triggered();

        /**
          * when fileDialog returns this function is invoked.
          * This functions will invoke MessageThread::registerApp() slot
          */
        void readFiles(QStringList);
//	void sendFile(const QString &f, int mediatype);

        /**
          * RESPOND_APP_LAYOUT handler
          */
//	void updateScene(const QByteArray layout);


        /*!
          receive wall layout and update scene continusously
          */
//	void on_showLayoutButton_clicked();


        QGraphicsRectItem * itemWithGlobalAppId(QGraphicsScene *scene, quint64 gaid);

};







class AppRect : public QGraphicsRectItem {
public:
        AppRect(QGraphicsItem *parent=0);

protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};





class ConnectionDialog : public QDialog {
        Q_OBJECT
public:
        ConnectionDialog(QSettings *s, QWidget *parent=0);
        ~ConnectionDialog();

        inline QString address() const {return addr;}
        inline int port() const {return portnum;}
        inline QString myAddress() const {return myaddr;}
        inline QString pointerName() const {return pName;}
        inline QString vncPasswd() const {return vncpass;}

private:
        Ui::connectionDialog *ui;
        QSettings *_settings;

        /**
          wall address
          */
        QString addr;

        /**
          wall port
          */
        quint16 portnum;

        /**
          my ip address
          */
        QString myaddr;

        QString vncpass;

        /**
          pointer name
          */
        QString pName;

private slots:
        void on_buttonBox_rejected();
        void on_buttonBox_accepted();
};

#endif // EXTERNALGUIMAIN_H
