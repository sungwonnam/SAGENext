#include "sagenextlauncher.h"
#include "sagenextscene.h"

#include "common/commonitem.h"

#include "sage/fsManager.h"

#include "applications/base/basewidget.h"
#include "applications/base/appinfo.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/affinityinfo.h"

#include "applications/pixmapwidget.h"
#include "applications/vncwidget.h"
#include "applications/webwidget.h"
#include "applications/sagestreamwidget.h"
#include "applications/base/dummyplugininterface.h"

#include "system/resourcemonitor.h"

#include "sage/fsmanagermsgthread.h"




SAGENextLauncher::SAGENextLauncher(const QSettings *s, SAGENextScene *scene,  ResourceMonitor *rm /*= 0*/, SchedulerControl *sc /* = 0*/,QObject *parent /*0*/) :
	QObject(parent),
	_settings(s),
	_globalAppId(1),
	_scene(scene),
	_rMonitor(rm),
	_schedCtrl(sc)
{
	Q_ASSERT(_settings);
	Q_ASSERT(_scene);

	// start listening for sage message
	createFsManager();
}

SAGENextLauncher::~SAGENextLauncher() {

	/* kills fsManager */
	if (_fsm) _fsm->close(); _fsm->deleteLater();

	qDebug("%s::%s()" , metaObject()->className(), __FUNCTION__);
}



void SAGENextLauncher::createFsManager() {
	/**
	  QTcpServer::listen() will be called in constructor
	  */
	_fsm = new fsManager(_settings, this);
//	connect(_fsm, SIGNAL(sailConnected(const quint64, QString, int, int, const QRect)), this, SLOT(startSageApp(const quint64,QString, int, int, const QRect)));
	connect(_fsm, SIGNAL(incomingSail(fsManagerMsgThread*)), this, SLOT(launch(fsManagerMsgThread*)));
}

/**
  public slot
  */
void SAGENextLauncher::launch(fsManagerMsgThread *fsmThread) {
	SageStreamWidget *sw = 0;

	if (_sageWidgetQueue.isEmpty()) {
		// create new sageWidget
		sw = new SageStreamWidget("", _globalAppId, _settings, "127.0.0.1", _rMonitor); // 127.0.0.1 ??????????
	}
	else {
		// there's sageWidget waiting for SAIL connection
		sw = _sageWidgetQueue.front();
		_sageWidgetQueue.pop_front();
	}


	// give fsmThread to the sagewidget
	if (sw) {
		fsmThread->setSageWidget(sw);
		sw->setFsmMsgThread(fsmThread);
		// now fsmThread has been started !

		QObject::connect(sw, SIGNAL(destroyed()), fsmThread, SLOT(sendSailShutdownMsg()));
	}


	/*!
	  Resource monitor & processor assignment
	  */
	if (sw && sw->affInfo() && _rMonitor)
	{
		QObject::connect(sw->affInfo(), SIGNAL(cpuOfMineChanged(RailawareWidget *,int,int)), _rMonitor, SLOT(updateAffInfo(RailawareWidget *,int,int)));

		// Below won't be necessary because BaseWidget::fadeOutClose() takes care widget removal from resource monitor
		// It's moved to fadeOutClose() because removal has to be handled early
		//connect(sageWidget->getAffInfoPtr(), SIGNAL(destroyed(QObject*)), resourceMonitor, SLOT(removeApp(QObject *)));

		/*******
		 This is used to send affinity info to sender (sail) so that sender can set proper affinity based on receivers affinity setting. This should be connected only when mplayer is run localy
		 *********/
		QObject::connect(sw->affInfo(), SIGNAL(streamerAffInfoChanged(AffinityInfo*, quint64)), fsmThread, SLOT(sendSailSetRailMsg(AffinityInfo*,quint64)));

		// assign most underloaded processor
//		resourceMonitor->assignProcessor(sageWidget);

		// update QList<RailawareWidget *> ResourceMonitor::widgetList
		_rMonitor->addSchedulableWidget(sw);
//		resourceMonitor->updateAffInfo(sageWidget, -1, -1);
	}

	launch(sw);
}

QGraphicsItem * SAGENextLauncher::createPointer(quint64 uiclientid, QColor c, QString pointername) {
	PolygonArrow *pa = new PolygonArrow(uiclientid, _settings, c);

	if ( !pointername.isNull() && !pointername.isEmpty())
		pa->setPointerName(pointername);

	_scene->addItem(pa);
	return pa;
}

/**
  * UiServer triggers this slot
  */
void SAGENextLauncher::launch(int type, QString filename, qint64 fsize /* 0 */, QString senderIP /* 127.0.0.1 */, QString recvIP /* "" */, quint16 recvPort /* 0 */) {
	qDebug("%s::%s() : filesize %lld, senderIP %s, recvIP %s, recvPort %hd", metaObject()->className(), __FUNCTION__, fsize, qPrintable(senderIP), qPrintable(recvIP), recvPort);

	BaseWidget *w = 0;
	switch(type) {
	case MEDIA_TYPE_IMAGE: {

		// streaming from ui client
		if ( fsize > 0 && !recvIP.isEmpty() && recvPort > 0) {

			// fire file receiving function
//			QFuture<bool> future = QtConcurrent::run(this, &SAGENextLauncher::fileReceivingFunction, type, filename, fsize, senderIP, recvIP, recvPort);

			w = new PixmapWidget(fsize, senderIP, recvIP, recvPort, _globalAppId, _settings);
		}

		// from local storage
		else if ( !filename.isEmpty() ) {
			qDebug() << "\n\n\n" << filename << _globalAppId;
			w = new PixmapWidget(filename, _globalAppId, _settings);
		}
		else
			qCritical("%s() : MEDIA_TYPE_IMAGE can't open", __FUNCTION__);

		break;
	}

	case MEDIA_TYPE_VIDEO: {

		// the file has to be downloaded first


		SageStreamWidget *sws = new SageStreamWidget(filename, _globalAppId, _settings, senderIP, _rMonitor);
		w = sws;
		_sageWidgetQueue.push_back(sws);

		// invoke sail remotely
		QProcess *proc1 = new QProcess(this);
		QStringList args1;
//		args1 << "-xf" << senderIP << "\"cd $HOME/.sageConfig;$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0\"";
		args1 << "-xf" << senderIP << "$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0" << filename;

		// this will invoke sail (outside of SAGENext)
		// _globalAppId shouldn't be incremented in here because StartSageApp() will increment eventually
		// Also SageStreamWidget will be added to the scene in there
		proc1->start("ssh",  args1);

		break;
	}


	case MEDIA_TYPE_LOCAL_VIDEO: {

		if ( ! filename.isEmpty() ) {
			/**
			  create sageWidget
			  */
			SageStreamWidget *sws = new SageStreamWidget(filename, _globalAppId, _settings, "127.0.0.1", _rMonitor);
			w = sws;

			/**
			  add the widget to the queue
			  */
			_sageWidgetQueue.push_back(sws);

			/**
			  initiate SAIL process
			  */
			QProcess *proc = new QProcess(this);
			QStringList args;
			args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-identify" << filename;

			// this will invoke sail (outside of SAGENext)
			// _globalAppId shouldn't be incremented in here because StartSageApp() will increment eventually
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
			w->setSettings(_settings);
			w->setGlobalAppId(_globalAppId);
		}
		/*
		  A plugin inherits QWidget
		  */
		else {
			w = new BaseWidget(_globalAppId, _settings, _rMonitor);
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

	launch(w);
}

void SAGENextLauncher::launch(QString vncPasswd, int display, QString vncServerIP, int framerate) {
	BaseWidget *w = new VNCClientWidget(_globalAppId, vncServerIP, display, vncPasswd, framerate, _settings, _rMonitor);
	launch(w);
}

void SAGENextLauncher::launch(BaseWidget *w) {
	if ( w ) {
		_scene->addItem(w);
//		connect(this, SIGNAL(showInfo()), w, SLOT(drawInfo()));
		++_globalAppId; // increment only when widget is created successfully
		w->setTopmost();
	}
	else {
		qCritical("%s::%s() : Couldn't create an widget", metaObject()->className(), __FUNCTION__);
	}
}



void SAGENextLauncher::fileReceivingFunction(int mediatype, QString filename, qint64 filesize, QString senderIP, QString recvIP, quint16 recvPort) {

	// download file

	if ( mediatype == MEDIA_TYPE_IMAGE) {
		QMetaObject::invokeMethod(this, "launch", Qt::QueuedConnection
								  , Q_ARG(int, MEDIA_TYPE_IMAGE)
								  , Q_ARG(QString, QDir::homePath().append("/image").append(filename))
								  );
	}
	else if (mediatype == MEDIA_TYPE_VIDEO) {
		QMetaObject::invokeMethod(this, "launch", Qt::QueuedConnection
								  , Q_ARG(int, MEDIA_TYPE_LOCAL_VIDEO)
								  , Q_ARG(QString, QDir::homePath().append("/video").append(filename))
								  );
	}
}










