#include "uiserver.h"
#include "../sagenextscene.h"
#include "../sagenextlauncher.h"
#include "../applications/base/basewidget.h"
#include "../common/sn_sharedpointer.h"

#include "../applications/base/sn_priority.h"
#include "../applications/base/appinfo.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSettings>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

SN_UiServer::SN_UiServer(const QSettings *s, SN_Launcher *snl, SN_TheScene *sns)
    : QTcpServer(0)
    , _settings(s)
    , _fileRecvPortBase(0)
    , _uiClientId(0)
    , _scene(sns)
    , _launcher(snl)
{
    _uiMsgThreadsMap.clear();
//    uiFileRecvRunnableMap.clear();

    if ( ! listen(QHostAddress::Any, _settings->value("general/wallport", 30003).toInt() ) ) {
        qCritical("UiServer::%s() : listen failed", __FUNCTION__);
        deleteLater();
    }

    appLayout = new QByteArray(EXTUI_MSG_SIZE, '\0');

    _fileRecvPortBase = 10000 + s->value("general/wallport", 30003).toInt();

    qWarning() << "UI Server has started." << serverAddress() << serverPort();

}

SN_UiServer::~SN_UiServer() {
    close();

	foreach (SN_PolygonArrowPointer *pa, _pointers) {
		delete pa;
	}
	
	foreach (UiMsgThread *thr, _uiMsgThreadsMap) {
		thr->terminate();
		thr->endThread();
	}

    //	QByteArray msg(EXTUI_MSG_SIZE, 0);
    //	sprintf(msg.data(), "%d dummy", WALL_IS_CLOSING);
    //	foreach (UiMsgThread *msgthr, uiThreadsMap) {

    //		if ( ! QMetaObject::invokeMethod(msgthr, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg)) ) {
    //			qDebug("UiServer::%s() : failed to send WALL_IS_CLOSING",__FUNCTION__);
    //		}
    //	}

    qDebug("SN_UiServer::%s()", __FUNCTION__);
}

void SN_UiServer::incomingConnection(qintptr sockfd) {

    ++_uiClientId;

    //	QGraphicsScene *scene = gview->scene(); // Returns a pointer to the scene that is currently visualized in the view. If no scene is currently visualized, 0 is returned.

    /*
     send uiclientid and scene size
     */
	/*
    char buf[EXTUI_MSG_SIZE]; memset(buf, '\0', EXTUI_MSG_SIZE);
//    int filetransferport = fileRecvPortBase + uiClientId;
	int filetransferport = _settings->value("general/fileserverport", 46000).toInt();
    sprintf(buf, "%u %d %d %d", _uiClientId, (int)_scene->width(), (int)_scene->height(), filetransferport);
    ::send(sockfd, buf, 1280, 0);
//    qDebug("UiServer::%s() : The scene size %d x %d sent to ui %u", __FUNCTION__, (int)_scene->width(), (int)_scene->height(), _uiClientId);
*/

    /**
      create a msg thread
      */
    UiMsgThread *thread = new UiMsgThread(_uiClientId, sockfd, this);
    _uiMsgThreadsMap.insert(_uiClientId, thread);

	QByteArray initMsg(EXTUI_MSG_SIZE, 0);
	int filetransferport = _settings->value("general/fileserverport", 46000).toInt();
    sprintf(initMsg.data(), "%d %u %d %d %d", SAGENext::ACK_FROM_WALL, _uiClientId, (int)_scene->width(), (int)_scene->height(), filetransferport);

	//
	// Note that the UiMsgThread::sendMsg() will run in this thread (which is main GUI thread)
	//
	thread->sendMsg(initMsg);

	//
	// This will run the function in a separate thread. I didn't test this !
	//
//	QtConcurrent::run(thread, &UiMsgThread::sendMsg, initMsg);


	connect(this, SIGNAL(destroyed()), thread, SLOT(endThread()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(clientDisconnected(quint32)), this, SLOT(removeFinishedThread(quint32)));

	if (! connect(thread, SIGNAL(msgReceived(QByteArray)), this, SLOT(handleMessage(const QByteArray))) ) {
		qCritical("%s::%s() : connecting UiMsgThread::msgReceived() to UiServer::handleMessage() failed", metaObject()->className(), __FUNCTION__);
	}

    thread->start();

    /**
          create a file recv thread
          */
    //        FileReceivingTcpServer *recvThread = new FileReceivingTcpServer(launcher, filetransferport);
    //        recvThread->setAutoDelete(false);
    //        uiFileRecvRunnableMap.insert(uiClientId, recvThread);

    // by default, ui client is not receiving app layout on the wall
//    appLayoutFlagMap.insert(uiClientId, false);

    qDebug("SN_UiServer::%s() : The ui client %u (%s) has connected to UiServer", __FUNCTION__, _uiClientId, qPrintable(thread->peerAddress().toString()));
}

int SN_UiServer::sendMsgToUiClient(quint32 uiclientid, const QByteArray &msg) {
    UiMsgThread *msgThread = getUiMsgThread(uiclientid);
    if (!msgThread) {
        qDebug() << "SN_UiServer::sendMessageToUiClient() : there's no msgThread for uiclient" << uiclientid;
        return -1;
    }

    /*
    if ( ! QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg)) ) {
        qDebug() << "SN_UiServer::sendMessageToUiClient() : failed to invoke sendMsg()";
        return -1;
    }
    */

    msgThread->sendMsg(msg);

    return 0;
}

void SN_UiServer::handleMessage(const QByteArray msg) {

	UiMsgThread *msgThread = static_cast<UiMsgThread *>(sender());

    // "msgcode data"
    int code = 0;
    //char data[512];
    sscanf(msg.constData(), "%d", &code);

    switch(code) {
    case SAGENext::MSG_NULL: {
        break;
    }

	case SAGENext::WIDGET_CHANGE: {
		quint64 gaid = 0;

		int sx, sy;
		int rx, ry, rw, rh;
		int z;

		::sscanf(msg.constData(), "%d %llu %d %d %d %d %d %d %d", &code, &gaid, &sx, &sy, &rx, &ry, &rw, &rh, &z);
		QSize nativesize(sx, sy);
		QRect winrect(rx, ry, rw, rh);

		qDebug() << "CHANGE" << gaid << nativesize << winrect;

		SN_BaseWidget *bw = _scene->getUserWidget(gaid);
		if (bw) {
			//
			// below assumes single SN_LayoutWidget covering entire scene
			//
			if ( bw->pos() != QPointF(rx,ry) ) {
//				qDebug() << "\tMOVED to" << rx << ry;
				bw->setPos(rx, ry);
//			bw->setPos(  bw->parentItem()->mapFromScene(rx,ry)  );
				bw->priorityData()->setLastInteraction(SN_Priority::MOVE);
			}

			qreal newscale = (qreal)rw / (qreal)sx;
			if (bw->scale() != newscale) {
//				qDebug() << "\tRESCALED" << newscale;
				bw->setScale(newscale);
				bw->priorityData()->setLastInteraction(SN_Priority::RESIZE);
			}
		}
		else {
			qDebug() << "Couldn't find the app" << gaid << "\n";
		}

		break;
	}
	case SAGENext::WIDGET_REMOVE: {
		quint64 gaid = 0;
		::sscanf(msg.constData(), "%d %llu", &code, &gaid);
		qDebug() << "REMOVE" << gaid;
		SN_BaseWidget *bw = _scene->getUserWidget(gaid);
		if (bw) {
			bw->close();
		}
		else {
			qDebug() << "Couldn't find the app" << gaid << "\n";
		}
		break;
	}
	case SAGENext::WIDGET_Z: {
		quint64 gaid = 0;
		qreal z = 0.0;
		::sscanf(msg.constData(), "%d %llu %lf", &code, &gaid, &z);
//		qDebug() << "Z" << gaid << z;
		SN_BaseWidget *bw = _scene->getUserWidget(gaid);
		if (bw) {
			if (z > bw->zValue()) {
//				qDebug() << "\tZ UP" << z;
				bw->priorityData()->setLastInteraction(SN_Priority::CLICK);
			}
			bw->setZValue(z);

		}
		else {
			qDebug() << "Couldn't find the app" << gaid << "\n";
		}
		break;
	}
	case SAGENext::WIDGET_CLOSEALL: {
//		::sscanf(msg.constData(), "%d", &code);
		qDebug() << "WIDGET_CLOSEALL";

		_scene->closeAllUserApp();

		emit ratkoDataFinished();

		break;
	}

		/*************
    case REG_FROM_UI: {
        qDebug("UiServer::%s() : received REG_FROM_UI [%s]", __FUNCTION__, msg.constData());

        // read message
        char senderIP[INET_ADDRSTRLEN];
        char filename[256];
        qint64 filesize;
        int mode, resx, resy, viewx, viewy, posx, posy;
        int mediatype;
        sscanf(msg.constData(), "%d %s %s %d %lld %d %d %d %d %d %d %d", &code, senderIP, filename, &mediatype, &filesize, &mode, &resx, &resy, &viewx, &viewy, &posx, &posy);


        // determine which receiver IP:port is going to be used to receive
        QString receiverIP( settings->value("general/wallip").toString() );
        int receiverPort = fileRecvPortBase + id;
        if ( receiverPort >= 65535 ) {
            qCritical("UiServer::%s() : file recv port is %d", __FUNCTION__, receiverPort);
            break;
        }

        QtConcurrent::run(this, &UiServer::fileReceivingFunction, mediatype, QString(filename), filesize, receiverPort);

        //
         //send ACK_FROM_WALL through msg channel
         //This will trigger uiclient to transfer the file
        //
        buffer.fill('\0');
        sprintf(buffer.data(), "%d %s %d", ACK_FROM_WALL, qPrintable(receiverIP), receiverPort);

        //	int ack = ::send(it.value(), buffer.data(), buffer.size(), 0);
        //	QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, buffer));
        msgThread->sendMsg(buffer);

        //	qDebug("UiServer::%s() : ACK_FROM_WALL [%s] sent to sock %d", __FUNCTION__, buffer.data(), it.value());

        break;
    }
    case TOGGLE_APP_LAYOUT: {
        quint64 uid;
        sscanf(msg.constData(), "%d %u", &code, &uid);

        QMap<quint64,bool>::iterator it = appLayoutFlagMap.find(uid);
        if ( it != appLayoutFlagMap.end() ) {

            // toggle the flag
            appLayoutFlagMap.insert(uid, ! it.value());
        }

		//updateAppLayout();

//		if ( appLayout->size() > EXTUI_MSG_SIZE ) {
//			qCritical("%s::%s() : REQUEST_APP_LAYOUT received. Couldn't send msg because the appLayout size is %d", metaObject()->className(), __FUNCTION__, appLayout->size());
//		}
//		else {
//			QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, *appLayout));
//		}
        break;
    }
	***************/

	case SAGENext::RESPOND_STRING: {
		//
		// uiclientid can be found using this. So uiclient doesn't have to send its id
		//
//		msgThread->getID();

//		QString str;

//		QByteArray text(EXTUI_SMALL_MSG_SIZE, 0);
//		sscanf(msg.constData(), "%d %s", &code, text.data()); // no white space support
//		str = QString(text);


//		QTextStream ts(msg);
//		ts >> code;
//		str = ts.readAll();
//		str = str.trimmed();

//		qDebug() << str;

		QString str(msg);

		int firstspace = str.indexOf(QChar(' '));
		str.remove(0, firstspace + 1); // remove messageCode followed by a space

		Q_ASSERT(msgThread);
		// find the PolygonArrow associated with this uiClientId
        SN_PolygonArrowPointer *pa = getSharedPointer(msgThread->getID());

		if (pa) {
//			qDebug() << "RESPOND_STRING" << str;
			pa->injectStringToItem(str.trimmed());
		}

		break;
	}

	case SAGENext::REQUEST_FILEINFO: {
		quint64 gaid = 0; // globalAppId of the application that uiclient wants to know

		// the message must be cleared with 0s in uiclient before sending
		sscanf(msg.constData(), "%d %llu", &code, &gaid);

		QString filepath;
		qint64 filesize = 0;
		SAGENext::MEDIA_TYPE mtype;

		SN_BaseWidget *bw =  0;
		if (gaid > 0) {
			bw = _scene->getUserWidget(gaid);
		}
		else {
			Q_ASSERT(msgThread);
			SN_PolygonArrowPointer *pa = getSharedPointer(msgThread->getID());
			Q_ASSERT(pa);

			//
			// a user must click the app before requesting this !!
			//
			bw = pa->appUnderPointer();
		}


		if(bw) {
			Q_ASSERT(bw->appInfo());
			mtype = bw->appInfo()->mediaType();

			QByteArray msg(EXTUI_MSG_SIZE, '\0');


			if (bw->appInfo()->fileInfo().isFile()) {
				filesize = bw->appInfo()->fileInfo().size();
				filepath = bw->appInfo()->mediaFilename(); // absolute filepath

				//
				// send only filename to the client
				//
				sprintf(msg.data(), "%d %d %s %lld", SAGENext::RESPOND_FILEINFO, mtype, qPrintable(filepath), filesize);

//				msgThread->sendMsg();
				if ( ! QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg)) ) {
					qDebug() << "UiServer: REQUEST_FILEINFO: failed to invoke UiMsgThread::sendMsg()";
				}
			}
			else {
				qDebug() << "UiServer: REQUEST_FILEINFO: media isn't a file";
			}
		}
		else {
			if (gaid > 0) qDebug() << "UiServer: REQUEST_FILEINFO: There's no such app with id" << gaid;
			else qDebug() << "UiServer: REQUEST_FILEINFO: User didn't set an application under his pointer";
		}

		break;
	}

    case SAGENext::VNC_SHARING: {
//        char senderIP[128];
        int display = 0;
        int framerate = 24;
        char vncpass[64];
		char username[64];
        sscanf(msg.constData(), "%d %d %s %s %d", &code, &display, username, vncpass, &framerate);

//		qDebug() << "uiserver" << QString(username) << QString(vncpass);

        QString vncusername = QString(username);
        if (vncusername == "_") {
            vncusername.clear();
        }
        QString vncpassword = QString(vncpass);
        if (vncpassword == "_") {
            vncpassword.clear();
        }

        // This isn't elegant way of invoking vnc widget. :(
        QMetaObject::invokeMethod(_launcher, "launch", Qt::QueuedConnection
                                  , Q_ARG(QString, vncusername)
                                  , Q_ARG(QString, vncpassword)
                                  , Q_ARG(int, display)
                                  , Q_ARG(QString, msgThread->peerAddress().toString())
                                  , Q_ARG(int, framerate)
                                  );
        //		gviewMain->startApp(MEDIA_TYPE_VNC, QString(vncpass), display, QString(senderIP), "", framerate);
        break;
    }


        /** shared pointer **/
    case SAGENext::POINTER_SHARE: {
		char colorname[16];
        quint32 uiclientid;
        QByteArray pname(64, '\0');
        sscanf(msg.constData(), "%d %u %s %s", &code, &uiclientid, pname.data(), colorname);

//        qDebug("UiServer::%s() : POINTER_SHARE from uiclient %u, (%s, %s)",__FUNCTION__, uiclientid, pname.constData(), colorname);

        SN_PolygonArrowPointer *pointerItem = getSharedPointer(uiclientid);
		if (pointerItem) {
			pointerItem->setEnabled(true);
			pointerItem->show();
		}
		else {
			pointerItem = _launcher->launchPointer(uiclientid, msgThread, QString(pname), QColor(QString(colorname)));
			Q_ASSERT(pointerItem);
			_pointers.insert(uiclientid, pointerItem);
		}
        break;
    }
    case SAGENext::POINTER_UNSHARE: {
        quint32 uiclientid;
        sscanf(msg.constData(), "%d %u", &code, &uiclientid);
        // find the PixmapArrow associated with this uiClientId
        QGraphicsItem *pa = getSharedPointer(uiclientid);
        if (pa) {
			/**
            delete pa;
            _pointers.remove(uiclientid);
			**/
			pa->hide();
			pa->setEnabled(false);
        }
        else {
            qDebug("UiServer::%s() : POINTER_UNSHARE can't find pointer object for the uiclient %u", __FUNCTION__, uiclientid);
        }
        break;
    }

    /*
      Nothing special happens. just the Pointer item is going to move on the scene
     */
    case SAGENext::POINTER_MOVING: {
        quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);

        // find the PolygonArrow associated with this uiClientId
        SN_PolygonArrowPointer *pa = getSharedPointer(uiclientid);
        if (pa) {
            //pa->setPos(x,y);
            pa->pointerMove(QPointF(x,y), Qt::NoButton); // just move pointer graphics item
        }
        else {
            qDebug("UiServer::%s() : POINTER_MOVING can't find pointer object", __FUNCTION__);
        }
        break;
    }

    /*
      An external ui client's mouseMoveEvent plus left button modifier triggers this
      This is always followed by POINTER_PRESS (left button)
    */
//	case POINTER_RIGHTDRAGGING:
    case SAGENext::POINTER_DRAGGING:
	{
        quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);

        SN_PolygonArrowPointer *pa = getSharedPointer(uiclientid);
        if (pa) {
            // direct widget manipulation instead of posting mouse event
            pa->pointerMove(QPointF(x,y), Qt::LeftButton);
        }
        else {
            qDebug("UiServer::%s() : POINTER_DRAGGING can't find pointer object", __FUNCTION__);
        }

        break;
	}
	case SAGENext::POINTER_RIGHTDRAGGING: {
		quint32 uiclientid;
		int x,y;
		sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);

		SN_PolygonArrowPointer *pa = getSharedPointer(uiclientid);
		if (pa) {
			// direct widget manipulation instead of posting mouse event
			pa->pointerMove(QPointF(x,y), Qt::RightButton);
		}
		else {
			qDebug("UiServer::%s() : POINTER_RIGHTDRAGGING can't find pointer object", __FUNCTION__);
		}

		break;
	}


   /*
    Pointer will set the widget if there is one (QGraphicsItem::UserType + 2) under it
    */
    case SAGENext::POINTER_PRESS:
	{
        quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid) ;
        if (pa) {
//            qDebug("UiServer::%s() : POINTER_PRESS : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());

            // Below will call setAppUnderPointer()
            // followed by setTopmost()
            pa->pointerPress(QPointF(x,y), Qt::LeftButton);
            //pa->setAppUnderPointer(QPointF(x,y));
        }
        else {
            qDebug("%s::%s() : POINTER_PRESS can't find pointer object", metaObject()->className(), __FUNCTION__);
        }
        break;
    }

		/*
	     Pointer will set the widget if there is one (QGraphicsItem::UserType + 2) under it
	     */
	case SAGENext::POINTER_RIGHTPRESS:
	{
		quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid) ;
        if (pa) {
            pa->pointerPress(QPointF(x,y), Qt::RightButton);
        }
        else {
            qDebug("%s::%s() : POINTER_RIGHTPRESS can't find pointer object",metaObject()->className(), __FUNCTION__);
        }
		break;
	}

		/*
		  This doesn't trigger pointer click. Don't use this to fire pointerClick()
		  This event is sent from uiclient when mouse left draggin has finished.
		  */
	case SAGENext::POINTER_RELEASE:
	{
		quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid);
		if (pa) {
			pa->pointerRelease(QPointF(x,y), Qt::LeftButton);
		}
		break;
	}

		/*
		  This doesn't trigger pointer click. Don't use this to fire pointerClick()
		  This event is sent from uiclient when mouse right draggin has finished.
		  */
	case SAGENext::POINTER_RIGHTRELEASE:
	{
		quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid);
		if (pa) {
			pa->pointerRelease(QPointF(x,y), Qt::RightButton);
		}
		break;
	}

    case SAGENext::POINTER_CLICK:
	{
        quint32 uiclientid;
        int x,y; // x,y from ui clients is the scene position of the wall
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid) ;
        if (pa) {
//            qDebug("UiServer::%s() : POINTER_CLICK : pointer clicked position (%.0f, %.0f)", __FUNCTION__, pa->x(), pa->y());
            pa->pointerClick(QPointF(x,y), Qt::LeftButton);

			// Let each application provides mouseClick()
			 // Widget under the pointer can reimplement BaseWidget::mouseClick()
//			if ( pa->appUnderPointer() ) pa->appUnderPointer()->mouseClick(QPointF(x, y), Qt::LeftButton);
        }
        else {
            qDebug("%s::%s() : POINTER_CLICK can't find pointer object",metaObject()->className(), __FUNCTION__);
        }
        break;
    }

		/**
		  contextMenuEvent -> POINTER_RIGHTCLICK
		  */
	case SAGENext::POINTER_RIGHTCLICK:
	{
        quint32 uiclientid;
        int x, y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid) ;
        if (pa) {
            pa->pointerClick(QPointF(x,y), Qt::RightButton);
        }
        else {
            qDebug("%s::%s() : POINTER_RIGHTCLICK can't find pointer object",metaObject()->className(), __FUNCTION__);
        }
        break;
    }


    /**
	  Always left button
	  */
    case SAGENext::POINTER_DOUBLECLICK: {
        quint32 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %u %d %d", &code, &uiclientid, &x, &y);
        SN_PolygonArrowPointer *pa =  getSharedPointer(uiclientid) ;
        if (pa) {
            pa->pointerDoubleClick(QPointF(x, y), Qt::LeftButton);
        }
		else {
            qDebug("%s::%s() : POINTER_DOUBLECLICK  can't find pointer object",metaObject()->className(), __FUNCTION__);
        }
        break;
    }


    case SAGENext::POINTER_WHEEL: {
        quint32 uiclientid;
        int x,y,tick;
        sscanf(msg.constData(), "%d %u %d %d %d", &code, &uiclientid, &x, &y, &tick);
        SN_PolygonArrowPointer *pa = getSharedPointer(uiclientid);
        if (pa) {
            //qDebug() << "UiServer:: wheel" << x << y << 120*tick;
            pa->pointerWheel(QPointF(x, y),  tick);
        }
		else {
            qDebug("%s::%s() : POINTER_WHEEL  can't find pointer object",metaObject()->className(), __FUNCTION__);
        }
        break;
    }

    default: {
        qDebug() << "SN_UiServer::handleMessage() : couldn't understand the message" << msg;
        break;
    }

    } /* end of switch */
}

void SN_UiServer::removeFinishedThread(quint32 uiclientid) {
    int removed = _uiMsgThreadsMap.remove(uiclientid);
    if ( removed > 1 ) {
        qDebug("UiServer::%s() : %d entries deleted from uiThreadsMap. There were duplicate uiClientId", __FUNCTION__, removed);
    }
    else if ( removed == 0 ) {
        qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
    }

//    removed = uiFileRecvRunnableMap.remove(uiclientid);
//    if ( removed > 1 ) {
//        qDebug("UiServer::%s() : %d entries deleted from uiFileRecvThreadsMap. There were duplicate uiClientId", __FUNCTION__, removed);
//    }
//    else if ( removed == 0 ) {
//        qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
//    }

	/*
    removed = appLayoutFlagMap.remove(uiclientid);
    if ( removed > 1 ) {
        qDebug("UiServer::%s() : %d entries deleted from appLayoutMap. There were duplicate uiClientId", __FUNCTION__, removed);
    }
    else if ( removed == 0 ) {
        qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
    }
	*/

    /* delete shared pointer */
    QGraphicsItem *pa = getSharedPointer(uiclientid);
    if (pa) {
        if (_scene) _scene->removeItem(pa);
        delete pa;

        removed = _pointers.remove(uiclientid);
        if ( removed > 1 ) {
            qDebug("UiServer::%s() : %d entries deleted. There were duplicate uiClientId", __FUNCTION__, removed);
        }
        else if ( removed == 0 ) {
            qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
        }
    }
}

/*!
  This needs to be called periodically. Currently it's not used.
  */
/**
void UiServer::updateAppLayout() {
    if (!scene) return;
    QList<QGraphicsItem *> allitems = scene->items();
    if (allitems.size() == 0 ) return;

    QList<QGraphicsItem *>::const_iterator it = allitems.constBegin();
    BaseWidget *bgw = 0;

    int x, y, width, height;
    qreal z;

    unsigned int count = 0;
    QByteArray ba;
    for (; it!=allitems.constEnd(); it++ ) {

        if ( !(*it) ) return;

        // only count app window
        if ( (*it)->type() != QGraphicsItem::UserType + 2 ) continue;

        bgw = static_cast<BaseWidget *>(*it);
        Q_ASSERT(bgw);
        QRect rect = bgw->sceneBoundingRect().toRect();
        rect.getRect(&x, &y, &width, &height);
        z = bgw->zValue();

        // <data> is gappid, x, y, width, height
        ba.append( QByteArray::number(bgw->globalAppId()).append(" ") );
        ba.append( QByteArray::number(x).append(" ") );
        ba.append( QByteArray::number(y).append(" ") );
        ba.append( QByteArray::number(z).append(" ") );
        ba.append( QByteArray::number(width).append(" ") );
        ba.append( QByteArray::number(height).append(" ") );

        ++count;
    }

    // RESPOND_APP_LAYOUT, # apps, <data> ...
    QByteArray prefix(QByteArray::number(RESPOND_APP_LAYOUT).append(" "));
    prefix.append(QByteArray::number(count).append(" "));
    ba.prepend(prefix);

    //	qDebug() << ba.size() << ", " << ba.constData();
    if ( ba.size() > appLayout->size() ) {
        qCritical("UiServer::%s() : RESPOND_APP_LAYOUT msg size %d", __FUNCTION__, ba.size());
        return;
    }

    // update appLayout with most recent data
    appLayout->replace(0, ba.size(), ba);


    // for each UiMsgThread
    QMap<quint32, UiMsgThread*>::const_iterator iter = uiMsgThreadsMap.constBegin();
    UiMsgThread *msgthr = 0;

    QMap<quint64, bool>::const_iterator flagiter;

    // only send appLayout info to a uiclient who chose to receive
    for (; iter!=uiMsgThreadsMap.constEnd(); iter++) {
        if ( ! iter.value() ) continue;

        flagiter = appLayoutFlagMap.find( iter.key() );
        if ( flagiter == appLayoutFlagMap.end()  ||  flagiter.value() == false ) continue;

        msgthr = iter.value();
        Q_ASSERT(msgthr);

        if ( ! QMetaObject::invokeMethod(msgthr, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, *appLayout)) ) {
            qDebug("UiServer::%s() : invokeMethod on UiMsgThread::sendMsg() failed", __FUNCTION__);
        }
    }
}
**/


UiMsgThread * SN_UiServer::getUiMsgThread(quint32 uiclientid) {
    QMap<quint32, UiMsgThread*>::iterator it = _uiMsgThreadsMap.find(uiclientid);
    if ( it == _uiMsgThreadsMap.end()) return 0;
    UiMsgThread *ret = it.value();
    return ret;
}

SN_PolygonArrowPointer * SN_UiServer::getSharedPointer(quint32 uiclientid) {
    QMap<quint32, SN_PolygonArrowPointer *>::iterator it = _pointers.find(uiclientid);
    if ( it == _pointers.end() ) return 0;
    return it.value();
}

/***
void UiServer::fileReceivingFunction(int mtype, const QString &fname, qint64 fsize, int port) {
    int serverSock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == -1) {
        qCritical("%s::%s() : can't creat socket", metaObject()->className(), __FUNCTION__);
        perror("socket");
        return;
    }

    sockaddr_in myAddr;
    memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(port);
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //    inet_pton(AF_INET, qPrintable(recvAddr.toString()), &myAddr.sin_addr.s_addr);

    if ( ::bind(serverSock, (struct sockaddr *)&myAddr, sizeof(myAddr)) != 0 ) {
        qCritical("%s::%s() : bind error", metaObject()->className(), __FUNCTION__);
        perror("bind");
        return;
    }
    if ( ::listen(serverSock, 10) != 0 ) {
        qCritical("%s::%s() : listen error", metaObject()->className(), __FUNCTION__);
        perror("listen");
        return;
    }

//    qDebug() << "UiServer::fileReceivingFunction thread listening on port" << port;

    int socket = ::accept(serverSock, 0, 0);

    if ( socket < 0 ) {
        qCritical("%s::%s() : accept error", metaObject()->className(), __FUNCTION__);
        perror("accept");
        return;
    }
//    qDebug("%s::%s() : file sender connected. socket %d",metaObject()->className(), __FUNCTION__, socket);


//    int optval = 1;
//    //socket.setSocketOption(QAbstractSocket::LowDelayOption, (const QVariant *)&optval);
//    if ( setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
//            qWarning("%s() : setsockopt SO_REUSEADDR failed", __FUNCTION__);
//    }
//    if ( setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &optval, (socklen_t)sizeof(optval)) != 0 ) {
//            qWarning("%s() : setsockopt TCP_NODELAY failed", __FUNCTION__);
//    }


    QByteArray buffer(fsize, 0);
    if ( ::recv(socket, buffer.data(), fsize, MSG_WAITALL) < 0 ) {
        qCritical("%s::%s() : error while receiving.", metaObject()->className(), __FUNCTION__);
        return;
    }


    QString savePath = QDir::homePath().append("/.sagenext/media/");
    switch(mtype) {
    case MEDIA_TYPE_IMAGE : {
        savePath.append("image/");
        break;
    }
    case MEDIA_TYPE_LOCAL_VIDEO : {
        savePath.append("video/");
        break;
    }
    }

    QFile file( savePath.append(fname) );
    file.open(QIODevice::WriteOnly);
    file.write(buffer);
    file.close();


    qDebug() << "UiServer::fileReceivingFunction thread is invoking launch() with media " << mtype << file.fileName();
//    launcher->launch(mtype, file.fileName());
    QMetaObject::invokeMethod(launcher, "launch", Qt::QueuedConnection, Q_ARG(int, mtype), Q_ARG(QString, file.fileName()));



	// Let the client close the connection
    sleep(1);
//    ::shutdown(socket, SHUT_RDWR);
//    ::close(socket);
//    ::close(serverSock);
}
***/
