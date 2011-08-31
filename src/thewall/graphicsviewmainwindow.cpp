#include "graphicsviewmainwindow.h"

//#include <QtGlobal>
#include <QtOpenGL>

#if defined(Q_OS_LINUX)
#include <numa.h>
#include <utmpx.h>
#endif

#include "common/commonitem.h"

#include "system/resourcemonitor.h"
#include "system/sagenextscheduler.h"
#include "system/resourcemonitorwidget.h"

#include "uiserver/uiserver.h"
#include "uiserver/launcher.h"

#include "sage/fsManager.h"
#include "sage/fsmanagermsgthread.h"

#include "applications/base/affinityinfo.h"
#include "applications/base/dummyplugininterface.h"
#include "applications/base/basewidget.h"
#include "applications/sagestreamwidget.h"
#include "applications/pixmapwidget.h"
#include "applications/webwidget.h"
#include "applications/vncwidget.h"


GraphicsViewMain::GraphicsViewMain(const QSettings *s, ResourceMonitor *rm, SchedulerControl *ss) :
	globalAppId(1),
	settings(s),
	gscene(0),
	extUiServer(0),
	fsm(0),

	paintingOnCpu(-1),
	textItem(0),

	resourceMonitor(rm),
	scheduler(ss),

	fdialog(0),

	rMonitorWidget(0)
{
	createActions();

	sageWidgetInfo.bwPtr = 0;
	sageWidgetInfo.gaid = -1;
}

void GraphicsViewMain::mousePressEvent(QMouseEvent *event) {
//	qDebug() << "GraphicsViewMain::mousePressEvent() : " << event;
	QGraphicsView::mousePressEvent(event);
}

GraphicsViewMain::~GraphicsViewMain()
{
	killTimer(timerID);

	if (rMonitorWidget) {
		rMonitorWidget->close();
	}


	/* must close all UI connection */
	if (extUiServer) delete extUiServer;

	/* must close all sage app */
	if (fsm) delete fsm;

	if (scheduler) delete scheduler;

	if (resourceMonitor) {
		prelimDataFile.flush();
		prelimDataFile.close();
		delete resourceMonitor;
	}


	delete gscene;

	qDebug() << "GOOD BYE";
}


void GraphicsViewMain::startWall() {
	/** init graphics **/
	qWarning("%s::%s() : Init graphics base", metaObject()->className(), __FUNCTION__);

	gscene = new QGraphicsScene(this);
	gscene->setBackgroundBrush(QColor(20, 20, 20));
	gscene->setSceneRect(0, 0, settings->value("general/width").toDouble(), settings->value("general/height").toDouble());
	/*  This approach is ideal for dynamic scenes, where many items are added, moved or removed continuously. */
	gscene->setItemIndexMethod(QGraphicsScene::NoIndex);

	setScene(gscene);
	resize(gscene->sceneRect().size().toSize());
	move(settings->value("general/offsetx", 0).toInt(), settings->value("general/offsety", 0).toInt());


	/** attach pixmap buttons **/
	QPixmap closeIcon(":/resources/close_over.png");
	PixmapCloseButton *pb = new PixmapCloseButton(closeIcon.scaledToWidth(gscene->width() * 0.02));
	gscene->addItem(pb);
	pb->setX( gscene->width() - pb->boundingRect().width() - 1);

//	QPushButton *b1 = new QPushButton("layout 1");
//	QPushButton *b2 = new QPushButton("layout 2");
//	QGraphicsProxyWidget *pw1 = gscene->addWidget(b1);
//	QGraphicsProxyWidget *pw2 = gscene->addWidget(b2);


	/* launcher.. temporary */

	/*
	Launcher *l = new Launcher(this);
	l->moveBy(100, 200);
	gscene->addItem(l);
*/

	/** resource monitor **/
	if ( resourceMonitor ) {
		/***
		int row = 2;
		int col = 3;
		QRectF sceneRectF = gscene->sceneRect(); // be careful. ROIRectF should be on viewport coordinate. For now viewport rect and scenrect is same
		qreal rectWidth = sceneRectF.width() / col;
		qreal rectHeight = sceneRectF.height() / row;
		for (int i=0; i< (row * col) ; i++) {
			ROIRectF *r = new ROIRectF(i, this);
			r->setWidth(rectWidth);
			r->setHeight(rectHeight);
			r->setX((i % col) * rectWidth);
			r->setY(  (i<col)? 0 : rectHeight ); // this isn't correct if grid changes

			resourceMonitor->addROIRectF(r);
		}
		****/

//            prelimDataFlag = false;
		char *val = 0;
		if (val=getenv("EXP_DATA_FILE")) {
#if QT_VERSION >= 0x040700
			prelimDataFile.setFileName( QString::number(QDateTime::currentMSecsSinceEpoch()) + ".data" );
#else
			prelimDataFile.setFileName( QDateTime::currentDateTime().toString("MM.dd.yyyy_hh.mm.ss.zzz") + ".data");
#endif
			if ( ! prelimDataFile.open(QIODevice::ReadWrite) ) {
				qDebug() << "can't open prelim data file" << prelimDataFile.fileName();
			}
			QString dataHeader = "";
			if ( scheduler) {
				SelfAdjustingScheduler *sas = static_cast<SelfAdjustingScheduler *>(scheduler->scheduler());
				dataHeader.append("Scheduler (");
				// freq, sensitivity, aggression, greediness
				dataHeader.append("Frequency: " + QString::number(scheduler->scheduler()->frequency()) );
				dataHeader.append(" Sensitivity: " + QString::number(sas->getQTH()));
				dataHeader.append(" Aggression: " + QString::number(sas->getDecF()));
				dataHeader.append(" Greediness: " + QString::number(sas->getIncF()));
				dataHeader.append(")");
			}
			else {
				dataHeader.append("No Scheduler");
			}

			if (settings->value("system/sailaffinity").toBool()) {
				dataHeader.append(", SAIL Affinity");
			}
			if (settings->value("system/threadaffinityonirq").toBool()) {
				dataHeader.append(", Thread Affinity on IRQ");
			}
			dataHeader.append("\n");
			prelimDataFile.write( qPrintable(dataHeader) );
		}


		resourceMonitor->refresh();
//		resourceMonitor->textItem->moveBy(5, 30);
//		gscene->addItem(resourceMonitor->textItem);
	}



	/** fsManager **/
	qWarning("GraphicsViewMain::%s() : Starting fsManager", __FUNCTION__);
	//sageWidgetMap.clear();
	fsm = new fsManager(settings, this);
	connect(fsm, SIGNAL(sailConnected(const quint64,QString, int, int, const QRect)), this, SLOT(startSageApp(const quint64,QString, int, int, const QRect)));
//	connect(fsm, SIGNAL(sailDisconnected(quint64)), this, SLOT(stopSageApp(quint64)));



	/** UI server */
	qWarning("GraphicsViewMain::%s() : Starting UI server", __FUNCTION__);
	extUiServer = new UiServer(settings, this);
	connect(extUiServer, SIGNAL(registerApp(MEDIA_TYPE, QString, qint64, QString, QString, quint16)),
			this, SLOT(startApp(MEDIA_TYPE, QString, qint64, QString, QString, quint16)));


//	textItem = new SwSimpleTextItem();
//	textItem->setText("No Religion, No War");
//	textItem->moveBy(10, 20);
//	gscene->addItem( textItem );


	/* WebWidget */
	// passing Qt::Window is very important in WebWidget which is default
	/*
	WebWidget *w = new WebWidget(globalAppId++, settings);
	gscene->addItem(w);
	w->moveBy(100, 100);
	w->setUrl("http://google.com");
	*/



	fdialog = new QFileDialog(this, "Open Files", QDir::homePath().append("/.sagenext") , "RatkoData (*.log);;Session (*.session);;Plugins (*.so *.dll *.dylib);;Images (*.tif *.tiff *.svg *.bmp *.png *.jpg *.jpeg *.gif *.xpm);;Videos (*.mov *.avi *.mpg *.mp4 *.mkv *.flv *.wmv);;Any (*)");
	fdialog->setModal(false);
//	fdialog->setFileMode(QFileDialog::Directory);
	connect(fdialog, SIGNAL(filesSelected(QStringList)), this, SLOT(on_actionFilesSelected(QStringList)));


	/** starts timer */
	timerID = startTimer(settings->value("system/refreshintervalmsec", 1000).toInt()); // default 1000 msec



	if (resourceMonitor) {
		rMonitorWidget = new ResourceMonitorWidget(this, resourceMonitor, scheduler);
		if ( rMonitorWidget) rMonitorWidget->show();
	}



	// vnc
		/**
	VNCClientWidget *vw = new VNCClientWidget(globalAppId, "131.193.77.191", 0, "evl123", 24, settings, resourceMonitor, scheduler);
	gscene->addItem(vw);
	vw->setTopmost();
	vw->moveBy(100,100);
	++globalAppId;
		**/
}



/**
  * UiServer triggers this slot
  */
void GraphicsViewMain::startApp(MEDIA_TYPE type, QString filename, qint64 fsize /* 0 */, QString senderIP /* 127.0.0.1 */, QString recvIP /* "" */, quint16 recvPort /* 0 */) {
	qDebug("GraphicsViewMain::%s() : filesize %lld, senderIP %s, recvIP %s, recvPort %hd", __FUNCTION__, fsize, qPrintable(senderIP), qPrintable(recvIP), recvPort);

	BaseWidget *w = 0;
	switch(type) {
	case MEDIA_TYPE_IMAGE: {

		// streaming from ui client
		if ( fsize > 0 && !recvIP.isEmpty() && recvPort > 0)
			w = new PixmapWidget(fsize, senderIP, recvIP, recvPort, globalAppId, settings);

		// from local storage
		else if ( !filename.isEmpty() )
			w = new PixmapWidget(filename, globalAppId, settings);
		else
			qCritical("%s() : MEDIA_TYPE_IMAGE can't open", __FUNCTION__);

		break;
	}

	case MEDIA_TYPE_VIDEO: {

		// download video file from ui client

		// invoke sail
		QProcess *proc1 = new QProcess(this);
		QStringList args1;
		args1 << "-xf" << senderIP << "\"cd $HOME/.sageConfig;$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0 $HOME/media/video/HansRosling_2006_480.mp4\"";

		// this will invoke sail (outside of SAGENext)
		// globalAppId shouldn't be incremented in here because StartSageApp() will increment eventually
		// Also SageStreamWidget will be added to the scene in there
		proc1->start("ssh",  args1);

		break;
	}


	case MEDIA_TYPE_LOCAL_VIDEO: {

		if ( ! filename.isEmpty() ) {
			QProcess *proc = new QProcess(this);
			QStringList args;
			args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-identify" << filename;

			// this will invoke sail (outside of SAGENext)
			// globalAppId shouldn't be incremented in here because StartSageApp() will increment eventually
			// Also SageStreamWidget will be added to the scene in there
			proc->start("mplayer",  args);
		}

		break;
	}

	case MEDIA_TYPE_AUDIO: {
		break;
	}
	case MEDIA_TYPE_UNKNOWN: {
		break;
	}

	case MEDIA_TYPE_VNC: {

		// filename - vncpasswd
		// fsize - display number
		// recvPort - frame rate
		w = new VNCClientWidget(globalAppId, senderIP, fsize, filename, recvPort, settings, resourceMonitor, scheduler);

		break;
	}

	case MEDIA_TYPE_PLUGIN: {
		QPluginLoader *loader = new QPluginLoader(filename);
		QObject *plugin = loader->instance();
		if (!plugin) {
			qWarning("%s::%s() : %s", metaObject()->className(), __FUNCTION__, qPrintable(loader->errorString()));
			return;
		}
		DummyPluginInterface *dpi = qobject_cast<DummyPluginInterface *>(plugin);
		if (!dpi) {
			qCritical() << "qobject_cast<DummyPluginInterface *>(plugin) failed";
			return;
		}

		/*
		  A plugin inherits BaseWidget
		  */
		if (dpi->name() == "BaseWidget") {
			w = static_cast<BaseWidget *>(plugin);

			/*************************
			 Below is very important because plugin always created using DEFAULT constructor !
			 **************************/
			w->setSettings(settings);
			w->setGlobalAppId(globalAppId);
		}
		/*
		  A plugin inherits QWidget
		  */
		else {
			w = new BaseWidget(globalAppId, settings, resourceMonitor);
			w->setProxyWidget(dpi->rootWidget()); // inefficient
			w->setWindowFlags(Qt::Window);
			w->setWindowFrameMargins(4,25,4,4);
		}

		if (w) {
			w->moveBy(100, 200);
		}
		else {
			qDebug() << "Failed to create widget from plugin object";
		}
		break;
	} // end MEDIA_TYPE_PLUGIN

	} // end switch

	if ( w ) {
		gscene->addItem(w);
		connect(this, SIGNAL(showInfo()), w, SLOT(drawInfo()));
		++globalAppId; // increment only when widget is created successfully
	}
	else {
		qCritical("GraphicsViewMain::%s() : widget failed instantiation", __FUNCTION__);
	}
}

/*!
  * When this function is called, the sail maybe is in blocking state because of ::connect()
  */
void GraphicsViewMain::startSageApp(const quint64 sageAppId, QString appName, int protocol, int port, const QRect initRect) {
	qDebug("GraphicsViewMain::%s() : (%s) Starting sage app id %llu, globalAppId %llu", __FUNCTION__, qPrintable(appName), sageAppId, globalAppId);

//	connect(affinfo, SIGNAL(affInfoChanged(quint64)), resourceMonitor, SLOT(updateAffInfo(quint64)));
//	connect(affinfo, SIGNAL(destroyed(QObject*)), resourceMonitor, SLOT(removeApp(QObject*)));
//	connect(affinfo, SIGNAL(destructor(quint64)), resourceMonitor, SLOT(removeApp(quint64)));

	/* resource monitor assigns appropriate bitmask for this app */

	SageStreamWidget *sageWidget = new SageStreamWidget(sageAppId, appName, protocol, port, initRect, globalAppId, settings, resourceMonitor, scheduler);
	if (!sageWidget) {
		qCritical("GraphicsViewMain::%s() : failed to create SageStreamWidget", __FUNCTION__);
		return;
	}
	else {
//		qDebug("GraphicsViewMain::%s() : [[[[[[[[[[[[[[[[[[   (%x)   ]]]]]]]]]]]]]]]]]]]\n", __FUNCTION__, sageWidget);

		// add to globalWidgetMap
//		globalWidgetMap.insert(globalAppId, sageWidget);
//		widgetInfoMutex.lock();

//		widgetInfoMutex.unlock();
//		widgetInfoFilled.wakeOne();

	}

	Q_ASSERT(sageWidget);
	connect(this, SIGNAL(showInfo()), sageWidget, SLOT(drawInfo()));

	connect(this, SIGNAL(destroyed()), sageWidget, SLOT(close()));

	// Sender (SAIL) should quit too. So, send APP_QUIT message to sail
	connect(sageWidget, SIGNAL(destructor(quint64)), fsm, SIGNAL(shutdownSail(quint64)));


	/*!
	  Resource monitor & processor assignment
	  */
	if(sageWidget && sageWidget->affInfo() && resourceMonitor)
	{
		connect(sageWidget->affInfo(), SIGNAL(cpuOfMineChanged(RailawareWidget *,int,int)), resourceMonitor, SLOT(updateAffInfo(RailawareWidget *,int,int)));

		// Below won't be necessary because BaseWidget::fadeOutClose() takes care widget removal from resource monitor
		// It's moved to fadeOutClose() because removal has to be handled early
		//connect(sageWidget->getAffInfoPtr(), SIGNAL(destroyed(QObject*)), resourceMonitor, SLOT(removeApp(QObject *)));

		/*******
		 This is used to send affinity info to sender (sail) so that sender can set proper affinity based on receivers affinity setting. This should be connected only when mplayer is run localy
		 *********/
		Q_ASSERT(fsm);
		connect(sageWidget->affInfo(), SIGNAL(streamerAffInfoChanged(AffinityInfo*, quint64)), fsm, SIGNAL(sailSendSetRailMsg(AffinityInfo*,quint64)));

//		connect(sageWidget, SIGNAL(destroyed()), resourceMonitor,SLOT(loadBalance()));

		// assign most underloaded processor
//		resourceMonitor->assignProcessor(sageWidget);

		// update QList<RailawareWidget *> ResourceMonitor::widgetList
		resourceMonitor->addSchedulableWidget(sageWidget);
//		resourceMonitor->updateAffInfo(sageWidget, -1, -1);

		if (scheduler) {
			// assign affinity
		}
	}
	gscene->addItem(sageWidget);

	// This is needed so that BaseWidget::effectiveVisibleRegion() returns correct value
	sageWidget->setTopmost();

//	qreal random = 1.0 / (qreal)(qrand() + 1); // 0 < x <= 1
//	random *= 0.7;
//	sageWidget->moveBy(scene()->width() * random, scene()->height() * (1-random));

	++globalAppId;

	// for loadSavedScenario()
	sageWidgetInfo.bwPtr = sageWidget;
	sageWidgetInfo.gaid = globalAppId;
}

/*
void GraphicsViewMain::stopSageApp(quint64 appID) {
	qDebug("GraphicsViewMain::%s() : appID %llu", __FUNCTION__, appID);
}
*/

/*!
  Note that this function doesn't create apps.
  It assumes apps are already running on the wall and just apply geometry info for each app on the wall
  */
void GraphicsViewMain::loadSavedSession(const QString fn) {

	QFile sessionFile(fn);

	if (!sessionFile.exists()) return;
	if (!sessionFile.open(QIODevice::ReadOnly)) return;

	qDebug() << "Loading a session from file: " << fn;
	QDataStream in(&sessionFile);

	foreach (QGraphicsItem *gi, scene()->items()) {
		if (!gi) continue;
		if (gi->type() != QGraphicsItem::UserType + 2) continue;

		BaseWidget *bw = static_cast<BaseWidget *>(gi);

		QPointF pos; // position
		qreal sc; // scale
		qreal zv; // zValue

		in >> pos >> sc >> zv;

//		qDebug() << pos << sc << zv;

		QParallelAnimationGroup *pag = bw->animGroup();

//		bw->setPos(pos);
//		bw->setScale(sc);

		QAbstractAnimation *a = pag->animationAt(0);
		QPropertyAnimation *ppos = qobject_cast<QPropertyAnimation *>(a);

		ppos->setStartValue(bw->pos());
		ppos->setEndValue(pos);

		a = pag->animationAt(1);
		QPropertyAnimation *pscl = qobject_cast<QPropertyAnimation *>(a);

		pscl->setStartValue(bw->scale());
		pscl->setEndValue(sc);

		bw->setZValue(zv);
		pag->start();
	}
//	QString text(fn);
//	text.prepend("Loading new session ...\n");
//	QGraphicsTextItem ti(text);

//	scene()->addItem(&ti);
//	ti.setZValue(19761205);
//	ti.adjustSize();
//	ti.moveBy( (scene()->width() / 2) - (ti.boundingRect().size().width() / 2) ,  (scene()->height() / 2) - (ti.boundingRect().size().height() / 2) );

//	/*
//	  clear the wall
//	  */
//	foreach (QGraphicsItem *gi, scene()->items()) {
//		if (!gi) continue;
//		if (gi->type() != QGraphicsItem::UserType + 2 ) continue;

//		BaseWidget *bw = static_cast<BaseWidget *>(gi);
//		bw->close();
//	}

////	char line[8192];
//	QByteArray line = sessionFile.readLine();
//	while ( !line.isNull()   &&   !line.isEmpty() ) {

//		if ( line.startsWith('#') ) continue;

//		int mtype;
//		QString filepath;
//		qreal posx, posy;
//		qreal scale;
//		qreal expectedFPS;

//		QList<QByteArray> elements = line.split('|');
//		bool ok = false;

//		// MEDIA_TYPE | filename path | scene position x | scene position y | scale | expectedFps
//		mtype = elements.at(0).toInt(&ok);
//		filepath = QString(elements.at(1));
//		posx = elements.at(2).toDouble(&ok);
//		posy = elements.at(3).toDouble(&ok);
//		scale = elements.at(4).toDouble(&ok);
//		expectedFPS = elements.at(5).toDouble(&ok);

//		if (ok) {
//			// add this
//			startApp( (MEDIA_TYPE)mtype, filepath );

//		}

//		line = sessionFile.readLine();
//	}

//	scene()->removeItem(&ti);

}

void GraphicsViewMain::createActions() {
	changeBGcolorAction = new QAction(this);
	connect(changeBGcolorAction, SIGNAL(triggered()), this, SLOT(on_actionBackground_triggered()));
	addAction(changeBGcolorAction);

	openMediaAction = new QAction(this);
	openMediaAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_O) );
	connect(openMediaAction, SIGNAL(triggered()), this, SLOT(on_actionOpen_Media_triggered()));
	addAction(openMediaAction);

	showInfoAction = new QAction(this);
	showInfoAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_I) );
	connect(showInfoAction, SIGNAL(triggered()), this, SLOT(on_actionShowInfo_triggered()));
	addAction(showInfoAction);

//	schedulingAction = new QAction(this);
////	schedulingAction->setShortcut( QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::ALT + Qt::Key_S));
//	connect(schedulingAction, SIGNAL(triggered()), this, SLOT(on_actionScheduling_triggered()));
//	addAction(schedulingAction);


	saveSessionAction = new QAction(this);
	saveSessionAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_S) );
	connect(saveSessionAction, SIGNAL(triggered()), this, SLOT(on_actionSaveSession_triggered()));
	addAction(saveSessionAction);

//	gridLayoutAction = new QAction(this);
//	gridLayoutAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_G) );
//	connect(gridLayoutAction, SIGNAL(triggered()), this, SLOT(on_actionGridLayout_triggered()));
//	addAction(gridLayoutAction);

//	loadSessionAction = new QAction(this);
}

void GraphicsViewMain::on_actionSaveSession_triggered() {

	QString filename(QDir::homePath());
	filename.append("/.sagenext/sessions/");
	filename.append( QDateTime::currentDateTime().toString() );
	filename.append(".session");

	QFile sFile(filename);
	if (!sFile.open(QIODevice::WriteOnly) ) {
		qCritical() << "file open error";
		return;
	}
	QDataStream out(&sFile);

	foreach (QGraphicsItem *gi, scene()->items()) {
		if (!gi) continue;
		if (gi->type() != QGraphicsItem::UserType + 2) continue;

		BaseWidget *bw = static_cast<BaseWidget *>(gi);
		out << bw->pos() << bw->scale() << bw->zValue();
	}

	sFile.close();
}

/*!
  Openining a media file
  */
void GraphicsViewMain::on_actionOpen_Media_triggered()
{
	if ( fdialog && fdialog->isHidden())
		fdialog->show();
}

void GraphicsViewMain::on_actionFilesSelected(const QStringList &filenames) {

	if ( filenames.empty() ) {
		return;
	}
	else {
		//qDebug("GraphicsViewMain::%s() : %d", __FUNCTION__, filenames.size());
	}

	QRegExp rxVideo("\\.(avi|mov|mpg|mpeg|mp4|mkv|flv|wmv)$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxImage("\\.(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxSession("\\.session$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxRatkoData("\\.log");
	QRegExp rxPlugin;
	rxPlugin.setCaseSensitivity(Qt::CaseInsensitive);
	rxPlugin.setPatternSyntax(QRegExp::RegExp);
#if defined(Q_OS_LINUX)
	rxPlugin.setPattern("\\.so$");
#elif defined(Q_OS_WIN32)
	rxPlugin.setPattern("\\.dll$");
#elif defined(Q_OS_DARWIN)
	rxPlugin.setPattern("\\.dylib$");
#endif

	for ( int i=0; i<filenames.size(); i++ ) {
		//qDebug("GraphicsViewMain::%s() : %d, %s", __FUNCTION__, i, qPrintable(filenames.at(i)));
		QString filename = filenames.at(i);


		if ( filename.contains(rxVideo) ) {
			qDebug("GraphicsViewMain::%s() : Opening a video file %s", __FUNCTION__, qPrintable(filename));
//			w = new PhononWidget(filename, globalAppId, settings);
//			QFuture<void> future = QtConcurrent::run(qobject_cast<PhononWidget *>(w), &PhononWidget::threadInit, filename);

			startApp(MEDIA_TYPE_LOCAL_VIDEO, filename);
			return;
		}

		/*!
		  Image
		  */
		else if ( filename.contains(rxImage) ) {
			qDebug("GraphicsViewMain::%s() : Opening an image file %s", __FUNCTION__, qPrintable(filename));
//			bw = new PixmapWidget(filename, globalAppId, settings);
			startApp(MEDIA_TYPE_IMAGE, filename);
			return;
		}


		/**
		  * plugin
		  */
		else if (filename.contains(rxPlugin) ) {
			qDebug("GraphicsViewMain::%s() : Loading a plugin %s", __FUNCTION__, qPrintable(filename));
			startApp(MEDIA_TYPE_PLUGIN, filename);
		}

		/*!
		  session
		  */
		else if (filename.contains(rxSession) ) {
//			qDebug() << "Loading a session " << filename;
			loadSavedSession(filename);
		}


		/*!
		  Ratko data
		  */
		else if (filename.contains(rxRatkoData) ) {
		//loadSavedScenario(filename);
			QFuture<void> future = QtConcurrent::run(this, &GraphicsViewMain::loadSavedScenario, filename);
//			RatkoDataSimulator *rdsThread = new RatkoDataSimulator(filename);
//			rdsThread->start();
		}


		else if ( QDir(filename).exists() ) {
			qDebug("%s::%s() : DIRECTORY", metaObject()->className(), __FUNCTION__);
			return;
		}
		else {
			qCritical("GraphicsViewMain::%s() : Unrecognized file format", __FUNCTION__);
			return;
		}
	}
}

void GraphicsViewMain::on_actionBackground_triggered()
{
	QColorDialog colorDialog(this);
	colorDialog.exec();
	gscene->setBackgroundBrush(QBrush(colorDialog.selectedColor()));
}

void GraphicsViewMain::on_actionShowInfo_triggered() {
	emit showInfo();
}


QGraphicsItem * GraphicsViewMain::createPointer(quint64 uiclientid, QColor c, QString pointername) {
	PolygonArrow *pa = new PolygonArrow(uiclientid, settings, c);

	if ( !pointername.isNull() && !pointername.isEmpty())
		pa->setPointerName(pointername);

	gscene->addItem(pa);
	return pa;
}




void GraphicsViewMain::paintEvent(QPaintEvent *e) {
//	qDebug("GraphicsViewMain::%s() : ", __FUNCTION__);
#if defined(Q_OS_LINUX)
	paintingOnCpu = sched_getcpu();
#endif
	QGraphicsView::paintEvent(e);
}

void GraphicsViewMain::resizeEvent(QResizeEvent *e) {
	setSceneRect(0, 0, e->size().width(), e->size().height());
	QGraphicsView::resizeEvent(e);
}

void GraphicsViewMain::timerEvent(QTimerEvent *) {

	// when gvm->setViewportUpdateMode(QGraphicsView::NoViewportUpdate); is true
//	if (scene()) scene()->update();

	if (extUiServer) {
		// update app layout on the wall. This info will be sent to uiclient
//		extUiServer->updateAppLayout();
	}
	if (textItem) {
		QString text = "Main paint() is running on ";
		text.append(QString::number(paintingOnCpu));
		textItem->setText( text );
	}

	if (resourceMonitor) {
		char *val = 0;
		if (val=getenv("EXP_DATA_FILE")) {
			if ( prelimDataFile.isOpen() ) {
				resourceMonitor->printPrelimData(&prelimDataFile);
				/*
 if (prelimDataFlag) {
  resourceMonitor->printPrelimData(&prelimDataFile);
  prelimDataFlag = false; // this flag will make printPrelimData called every 2 sec
 }
 else {
  prelimDataFlag = true;
 }
 */
			}
		}

		resourceMonitor->refresh();
		if(rMonitorWidget) rMonitorWidget->refresh();
	}
}


/*!
  start video stream through ssh reading Ratko's user study data
  */
void GraphicsViewMain::loadSavedScenario(const QString file) {
	QFile scenarioFile(file);
	if (!scenarioFile.exists()) return;
	if (!scenarioFile.open(QFile::ReadOnly)) return;

	QString sender("127.0.0.1");
	QString program("checker");
	int timeShrink = 10;
	int fps = 12;
	QString header(file);
	header.append(" ");
	header.append(sender);
	header.append(" ");
	header.append(program);
	header.append(QString(" fps ").append(QString::number(fps)));
	header.append(QString(" timeShrink ").append(QString::number(timeShrink)));
	header.append("\n");


	QMetaObject::invokeMethod(this, "writePrelimDataHeader", Qt::QueuedConnection, Q_ARG(QString, header));

	// maps data's Appid to globalAppId
	QMap<int, BaseWidget *> appMap;
	appMap.clear();

	QMap<int, QProcess *> procMap;
	procMap.clear();

//	qint64 startTime = QDateTime::currentMSecsSinceEpoch();
//	qDebug() << "start time" << startTime;
//	qint64 timeOffset = 0; // to convert ratko's data time to current time

//	QRegExp windowNew("WINDOW NEW");
	QRegExp windowRemove("WINDOW REMOVE");
	QRegExp windowChange("WINDOW CHANGE");
	QRegExp windowZ("WINDOW Z");
	QRegExp window("WINDOW");
//	QRegExp starttime("^\\d+\\.\\d+$");

	char buff[2048]; // line buffer
	int left,right,bottom,top,width,height,resx,resy,zvalue,sageappid,value;

	bool hasStartTime = false;
	qint64 startTime;
	qint64 timestamp;
	char ts[128]; // to temporarily store ratko's timestamp value

	char dummy[1024];
	char dummyChar;
	bool ok = false;

	quint64 linenumber = 0;

	qint64 simulstart = 0;
#if QT_VERSION >= 0x040700
	simulstart = QDateTime::currentMSecsSinceEpoch();
#else
#endif

	while( scenarioFile.readLine(buff, sizeof(buff)) > 0 ) {
		QString line(buff);
		line = line.trimmed();

		++linenumber;

		if (line.contains(window)) {
			sscanf(buff, "%c %s", &dummyChar, ts);
			QString tsstring(ts);
			ok = false;

			if (!hasStartTime) {
				startTime = 1000 * tsstring.toDouble(&ok); // to msec
				if (ok) {
					hasStartTime = true;
					ok = false; // reset flag
				}
				else {
					continue;
				}
			}

			timestamp = 1000 * tsstring.toDouble(&ok); // to msec
			if (!ok) continue;

			// sleep (timestamp - startTime) / timeShrink  msec
			qint64 sleeptime = (timestamp - startTime) / timeShrink;
//			qDebug() << "actual sleep" << timestamp-startTime << " shrinked to" << sleeptime;
			usleep(sleeptime * 1000); // microsec
			startTime = timestamp;
		}

		if (line.contains(windowRemove)) {
			sscanf(buff, "%c %s %c %s %s %d", &dummyChar,ts,&dummyChar,dummy,dummy,&sageappid);

//			qDebug("%llu) REMOVING appid %d\n", linenumber, sageappid);
			if ( appMap.find(sageappid) != appMap.end() ) {
				BaseWidget *bw = appMap.value(sageappid);
				Q_ASSERT(bw);
				SageStreamWidget *ssw = static_cast<SageStreamWidget *>(bw);
				Q_ASSERT(ssw);
				ssw->affInfo()->disconnect();
				QMetaObject::invokeMethod(ssw, "fadeOutClose", Qt::QueuedConnection);

				if (procMap.find(sageappid) != procMap.end() ) {
					QProcess *p = procMap.value(sageappid);
					Q_ASSERT(p);
					p->kill();
				}
			}
			appMap.remove(sageappid);
			procMap.remove(sageappid);
		}
		else if (line.contains(windowChange)) {
			sscanf(buff, "%c %s %c %s %s %s %d %d %d %d %d %d %d %d %d %d %s %d %d %d", &dummyChar, ts, &dummyChar, dummy, dummy, dummy, &sageappid, &left, &right, &bottom, &top, &value, &zvalue, &value, &value, &value, dummy, &resx, &resy, &value);

			width = right - left;
			height = top - bottom;

			BaseWidget *bw = 0;

			if ( appMap.find(sageappid) == appMap.end() ) {
				// app hasn't created
				QProcess *process = new QProcess;
				//process->start("ssh -xf 127.0.0.1 \"cd $HOME/media/video;$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0 HansRosling_2006_480.mp4\"");
				QString cmd("ssh -xf ");
				cmd.append(sender);
				cmd.append(" ");
				cmd.append(program);
				cmd.append(" 0 ");
				cmd.append(QString::number(resx));
				cmd.append(" ");
				cmd.append(QString::number(resy));
				cmd.append(" ");
				cmd.append(QString::number(fps)); // framerate

				qDebug() << linenumber << ") -------------invoking sage app";
				process->start(cmd);

				while ( !sageWidgetInfo.bwPtr ) {
					thread()->yieldCurrentThread();
					usleep(200*1000);
				}
				bw = sageWidgetInfo.bwPtr;
				Q_ASSERT(bw);
				appMap.insert(sageappid, bw);
				procMap.insert(sageappid, process);

				sageWidgetInfo.bwPtr = 0; // reset
//				sageWidgetInfo.gaid = -1; // reset
				qDebug() << linenumber << ") --------------sage app started";
			}
			else {
				// is now being interacted. manually update lastTouch
				bw = appMap.value(sageappid);
				bw->setLastTouch();
			}

//			qDebug("%llu) CHANGE : App id %d, WxH (%d x %d), native (%d x %d), scale %.2f", linenumber, sageappid,width,height,resx,resy, (qreal)width / (qreal)resx);

			// change geometry ( window move, window resize )
			bw->setPos(left, bottom);
			bw->setScale((qreal)width / (qreal)resx);
		}
		else if (line.contains(windowZ)) {
			QStringList strlist = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

			int offset = 6;
			int numapp = strlist.at(5).toInt();
			for (int i=0; i<numapp; ++i) {
				int index = i * 2 + offset;
				int appid = strlist.at(index).toInt();
				if ( appMap.find(appid) != appMap.end() ) {
					BaseWidget *bw = appMap.value(appid);
					Q_ASSERT(bw);
					qreal z = strlist.at(index+1).toDouble();
					if ( z == 0 ) {
						bw->setZValue( 9999 );
					}
					else {
						bw->setZValue( 1.0 / z);
					}
				}
			}
		}
		else {
			// do nothing
		}
	}

	qint64 simulend = 0;
#if QT_VERSION >= 0x040700
	simulend = QDateTime::currentMSecsSinceEpoch();
#else
#endif

	QProcess process;
	process.start("ssh -xf 127.0.0.1 killall -9 checker");
	process.waitForFinished();
	qDebug() << "\n\n------------ Scenario finished in" << (simulend - simulstart) / 1000 << "sec. TimeShrink was" << timeShrink << "Total" << linenumber << "lines.\n\n";
}

void GraphicsViewMain::writePrelimDataHeader(const QString str) {
	prelimDataFile.write(qPrintable(str));
}

