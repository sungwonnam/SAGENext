#include "uiserver.h"
//#include "../applications/base/basegraphicswidget.h"
#include "../sagenextscene.h"
#include "../sagenextlauncher.h"
#include "../applications/base/basewidget.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSettings>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

UiServer::UiServer( const QSettings *s, SAGENextLauncher *snl, SAGENextScene *sns) :
	QTcpServer(0),
	settings(s),
	scene(sns),
	launcher(snl)
{
	uiThreadsMap.clear();
	uiClientId = 0;

	if ( ! listen(QHostAddress::Any, settings->value("general/wallport", 30003).toInt() ) ) {
		qCritical("UiServer::%s() : listen failed", __FUNCTION__);
		deleteLater();
	}

	appLayout = new QByteArray(EXTUI_MSG_SIZE, '\0');

	qDebug("UiServer::%s() : UI Server has started on %s:%u", __FUNCTION__, qPrintable(serverAddress().toString()), serverPort());
}

UiServer::~UiServer() {
	close();

//	QByteArray msg(EXTUI_MSG_SIZE, 0);
//	sprintf(msg.data(), "%d dummy", WALL_IS_CLOSING);
//	foreach (UiMsgThread *msgthr, uiThreadsMap) {

//		if ( ! QMetaObject::invokeMethod(msgthr, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, msg)) ) {
//			qDebug("UiServer::%s() : failed to send WALL_IS_CLOSING",__FUNCTION__);
//		}
//	}

	qDebug("UiServer::%s()", __FUNCTION__);
}

void UiServer::incomingConnection(int sockfd) {
	++uiClientId;

//	QGraphicsScene *scene = gview->scene(); // Returns a pointer to the scene that is currently visualized in the view. If no scene is currently visualized, 0 is returned.

	/* send uiclientid and scene size */
	char buf[EXTUI_MSG_SIZE]; memset(buf, '\0', EXTUI_MSG_SIZE);
	sprintf(buf, "%llu %d %d", uiClientId, (int)scene->width(), (int)scene->height());
	::send(sockfd, buf, 1280, 0);
	qDebug("UiServer::%s() : The scene size %d x %d sent to ui %llu", __FUNCTION__, (int)scene->width(), (int)scene->height(), uiClientId);


	UiMsgThread *thread = new UiMsgThread(uiClientId, sockfd, this);

	connect(this, SIGNAL(destroyed()), thread, SLOT(breakWhileLoop()));
	connect(thread,SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(thread, SIGNAL(clientDisconnected(quint64)), this, SLOT(removeFinishedThread(quint64)));
	connect(thread,SIGNAL(msgReceived(quint64, UiMsgThread *, QByteArray)), this, SLOT(handleMessage(const quint64, UiMsgThread *, const QByteArray)));

	// by default, ui client is not receiving app layout on the wall
	appLayoutFlagMap.insert(uiClientId, false);

	uiThreadsMap.insert(uiClientId, thread);
	thread->start();

	qDebug("UiServer::%s() : ui client %llu has connected.", __FUNCTION__, uiClientId);
}

void UiServer::handleMessage(const quint64 id, UiMsgThread *msgThread, const QByteArray msg) {
//	if (!msg) return;
	QByteArray buffer(EXTUI_MSG_SIZE, '\0');

	// "msgcode data"
	int code = 0;
	//char data[512];
	sscanf(msg.constData(), "%d", &code);

	switch(code) {
	case MSG_NULL: {
			break;
		}
	case REG_FROM_UI: {
			qDebug("UiServer::%s() : received REG_FROM_UI [%s]", __FUNCTION__, msg.constData());

			/* read message */
			char senderIP[INET_ADDRSTRLEN];
			char filename[256];
			qint64 filesize;
			int mode, resx, resy, viewx, viewy, posx, posy;
			int mediatype;
			sscanf(msg.constData(), "%d %s %s %d %lld %d %d %d %d %d %d %d", &code, senderIP, filename, &mediatype, &filesize, &mode, &resx, &resy, &viewx, &viewy, &posx, &posy);


			/* determine which receiver IP:port is going to be used to receive */
			QString receiverIP( settings->value("general/wallip").toString() );
			int receiverPort = settings->value("general/wallport", 30003).toInt();
			receiverPort += id;
			if ( receiverPort >= 65535 ) {
				qCritical("UiServer::%s() : receiverPort set to %d", __FUNCTION__, receiverPort);
				break;
			}


			/* SIGNAL MainWindow. This launches widget */
//			emit registerApp((MEDIA_TYPE)mediatype, QString(filename), filesize, QString(senderIP), receiverIP, receiverPort);
			if (!launcher) {
				qCritical("%s::%s() : SAGENextLauncher is not valid. ", metaObject()->className(), __FUNCTION__);
			}
			else {
				QMetaObject::invokeMethod(launcher, "launch", Qt::QueuedConnection,
										  Q_ARG(int, mediatype),
										  Q_ARG(QString, QString(filename)),
										  Q_ARG(qint64, filesize),
										  Q_ARG(QString, QString(senderIP)),
										  Q_ARG(QString, QString(receiverIP)),
										  Q_ARG(quint16, receiverPort));
			}


			/* send ACK_FROM_WALL through msg channel */
			buffer.fill('\0');
			sprintf(buffer.data(), "%d %s %d", ACK_FROM_WALL, qPrintable(receiverIP), receiverPort);

//			int ack = ::send(it.value(), buffer.data(), buffer.size(), 0);
//			QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, buffer));
			msgThread->sendMsg(buffer);

//			qDebug("UiServer::%s() : ACK_FROM_WALL [%s] sent to sock %d", __FUNCTION__, buffer.data(), it.value());

			break;
		}
	case TOGGLE_APP_LAYOUT: {
			quint64 uid;
			sscanf(msg.constData(), "%d %llu", &code, &uid);

			QMap<quint64,bool>::iterator it = appLayoutFlagMap.find(uid);
			if ( it != appLayoutFlagMap.end() ) {

				// toggle the flag
				appLayoutFlagMap.insert(uid, ! it.value());
			}

//			updateAppLayout();

			/*
			if ( appLayout->size() > EXTUI_MSG_SIZE ) {
				qCritical("%s::%s() : REQUEST_APP_LAYOUT received. Couldn't send msg because the appLayout size is %d", metaObject()->className(), __FUNCTION__, appLayout->size());
			}
			else {
				QMetaObject::invokeMethod(msgThread, "sendMsg", Qt::QueuedConnection, Q_ARG(QByteArray, *appLayout));
			}
			*/
			break;
		}

	case VNC_SHARING: {
		quint64 uiclientid;
		char senderIP[128];
		int display = 0;
		int framerate = 24;
		char vncpass[64];
		sscanf(msg.constData(), "%d %llu %s %d %s %d", &code, &uiclientid, senderIP, &display, vncpass, &framerate);
		// void startApp(MEDIA_TYPE type, QString filename, qint64 filesize=0, QString senderIP="127.0.0.1", QString recvIP="", quint16 recvPort=0);

		// This isn't elegant way of invoking vnc widget. :(
		QMetaObject::invokeMethod(launcher, "launch", Qt::QueuedConnection,
								  Q_ARG(QString, QString(vncpass)),
								  Q_ARG(int, display),
								  Q_ARG(QString, QString(senderIP)),
								  Q_ARG(int, framerate));
//		gviewMain->startApp(MEDIA_TYPE_VNC, QString(vncpass), display, QString(senderIP), "", framerate);
		break;
	}


		/** shared pointer **/
	case POINTER_SHARE: {
			int r,g,b;
			quint64 uiclientid;
			QByteArray pname(64, '\0');
			sscanf(msg.constData(), "%d %llu %s %d %d %d", &code, &uiclientid, pname.data(), &r, &g, &b);

//			qDebug("UiServer::%s() : POINTER_SHARE from uiclient %llu, (%s, %d %d %d)",__FUNCTION__, uiclientid, pname.constData(), r,g,b);


			/*
			PolygonArrow *pa = new PolygonArrow(uiclientid, settings, c);

			if ( !pointername.isNull() && !pointername.isEmpty())
				pa->setPointerName(pointername);

			gscene->addItem(pa);
			*/

//			QGraphicsItem *pointerItem = 0;
//			QMetaObject::invokeMethod(launcher, "createPointer", Qt::QueuedConnection,
//									  Q_RETURN_ARG(QGraphicsItem *, pointerItem),
//									  Q_ARG(quint64, uiclientid),
//									  Q_ARG(QColor, QColor(r,g,b)),
//									  Q_ARG(QString, QString(pname)));

			PolygonArrow *pointerItem = 0;
			pointerItem = new PolygonArrow(uiclientid, settings, QColor(r,g,b));
			Q_ASSERT(pointerItem);
			if (!pname.isNull() && !pname.isEmpty()) {
				pointerItem->setPointerName(QString(pname));
			}
			scene->addItem(pointerItem);

			pointers.insert(uiclientid, pointerItem);

			break;
		}
	case POINTER_UNSHARE: {
			quint64 uiclientid;
			sscanf(msg.constData(), "%d %llu", &code, &uiclientid);
			// find the PixmapArrow associated with this uiClientId
			QGraphicsItem *pa = arrow(uiclientid);
			if (pa) {
//				gviewMain->scene()->removeItem(pa);
				delete pa;
				pointers.remove(uiclientid);
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}

/*
  Nothing special happens. just the Pointer item is going to move on the scene
  */
	case POINTER_MOVING: {
			quint64 uiclientid;
			int x,y;
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);

			// find the PolygonArrow associated with this uiClientId
			PolygonArrow *pa = arrow(uiclientid);
			if (pa) {
//				pa->setPos(x,y);
				pa->pointerMove(QPointF(x,y), Qt::NoButton); // just move pointer graphics item
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}

		/*
		  POINTER_PRESS is done already
		  widget under the pointer is going to be moved by QGraphicsItem::moveBy()
		  */
	case POINTER_DRAGGING: {
			quint64 uiclientid;
			int x,y;
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);

			PolygonArrow *pa = arrow(uiclientid);
			if (pa) {
				// direct widget manipulation instead of posting mouse event
				pa->pointerMove(QPointF(x,y), Qt::LeftButton);


/***********
				pa->setPos(x, y); // x, y is in parent coordinate, scene coordiante in this case

				// generate mouse event directly from ui server
				QMouseEvent moveEvent(QEvent::MouseMove, gviewMain->mapFromScene(QPointF(x,y)), Qt::LeftButton, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);
				pa->appUnderPointer()->grabMouse();
				qDebug() << QPointF(x,y);
				if ( ! QApplication::sendEvent(gviewMain->viewport(), &moveEvent) ) {
					qDebug("UiServer::%s() : sendEvent MouseMovePress failed", __FUNCTION__);
				}
				pa->appUnderPointer()->ungrabMouse();
				******************/
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}

			break;
		}


/*
  Pointer will have widget if there is one under it
  */
	case POINTER_PRESS: {
			quint64 uiclientid;
			int x,y;
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
			PolygonArrow *pa =  arrow(uiclientid) ;
			if (pa) {
				qDebug("UiServer::%s() : POINTER_PRESS : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());

				// Below will call setAppUnderPointer()
				pa->pointerPress(QPointF(x,y), Qt::LeftButton, Qt::NoButton | Qt::LeftButton);
//				pa->setAppUnderPointer(QPointF(x,y));
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}
		/**
	case POINTER_RELEASE: {
			quint64 uiclientid;
			int x,y;
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
			PolygonArrow *pa =  arrow(uiclientid) ;
			if (pa) {
				qDebug("UiServer::%s() : POINTER_RELEASE : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());
				pa->pointerRelease(QPointF(x,y), Qt::LeftButton, Qt::LeftButton);
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}
		**/

	case POINTER_DOUBLECLICK: {
			quint64 uiclientid;
			int x,y;
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
			PolygonArrow *pa =  arrow(uiclientid) ;
			if (pa && scene) {
				// direct widget manipulation
//				pa->pointerDoubleClick(QPointF(x,y), Qt::LeftButton, Qt::LeftButton);


				// Generate mouse event directly from here
				QGraphicsView *gview = 0;
				foreach(QGraphicsView *v, scene->views()) {
					if ( v->rect().contains(x, y) ) {
						gview = v;
						break;
					}
				}

				if (gview) {
					QMouseEvent dblClickEvent(QEvent::MouseButtonDblClick, gview->mapFromScene(QPointF(x,y)), Qt::LeftButton, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);

					// sendEvent doesn't delete event object, so event should be created in stack space
					if ( ! QApplication::sendEvent(gview->viewport(), &dblClickEvent) ) {
						qDebug("UiServer::%s() : sendEvent MouseMuttonDblClick on %d,%d failed", __FUNCTION__,x,y);
					}
				}
				else {
					qDebug("%s::%s() : there is no viewport on %d, %d", metaObject()->className(), __FUNCTION__, x, y);
				}
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}


	case POINTER_WHEEL: {
			quint64 uiclientid;
			int x,y,tick;
			sscanf(msg.constData(), "%d %llu %d %d %d", &code, &uiclientid, &x, &y, &tick);
			PolygonArrow *pa = arrow(uiclientid);
			if (pa) {
//				qDebug() << "UiServer:: wheel" << x << y << 120*tick;
				pa->pointerWheel(QPointF(x,y), 120 * tick);
			}
//			if (pa && pa->setAppUnderPointer(QPointF(x,y))) {
//				qDebug() << "WHEEL " << tick;
//				Q_ASSERT(pa->appUnderPointer());
////				pa->appUnderPointer()->reScale(tick, 0.03);
//			}
			break;
		}

		/**
		  * mouseReleaseEvent from external GUI sends POINTER_CLICK message
		  */
	case POINTER_CLICK: {
			quint64 uiclientid;
			int x,y; // x,y from ui clients is the scene position of the wall
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
			PolygonArrow *pa =  arrow(uiclientid) ;
			if (pa) {
				qDebug("UiServer::%s() : POINTER_CLICK : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());

				// Widget under the pointer should reimplement BaseWidget::mouseClick()
				pa->pointerClick(QPointF(x,y), Qt::LeftButton, Qt::LeftButton);



				/***
				// Generate mouse event directly from here
				QMouseEvent pressEvent(QEvent::MouseButtonPress, gviewMain->mapFromScene(QPointF(x,y)), Qt::LeftButton, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);
				QMouseEvent releaseEvent(QEvent::MouseButtonRelease, gviewMain->mapFromScene(QPointF(x,y)), Qt::LeftButton, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);

				// If another item calls grabMouse() this item will lose mouse grab. this item will regain the mouse grab when the other item calls ungrabMouse()
				//					pa->appUnderPointer()->grabMouse(); // inherited from QGraphicsItem

				// sendEvent doesn't delete event object, so event should be created in stack space
				if ( ! QApplication::sendEvent(gviewMain->viewport(), &pressEvent) ) {
					qDebug("UiServer::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
				}
				else {
					if ( ! QApplication::sendEvent(gviewMain->viewport(), &releaseEvent) ) {
						qDebug("UiServer::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
					}
				}

				//					pa->appUnderPointer()->ungrabMouse();
				**/

				/*
 // event loop will take ownership of posted event, so event must be created in heap space
 QApplication::postEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier));
 QApplication::postEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier));
 */
				//				}
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}
	case POINTER_RIGHTCLICK: {
			quint64 uiclientid;
			int x,y;
			sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
			PolygonArrow *pa =  arrow(uiclientid) ;
			if (pa) {
				qDebug("UiServer::%s() : POINTER_RIGHTCLICK : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());
//				if ( pa->setAppUnderPointer( QPointF(x,y)) ) {
//					if ( pa->appUnderPointer()->isSelected() ) {
//						pa->appUnderPointer()->setSelected(false);
//					}
//					else {
//						pa->appUnderPointer()->setSelected(true);
//					}
//				}
			}
			else {
				qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
			}
			break;
		}


	} /* end of switch */
}

void UiServer::removeFinishedThread(quint64 uiclientid) {
	int removed = uiThreadsMap.remove(uiclientid);
	if ( removed > 1 ) {
		qDebug("UiServer::%s() : %d entries deleted from uiThreadsMap. There were duplicate uiClientId", __FUNCTION__, removed);
	}
	else if ( removed == 0 ) {
		qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
	}

	removed = appLayoutFlagMap.remove(uiclientid);
	if ( removed > 1 ) {
		qDebug("UiServer::%s() : %d entries deleted from appLayoutMap. There were duplicate uiClientId", __FUNCTION__, removed);
	}
	else if ( removed == 0 ) {
		qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
	}

	/* delete shared pointer */
	QGraphicsItem *pa = arrow(uiclientid);
	if (pa) {
		if (scene) scene->removeItem(pa);
		delete pa;

		removed = pointers.remove(uiclientid);
		if ( removed > 1 ) {
			qDebug("UiServer::%s() : %d entries deleted. There were duplicate uiClientId", __FUNCTION__, removed);
		}
		else if ( removed == 0 ) {
			qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
		}
	}
}

/*!
  This is called periodically in GraphicsViewMain::timerEvent()
  */
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
	QMap<quint64, UiMsgThread*>::const_iterator iter = uiThreadsMap.constBegin();
	UiMsgThread *msgthr = 0;

	QMap<quint64, bool>::const_iterator flagiter;

	// only send appLayout info to a uiclient who chose to receive
	for (; iter!=uiThreadsMap.constEnd(); iter++) {
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


UiMsgThread * UiServer::getUiMsgThread(quint64 uiclientid) {
	QMap<quint64, UiMsgThread*>::iterator it = uiThreadsMap.find(uiclientid);
	if ( it == uiThreadsMap.end()) return 0;
	UiMsgThread *ret = it.value();
	return ret;
}

PolygonArrow * UiServer::arrow(quint64 uiclientid) {
	QMap<quint64, PolygonArrow *>::iterator it = pointers.find(uiclientid);
	if ( it == pointers.end() ) return 0;
	return it.value();
}
