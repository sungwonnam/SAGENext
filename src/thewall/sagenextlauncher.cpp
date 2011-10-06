#include "sagenextlauncher.h"
#include "sagenextscene.h"
#include "mediastorage.h"

#include "common/commonitem.h"

#include "sage/fsManager.h"
#include "sage/fsmanagermsgthread.h"

#include "system/resourcemonitor.h"

#include "applications/base/basewidget.h"
#include "applications/base/appinfo.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/affinityinfo.h"

#include "applications/pdfviewerwidget.h"
#include "applications/pixmapwidget.h"
#include "applications/vncwidget.h"
#include "applications/webwidget.h"
#include "applications/sagestreamwidget.h"
#include "applications/base/SN_plugininterface.h"


SN_Launcher::SN_Launcher(const QSettings *s, SN_TheScene *scene, SN_MediaStorage *mediaStorage, SN_ResourceMonitor *rm /*= 0*/, SN_SchedulerControl *sc /* = 0*/, QFile *scenarioFile, QObject *parent /*0*/)
	: QObject(parent)
	, _settings(s)
	, _globalAppId(1)
	, _scene(scene)
	, _mediaStorage(mediaStorage)
	, _rMonitor(rm)
	, _schedCtrl(sc)
	, _scenarioFile(scenarioFile)
{
	Q_ASSERT(_settings);
	Q_ASSERT(_scene);

	loadPlugins();

	// start listening for sage message
	createFsManager();
}

SN_Launcher::~SN_Launcher() {
	/* kills fsManager */
	if (_fsm) _fsm->close(); _fsm->deleteLater();

	qDebug("%s::%s()" , metaObject()->className(), __FUNCTION__);
}

void SN_Launcher::loadPlugins() {

	QDir pluginDir(QDir::homePath() + "/.sagenext/plugins");

	foreach (QString filename, pluginDir.entryList(QDir::Files)) {
		QPluginLoader loader(pluginDir.absoluteFilePath(filename));
		QObject *plugin = loader.instance();
		if (plugin) {
			SN_PluginInterface *dpi = qobject_cast<SN_PluginInterface *>(plugin);
			if (!dpi) {
				qCritical() << "qobject_cast<SN_PluginInterface *>(plugin) failed for" << filename;
			}
			else {
				qDebug() << filename << "has added";
				_pluginMap.insert(pluginDir.absoluteFilePath(filename), dpi);
			}
		}
		else {
			qWarning("%s::%s() : %s", metaObject()->className(), __FUNCTION__, qPrintable(loader.errorString()));
		}
	}
}


void SN_Launcher::createFsManager() {
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
SN_BaseWidget * SN_Launcher::launch(fsManagerMsgThread *fsmThread) {
        SN_SageStreamWidget *sw = 0;
		QPointF scenepos;

        if (_sageWidgetQueue.isEmpty()) {
			// This means the SAGE application was NOT started by the launcher but manually by a user.
			// For instance, running mplayer in console terminal will make fsManager to emit incomingSail(fsmThread *)
			// which is connected to this slot

                // create new sageWidget
                sw = new SN_SageStreamWidget("", _globalAppId, _settings, "127.0.0.1", _rMonitor); // 127.0.0.1 ??????????
        }
        else {
			// This means the SAGE application was started by the launcher.
			// For instances, through mediaBrowser, or drag&drop on sageNextPointer's drop frame

                // there's sageWidget waiting for SAIL connection
                sw = _sageWidgetQueue.front();
                _sageWidgetQueue.pop_front();

				scenepos = _sageWidgetScenePosQueue.front();
				_sageWidgetScenePosQueue.pop_front();
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
                QObject::connect(sw->affInfo(), SIGNAL(cpuOfMineChanged(SN_RailawareWidget *,int,int)), _rMonitor, SLOT(updateAffInfo(SN_RailawareWidget *,int,int)));

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

        return launch(sw, scenepos);
}


/**
  * UiServer triggers this slot
  */
SN_BaseWidget * SN_Launcher::launch(int type, QString filename, const QPointF &scenepos/* = QPointF()*/, qint64 fsize /* 0 */, QString senderIP /* 127.0.0.1 */, QString recvIP /* "" */, quint16 recvPort /* 0 */) {
	//qDebug("%s::%s() : filesize %lld, senderIP %s, recvIP %s, recvPort %hd", metaObject()->className(), __FUNCTION__, fsize, qPrintable(senderIP), qPrintable(recvIP), recvPort);

	qDebug() << "Launcher::launch() :" << type << filename << scenepos;



	//
	// record event (WIDGET_NEW or POINTER_NEW)
	//
	if (_scenarioFile  &&  _settings->value("misc/record_launcher", false).toBool()) {
		if ( _scenarioFile->isOpen() && _scenarioFile->isWritable() ) {
			char record[256];
			sprintf(record, "%lld %d %d %s\n",QDateTime::currentMSecsSinceEpoch(), 0, (int)type, qPrintable(filename));
			_scenarioFile->write(record);
		}
		else {
			qDebug() << "Launcher::launch() : can't write the launching event";
		}
	}





	SN_BaseWidget *w = 0;
	switch(type) {
	case SAGENext::MEDIA_TYPE_IMAGE: {
		//
		// streaming from ui client
		//
		if ( fsize > 0 && !recvIP.isEmpty() && recvPort > 0) {

			// fire file receiving function
			//QFuture<bool> future = QtConcurrent::run(this, &SAGENextLauncher::fileReceivingFunction, type, filename, fsize, senderIP, recvIP, recvPort);

			w = new SN_PixmapWidget(fsize, senderIP, recvIP, recvPort, _globalAppId, _settings);
			_mediaStorage->insertNewMediaToHash(filename);
		}

		//
		// from local storage
		//
		else if ( !filename.isEmpty() ) {

//			qDebug("%s::%s() : MEDIA_TYPE_IMAGE %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
			w = new SN_PixmapWidget(filename, _globalAppId, _settings);
			_mediaStorage->insertNewMediaToHash(filename);
		}
		else
			qCritical("%s::%s() : MEDIA_TYPE_IMAGE can't open", metaObject()->className(), __FUNCTION__);

		break;
	}

	case SAGENext::MEDIA_TYPE_VIDEO: {

		// Assumes that the remote side (senderIP) has maplyer (compiled with SAIL) already

		if (!filename.isEmpty() && !senderIP.isEmpty()) {
			SN_SageStreamWidget *sws = new SN_SageStreamWidget(filename, _globalAppId, _settings, senderIP, _rMonitor);
			sws->appInfo()->setFileInfo(filename);
			w = sws;

			//
			// add widget to the queue for launch(fsmThread *)
			//
			_sageWidgetQueue.push_back(sws);
			_sageWidgetScenePosQueue.push_back(scenepos);

			// invoke sail remotely
			QProcess *proc1 = new QProcess(this);
			QStringList args1;
			//	args1 << "-xf" << senderIP << "\"cd $HOME/.sageConfig;$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0\"";
			args1 << "-xf" << senderIP << "$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0" << filename;

			sws->appInfo()->setCmdArgs(args1);

			//
			// this will invoke sail (outside of SAGENext) which will trigger fsManager::incomingSail(fsmThread *) signal which is connected to launch(fsmThread *)
			//
			proc1->start("ssh",  args1);
			sws->appInfo()->setExecutableName("ssh");
			sws->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_VIDEO);
			sws->setSailAppProc(proc1);

			//
			///
			//// launch(w) will be called in launch(fsmMessageThread *)
			///  Because of this, scenepos in savedSession has no effect !!!
			//
			return sws;
		}
		break;
	}


	case SAGENext::MEDIA_TYPE_LOCAL_VIDEO: {
		qDebug("%s::%s() : MEDIA_TYPE_LOCAL_VIDEO %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));

		if ( ! filename.isEmpty() ) {
			/**
			  create sageWidget
			*/
			SN_SageStreamWidget *sws = new SN_SageStreamWidget(filename, _globalAppId, _settings, "127.0.0.1", _rMonitor);
			sws->appInfo()->setFileInfo(filename);
			w = sws;

			/**
		      add the widget to the queue
			*/
			_sageWidgetQueue.push_back(sws);
			_sageWidgetScenePosQueue.push_back(scenepos);

			/**
	          initiate SAIL process
			*/
			QProcess *proc = new QProcess(this);
//			proc->setWorkingDirectory("$SAGE_DIRECTORY");
//			proc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

			QStringList args;
			args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-identify" << filename;

			sws->appInfo()->setCmdArgs(args);

			//
			// this will invoke sail (outside of SAGENext) which will trigger fsManager::incomingSail(fsmThread *) signal which is connected to launch(fsmThread *)
			//
//			qDebug() << proc->environment();

			proc->start("mplayer",  args);

			sws->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_LOCAL_VIDEO);
			sws->appInfo()->setExecutableName("mplayer");
			sws->setSailAppProc(proc);


			//
			///
			//// launch(w) will be called in launch(fsmMessageThread *)
			//// Because of this, scenepos in savedSession has no effect !!! That's why _sageWidgetScenePosQueue has to be maintained
			//
			return sws;
		}

		break;
	}

	case SAGENext::MEDIA_TYPE_WEBURL: {
		// filename is url string
		SN_WebWidget *ww = new SN_WebWidget(_globalAppId, _settings, 0, Qt::Window);
		w = ww;
		ww->setUrl( filename );
		break;
	}

	case SAGENext::MEDIA_TYPE_PDF: {
		SN_PDFViewerWidget *pdfviewer = new SN_PDFViewerWidget(filename, _globalAppId, _settings, 0, Qt::Widget);
		w = pdfviewer;
		break;
	}

	case SAGENext::MEDIA_TYPE_AUDIO: {
		break;
	}
	case SAGENext::MEDIA_TYPE_UNKNOWN: {
		break;
	}


	case SAGENext::MEDIA_TYPE_PLUGIN: {
//		qDebug() << "MEDIA_TYPE_PLUGIN" << filename;
		SN_PluginInterface *dpi = _pluginMap.value(filename);
		if (dpi) {
			w = dpi->createInstance();

			qDebug() << w;
			w->setSettings(_settings);
			w->setGlobalAppId(_globalAppId);
			w->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_PLUGIN);
			w->appInfo()->setFileInfo(filename);
		}
		else {
			qDebug() << "dpi is null";
		}

		break;
	} // end MEDIA_TYPE_PLUGIN

	} // end switch

	return launch(w, scenepos);
}

SN_BaseWidget * SN_Launcher::launch(QString username, QString vncPasswd, int display, QString vncServerIP, int framerate, const QPointF &scenepos /*= QPointF()*/) {
	//	qDebug() << "launch" << username << vncPasswd;
	SN_BaseWidget *w = new SN_VNCClientWidget(_globalAppId, vncServerIP, display, username, vncPasswd, framerate, _settings);
	return launch(w, scenepos);
}

SN_BaseWidget * SN_Launcher::launch(SN_BaseWidget *w, const QPointF &scenepos /*= QPointF()*/) {
	if ( w ) {

		/**
		  Without SAGENextLayoutWidget, applications are added to the scene directly
		  */
//		_scene->addItem(w);

		if ( w->isRegisteredForMouseHover() ) {
			_scene->hoverAcceptingApps.push_back(w);
		}
		_scene->addItemOnTheLayout(w, scenepos);

		//connect(this, SIGNAL(showInfo()), w, SLOT(drawInfo()));
		++_globalAppId; // increment only when widget is created successfully
		w->setTopmost();
	}
	else {
		qCritical("%s::%s() : Couldn't create an widget", metaObject()->className(), __FUNCTION__);
	}
	return w;
}

SN_BaseWidget * SN_Launcher::launch(const QStringList &fileList) {
	if ( fileList.empty() ) {
		return 0;
	}

	QRegExp rxVideo("\\.(avi|mov|mpg|mpeg|mp4|mkv|flv|wmv)$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxImage("\\.(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxPdf("\\.(pdf)$", Qt::CaseInsensitive, QRegExp::RegExp);
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

	QRegExp rxSession("\\.session$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxScenario("\\.recording", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxRatkoData("\\.log", Qt::CaseInsensitive, QRegExp::RegExp);

	for ( int i=0; i<fileList.size(); i++ ) {
		//qDebug("GraphicsViewMain::%s() : %d, %s", __FUNCTION__, i, qPrintable(filenames.at(i)));
		QString filename = fileList.at(i);

		if ( filename.contains(rxVideo) ) {
//			qDebug("%s::%s() : Opening a video file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));

//			w = new PhononWidget(filename, globalAppId, settings);
//			QFuture<void> future = QtConcurrent::run(qobject_cast<PhononWidget *>(w), &PhononWidget::threadInit, filename);

			return launch((int)SAGENext::MEDIA_TYPE_LOCAL_VIDEO, filename);
		}

		/*!
		  Image
		  */
		else if ( filename.contains(rxImage) ) {
//			qDebug("%s::%s() : Opening an image file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
			return launch((int)SAGENext::MEDIA_TYPE_IMAGE, filename);
		}

		/*!
		  PDF
		  */
		else if ( filename.contains(rxPdf) ) {
//			qDebug("%s::%s() : Opening a PDF file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
			return launch((int)SAGENext::MEDIA_TYPE_PDF, filename);
		}

		/**
		  * plugin
		  */
		else if (filename.contains(rxPlugin) ) {
//			qDebug("%s::%s() : Loading a plugin %s", metaObject()->className(),__FUNCTION__, qPrintable(filename));
			return launch((int)SAGENext::MEDIA_TYPE_PLUGIN, filename);
		}

		/*!
		  session
		  */
		else if (filename.contains(rxSession) ) {
//			qDebug("%s::%s() : Loading a session %s", metaObject()->className(),__FUNCTION__, qPrintable(filename));
			launchSavedSession(filename);
			return 0;
		}

		/**
		  Recording
		  */
		else if (filename.contains(rxScenario)) {
//			qDebug("%s::%s() : Launching a recording file, %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
			launchRecording(filename);
			return 0;
		}

		/*!
		  Ratko data
		  */
		else if (filename.contains(rxRatkoData) ) {
			//loadSavedScenario(filename);
			/*
			QFuture<void> future = QtConcurrent::run(this, &GraphicsViewMain::loadSavedScenario, filename);
			*/
//			RatkoDataSimulator *rdsThread = new RatkoDataSimulator(filename);
//			rdsThread->start();
		}

		else if ( QDir(filename).exists() ) {
			qDebug("%s::%s() : DIRECTORY", metaObject()->className(), __FUNCTION__);
			return launch(QDir(filename).entryList());
		}
		else {
			qCritical("%s::%s() : Unrecognized file format", metaObject()->className(),__FUNCTION__);
		}
	}
	return 0;
}

SN_PolygonArrowPointer * SN_Launcher::launchPointer(quint32 uiclientid, const QString &name, const QColor &color, const QPointF &scenepos /*= QPointF()*/) {

	SN_PolygonArrowPointer *pointer = 0;

	//
	// record NEW_POINTER
	//
	if (_scenarioFile  &&  _settings->value("misc/record_launcher", false).toBool()) {
		if ( _scenarioFile->isOpen() && _scenarioFile->isWritable() ) {
			char record[256];
			sprintf(record, "%lld %d %u %s %s\n",QDateTime::currentMSecsSinceEpoch(), 1, uiclientid, qPrintable(name), qPrintable(color.name()));
			_scenarioFile->write(record);

			pointer = new SN_PolygonArrowPointer(uiclientid, _settings, name, color, _scenarioFile);
		}
		else {
			qDebug() << "Launcher::launchPointer() : Can't write";
		}
	}
	else {
		pointer = new SN_PolygonArrowPointer(uiclientid, _settings, name, color);
	}

	_scene->addItem(pointer);
	if(!scenepos.isNull()) {
		pointer->setPos(scenepos);
	}

	//
	// temporary for scenario
	//
	_pointerMap.insert(uiclientid, pointer);

	return pointer;
}

void SN_Launcher::launchSavedSession(const QString &sessionfilename) {
	QFile f(sessionfilename);
	if (!f.exists()) {
		qDebug() << "launch session: file not exist" << sessionfilename;
		return;
	}
	if(!f.open(QIODevice::ReadOnly)) {
		qDebug() << "launch session: can't open file" << sessionfilename;
		return;
	}

	qDebug() << "SAGENextLauncher::launchSavedSession() : Loading a session" << sessionfilename;

	QDataStream in(&f);

	int mtype;
	QPointF scenepos;
	QSizeF size;
	qreal scale;
	while (!f.atEnd()) {
		in >> mtype >> scenepos >> size >> scale;
//		qDebug() << "\tentry : " << mtype << scenepos << size << scale;

		QString file;
		QString user;
		QString pass;
		QString srcaddr;

		SN_BaseWidget *bw = 0;

		if (mtype == SAGENext::MEDIA_TYPE_VNC) {
			in >> srcaddr >> user >> pass;
			bw = launch(user, pass, 0, srcaddr, 10, scenepos);
		}
		else {
			in >> file;
			bw = launch(mtype, file, scenepos);
		}
		if (!bw) {
			qDebug() << "Error : can't launch this entry from the session file" << mtype << file << srcaddr << user << pass << scenepos << size << scale;
			continue;
		}

		bw->resize(size);
		bw->setScale(scale);
	}

	f.close();
}

void SN_Launcher::launchRecording(const QString &scenarioFilename) {
	ScenarioThread *thread = new ScenarioThread(this, scenarioFilename);
	qDebug() << "\nSAGENextLauncher::launchScenario() : START  " << scenarioFilename;
	thread->start();
}















ScenarioThread::ScenarioThread(SN_Launcher *launcher, const QString &file, QObject *parent)
	: QThread(parent)
	, _launcher(launcher)
{
	_scenarioFile.setFileName(file);
}
ScenarioThread::~ScenarioThread()
{
	wait();
}

void ScenarioThread::run() {
	Q_ASSERT(_launcher);
	if (!_scenarioFile.exists()) {
		qDebug() << _scenarioFile.fileName() << "doesn't exist";
		return;
	}

	if (!_scenarioFile.open(QIODevice::ReadOnly)) {
		qDebug() << "can't open" << _scenarioFile.fileName();
		return;
	}

	// the very first line contains start time
	char line[64];
	qint64 read = _scenarioFile.readLine(line, 64);
	qint64 starttime;
	sscanf(line, "%lld", &starttime);

	qint64 offset = QDateTime::currentMSecsSinceEpoch() - starttime;
	qDebug() << "Playback a recording:" << _scenarioFile.fileName() << ", Time offset:" << offset << "msec";

	forever {
		char line[512];
		qint64 read = _scenarioFile.readLine(line, 512);

		if (read == 0) {
			qDebug() << "scenario thread : readLine returned" << read;
			break;
		}
		else if (read < 0) {
			qDebug() << "scenario thread : readLine returned" << read;
			break;
		}

		qint64 when;
		int type;
		sscanf(line, "%lld %d", &when, &type);

		qint64 sleep = (when + offset) - QDateTime::currentMSecsSinceEpoch();
		if (sleep > 0) QThread::msleep((unsigned long)sleep);


		quint32 pointerid;
		int x,y;
		int button;
		SN_PolygonArrowPointer *pointer = 0;



		switch(type) {

		case 0: {
			int mtype;
			char filename[256];
			sscanf(line, "%lld %d %d %s", &when, &type, &mtype, filename);
//			qDebug() << "NEW_WIDGET" << mtype << filename;
//			_launcher->launch(mtype, QString(filename));
			QMetaObject::invokeMethod(_launcher, "launch", Qt::QueuedConnection, Q_ARG(int, mtype), Q_ARG(QString, QString(filename)));
			break;
		}
		case 1: {
			quint32 uiclientid;
			char pname[128];
			char color[16];
			sscanf(line, "%lld %d %u %s %s", &when, &type, &uiclientid, pname, color);
//			qDebug() << "NEW_POINTER" << uiclientid << pname << color;
			_launcher->launchPointer(uiclientid, QString(pname), QColor(QString(color)));
//			QMetaObject::invokeMethod(_launcher, "launch", Qt::QueuedConnection, Q_ARG(quint64, uiclientid), Q_ARG(QString, QString(pname)), Q_ARG(QColor, QColor(QString(color))));
			break;
		}
		case 11: { // POINTER_UNSHARE
			sscanf(line, "%lld %d %u", &when, &type, &pointerid);
			pointer = _launcher->_pointerMap.value(pointerid);
			if (pointer) {
				qDebug() << "POINTER_UNSHARE" << pointer->id() << pointer->name();
				_launcher->_pointerMap.remove(pointerid);
				delete pointer;
			}
			break;
		}
		case 2: // move
		case 3: // press
		case 4: // release
		case 5: // click
		case 6: // dbl click
		case 7: // wheel
		{
			sscanf(line, "%lld %d %u %d %d %d", &when, &type, &pointerid, &x, &y, &button);
			pointer = _launcher->_pointerMap.value(pointerid);
			Q_ASSERT(pointer);

			Qt::MouseButton btn = Qt::NoButton;
			if (button == 1) btn = Qt::LeftButton;
			else if (button == 2) btn = Qt::RightButton;

			Qt::MouseButtons btnflag = btn | Qt::NoButton;

			pointer->pointerOperation(type-2, QPointF(x,y), btn, button, btnflag);

			break;
		}

		} // end of switch
	}

	qDebug() << "Scenario Thread finished";
}


