#include "uiserver.h"
//#include "../applications/base/basegraphicswidget.h"
#include "../sagenextscene.h"
#include "../sagenextlauncher.h"
#include "../applications/base/basewidget.h"
#include "../common/filereceivingrunnable.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QSettings>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

UiServer::UiServer( const QSettings *s, SAGENextLauncher *snl, SAGENextScene *sns)
    : QTcpServer(0)
    , settings(s)
    , fileRecvPortBase(0)
    , uiClientId(0)
    , scene(sns)
    , launcher(snl)
{
    uiMsgThreadsMap.clear();
    uiFileRecvRunnableMap.clear();

    if ( ! listen(QHostAddress::Any, settings->value("general/wallport", 30003).toInt() ) ) {
        qCritical("UiServer::%s() : listen failed", __FUNCTION__);
        deleteLater();
    }

    appLayout = new QByteArray(EXTUI_MSG_SIZE, '\0');

    fileRecvPortBase = 10000 + s->value("general/wallport", 30003).toInt();

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

    /*
         send uiclientid and scene size
        */
    char buf[EXTUI_MSG_SIZE]; memset(buf, '\0', EXTUI_MSG_SIZE);
    int filetransferport = fileRecvPortBase + uiClientId;
    sprintf(buf, "%llu %d %d %d", uiClientId, (int)scene->width(), (int)scene->height(), filetransferport);
    ::send(sockfd, buf, 1280, 0);
    qDebug("UiServer::%s() : The scene size %d x %d sent to ui %llu", __FUNCTION__, (int)scene->width(), (int)scene->height(), uiClientId);


    /**
          create a msg thread
          */
    UiMsgThread *thread = new UiMsgThread(uiClientId, sockfd, this);
    uiMsgThreadsMap.insert(uiClientId, thread);

    connect(this, SIGNAL(destroyed()), thread, SLOT(breakWhileLoop()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(clientDisconnected(quint64)), this, SLOT(removeFinishedThread(quint64)));
    connect(thread, SIGNAL(msgReceived(quint64, UiMsgThread *, QByteArray)), this, SLOT(handleMessage(const quint64, UiMsgThread *, const QByteArray)));

    thread->start();



    /**
          create a file recv thread
          */
    //        FileReceivingTcpServer *recvThread = new FileReceivingTcpServer(launcher, filetransferport);
    //        recvThread->setAutoDelete(false);
    //        uiFileRecvRunnableMap.insert(uiClientId, recvThread);




    // by default, ui client is not receiving app layout on the wall
    appLayoutFlagMap.insert(uiClientId, false);


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
        int receiverPort = fileRecvPortBase + id;
        if ( receiverPort >= 65535 ) {
            qCritical("UiServer::%s() : file recv port is %d", __FUNCTION__, receiverPort);
            break;
        }


        /**
                          Trigger FileReceivingThread
                          The media will be launched in here
                          **/
        //                        FileReceivingTcpServer *recvThread = uiFileRecvRunnableMap.value(id);
        //                        recvThread->setFileInfo(QString(filename), filesize); // this can block because of the mutex
        //                        QThreadPool::globalInstance()->start(recvThread);


        QtConcurrent::run(this, &UiServer::fileReceivingFunction, mediatype, QString(filename), filesize, receiverPort);


        /*
         send ACK_FROM_WALL through msg channel
         This will trigger uiclient to transfer the file
        */
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
		char username[64];
        sscanf(msg.constData(), "%d %llu %s %d %s %s %d", &code, &uiclientid, senderIP, &display, username, vncpass, &framerate);

//		qDebug() << "uiserver" << QString(username) << QString(vncpass);

        // This isn't elegant way of invoking vnc widget. :(
        QMetaObject::invokeMethod(launcher, "launch", Qt::QueuedConnection,
		                          Q_ARG(QString, QString(username)),
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

        //qDebug("UiServer::%s() : POINTER_SHARE from uiclient %llu, (%s, %d %d %d)",__FUNCTION__, uiclientid, pname.constData(), r,g,b);

        PolygonArrow *pointerItem = 0;
        pointerItem = new PolygonArrow(uiclientid, settings, QColor(r,g,b));
        Q_ASSERT(pointerItem);
        if (!pname.isNull() && !pname.isEmpty()) {
            pointerItem->setPointerName(QString(pname));
        }
        Q_ASSERT(scene);
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
            //pa->setPos(x,y);
            pa->pointerMove(QPointF(x,y), Qt::NoButton); // just move pointer graphics item
        }
        else {
            qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
        }
        break;
    }

    /*
      An external ui client's mouseMoveEvent plus left button modifier triggers this
      This is always followed by POINTER_PRESS
    */
    case POINTER_DRAGGING: {
        quint64 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);

        PolygonArrow *pa = arrow(uiclientid);
        if (pa) {
            // direct widget manipulation instead of posting mouse event
            pa->pointerMove(QPointF(x,y), Qt::LeftButton);
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
//            qDebug("UiServer::%s() : POINTER_PRESS : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());

            // Below will call setAppUnderPointer()
            // followed by setTopmost()
            pa->pointerPress(QPointF(x,y), Qt::LeftButton, Qt::NoButton | Qt::LeftButton);
            //pa->setAppUnderPointer(QPointF(x,y));
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

        /**
         * mouseReleaseEvent from external GUI sends POINTER_CLICK message
         */
    case POINTER_CLICK: {
        quint64 uiclientid;
        int x,y; // x,y from ui clients is the scene position of the wall
        sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
        PolygonArrow *pa =  arrow(uiclientid) ;
        if (pa) {
            qDebug("UiServer::%s() : POINTER_CLICK : pointer clicked position (%.0f, %.0f)", __FUNCTION__, pa->x(), pa->y());

//            pa->pointerClick(QPointF(x,y), Qt::LeftButton, Qt::LeftButton);

			// Let each application provides mouseClick()
			 // Widget under the pointer can reimplement BaseWidget::mouseClick()
			if ( pa->appUnderPointer() ) pa->appUnderPointer()->mouseClick(QPointF(x, y), Qt::LeftButton);
        }
        else {
            qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
        }
        break;
    }



        /**
          In response to external UI's right release
          */
    case POINTER_RIGHTCLICK: {
        quint64 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
        PolygonArrow *pa =  arrow(uiclientid) ;
        if (pa) {
//            qDebug("UiServer::%s() : POINTER_RIGHTCLICK : pointer's pos %.0f, %.0f", __FUNCTION__, pa->x(), pa->y());
            pa->pointerClick(QPointF(x,y), Qt::RightButton, Qt::RightButton);
        }
        else {
            qDebug("UiServer::%s() : can't find pointer object", __FUNCTION__);
        }
        break;
    }


    case POINTER_DOUBLECLICK: {
        quint64 uiclientid;
        int x,y;
        sscanf(msg.constData(), "%d %llu %d %d", &code, &uiclientid, &x, &y);
        PolygonArrow *pa =  arrow(uiclientid) ;
        if (pa) {
            //qDebug() << "UiServer:: wheel" << x << y << 120*tick;
            pa->pointerDoubleClick(QPointF(x, y), Qt::LeftButton, Qt::LeftButton);
        }

        break;
    }


    case POINTER_WHEEL: {
        quint64 uiclientid;
        int x,y,tick;
        sscanf(msg.constData(), "%d %llu %d %d %d", &code, &uiclientid, &x, &y, &tick);
        PolygonArrow *pa = arrow(uiclientid);
        if (pa) {
            //qDebug() << "UiServer:: wheel" << x << y << 120*tick;
            pa->pointerWheel(QPointF(x, y), 120 * tick);
        }
        break;
    }

    } /* end of switch */
}

void UiServer::removeFinishedThread(quint64 uiclientid) {
    int removed = uiMsgThreadsMap.remove(uiclientid);
    if ( removed > 1 ) {
        qDebug("UiServer::%s() : %d entries deleted from uiThreadsMap. There were duplicate uiClientId", __FUNCTION__, removed);
    }
    else if ( removed == 0 ) {
        qDebug("UiServer::%s() : No such uiClientId", __FUNCTION__);
    }

    removed = uiFileRecvRunnableMap.remove(uiclientid);
    if ( removed > 1 ) {
        qDebug("UiServer::%s() : %d entries deleted from uiFileRecvThreadsMap. There were duplicate uiClientId", __FUNCTION__, removed);
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
    QMap<quint64, UiMsgThread*>::const_iterator iter = uiMsgThreadsMap.constBegin();
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


UiMsgThread * UiServer::getUiMsgThread(quint64 uiclientid) {
    QMap<quint64, UiMsgThread*>::iterator it = uiMsgThreadsMap.find(uiclientid);
    if ( it == uiMsgThreadsMap.end()) return 0;
    UiMsgThread *ret = it.value();
    return ret;
}

PolygonArrow * UiServer::arrow(quint64 uiclientid) {
    QMap<quint64, PolygonArrow *>::iterator it = pointers.find(uiclientid);
    if ( it == pointers.end() ) return 0;
    return it.value();
}


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



/* Let the client close the connection */
    sleep(1);
//    ::shutdown(socket, SHUT_RDWR);
//    ::close(socket);
//    ::close(serverSock);
}
