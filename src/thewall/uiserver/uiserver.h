#ifndef UISERVER_H
#define UISERVER_H


#include "uimsgthread.h"
#include "../common/commondefinitions.h"
//#include "../common/commonitem.h"

#include <QTcpServer>



class SN_TheScene;
class SN_Launcher;
class QSettings;
class SN_PolygonArrowPointer;


class SN_UiServer : public QTcpServer
{
	Q_OBJECT
public:
	explicit SN_UiServer(const QSettings *s, SN_Launcher *snl, SN_TheScene *sns);
	~SN_UiServer();

        /**
          * This function is called by the main process (GraphicsViewMain::TimerEvent()) every 1sec (TimerEvent of the main process)
          * It updates this->appLayout and *BROADCAST* this buffer to ALL ui clients
          */
//        void updateAppLayout();

protected:
        /**
          * overrides QTcpServer::incomingConnection()
          * The base implementation emits newConnection() signal
          *
          * creates a ui msg thread for each connection
          * insert this connection to clientMap
          */
	void incomingConnection(qintptr sockfd);

private:
        //QHostAddress uiServerIP;
	const QSettings *_settings;

        /**
          TCP port number base for uiclient to connect to transfer a file
          */
	int _fileRecvPortBase;

        /*!
          * Each ui client has to have unique identifier
          This id will be set to 0 if fileRecvPortBase + id >= 65535
          */
	quint32 _uiClientId;

        /*!
          * uiclientid as a key and corresponding msg thread
          */
	QMap<quint32, UiMsgThread *> _uiMsgThreadsMap;

//	QMap<quint32, FileReceivingTcpServer *> uiFileRecvRunnableMap;

        /*!
          * uiclientid and corresponding shared pointer
          */
	QMap<quint32, SN_PolygonArrowPointer *> _pointers;

        /*!
          uiclientid and corresponding app layout flag
          appLayout is sent to those clients who set the flag
          */
//        QMap<quint64, bool> appLayoutFlagMap;


//	QGraphicsScene *scene;

        /**
          * This buffer is updated periodically by this->updateAppLayout() and broadcasted to ALL ext UI clients (RESPOND_APP_LAYOUT)
          */
	QByteArray *appLayout;

	SN_TheScene *_scene;

	SN_Launcher *_launcher;

	UiMsgThread * getUiMsgThread(quint32 uiclientid);

	SN_PolygonArrowPointer * getSharedPointer(quint32 uiclientid);

        /**
          When a user drag and drop media file on the sagenextpointer, the launcher receives the file and fire corresponding widget.
          The file is stored at $HOME/.sagenext/<image|video>
          This function runs in separate thread.
          */
//        void fileReceivingFunction(int mtype, const QString &fname, qint64 fsize, int port);

signals:
	void ratkoDataFinished();

        /**
          * REG_FROM_UI handler.
          * will invoke GraphicsViewMain::startApp()
          */
//        void registerApp(MEDIA_TYPE type, QString filename, qint64 filesize, QString senderIP, QString recvIP, quint16 recvPort);

private slots:
	void handleMessage(const QByteArray msg);

        /**
          * invoked by the signal UiMsgThread::clientDisconnected()
          * update uiThreadsMap and remove PolygonArrow from the scene
          */
	void removeFinishedThread(quint32);

public slots:
    int sendMsgToUiClient(quint32 uiclientid, const QByteArray &msg);
};

#endif // UISERVER_H
