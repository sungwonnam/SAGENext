#include "sagenextlauncher.h"
#include "sagenextscene.h"

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
#include "applications/base/dummyplugininterface.h"


SAGENextLauncher::SAGENextLauncher(const QSettings *s, SAGENextScene *scene, ResourceMonitor *rm /*= 0*/, SchedulerControl *sc /* = 0*/, QFile *scenarioFile, QObject *parent /*0*/)
	: QObject(parent)
	, _settings(s)
	, _globalAppId(1)
	, _scene(scene)
	, _rMonitor(rm)
	, _schedCtrl(sc)
	, _scenarioFile(scenarioFile)
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
BaseWidget * SAGENextLauncher::launch(fsManagerMsgThread *fsmThread) {
        SageStreamWidget *sw = 0;

        if (_sageWidgetQueue.isEmpty()) {
			// This means the SAGE application was NOT started by the launcher but manually by a user.
			// For instance, running mplayer in console terminal will make fsManager to emit incomingSail(fsmThread *)
			// which is connected to this slot

                // create new sageWidget
                sw = new SageStreamWidget("", _globalAppId, _settings, "127.0.0.1", _rMonitor); // 127.0.0.1 ??????????
        }
        else {
			// This means the SAGE application was started by the launcher.
			// For instances, through mediaBrowser, or drag&drop on sageNextPointer's drop frame

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

        return launch(sw);
}


/**
  * UiServer triggers this slot
  */
BaseWidget * SAGENextLauncher::launch(int type, QString filename, qint64 fsize /* 0 */, QString senderIP /* 127.0.0.1 */, QString recvIP /* "" */, quint16 recvPort /* 0 */) {
	//        qDebug("%s::%s() : filesize %lld, senderIP %s, recvIP %s, recvPort %hd", metaObject()->className(), __FUNCTION__, fsize, qPrintable(senderIP), qPrintable(recvIP), recvPort);




	//
	// record event
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





	BaseWidget *w = 0;
	switch(type) {
	case MEDIA_TYPE_IMAGE: {
		//
		// streaming from ui client
		//
		if ( fsize > 0 && !recvIP.isEmpty() && recvPort > 0) {

			// fire file receiving function
			//QFuture<bool> future = QtConcurrent::run(this, &SAGENextLauncher::fileReceivingFunction, type, filename, fsize, senderIP, recvIP, recvPort);

			w = new PixmapWidget(fsize, senderIP, recvIP, recvPort, _globalAppId, _settings);
		}

		//
		// from local storage
		//
		else if ( !filename.isEmpty() ) {

			qDebug("%s::%s() : MEDIA_TYPE_IMAGE %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
			w = new PixmapWidget(filename, _globalAppId, _settings);
		}
		else
			qCritical("%s::%s() : MEDIA_TYPE_IMAGE can't open", metaObject()->className(), __FUNCTION__);

		break;
	}

	case MEDIA_TYPE_VIDEO: {

		// Assumes that the remote side (senderIP) has maplyer (compiled with SAIL) already

		if (!filename.isEmpty() && !senderIP.isEmpty()) {
			SageStreamWidget *sws = new SageStreamWidget(filename, _globalAppId, _settings, senderIP, _rMonitor);
			sws->appInfo()->setFileInfo(filename);
			w = sws;

			//
			// add widget to the queue for launch(fsmThread *)
			//
			_sageWidgetQueue.push_back(sws);

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
			sws->setSailAppProc(proc1);

			//
			///
			//// launch(w) will be called in launch(fsmMessageThread *)
			///
			//
			return sws;
		}
		break;
	}


	case MEDIA_TYPE_LOCAL_VIDEO: {
		qDebug("%s::%s() : MEDIA_TYPE_LOCAL_VIDEO %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));

		if ( ! filename.isEmpty() ) {
			/**
			  create sageWidget
			*/
			SageStreamWidget *sws = new SageStreamWidget(filename, _globalAppId, _settings, "127.0.0.1", _rMonitor);
			sws->appInfo()->setFileInfo(filename);
			w = sws;

			/**
		      add the widget to the queue
			*/
			_sageWidgetQueue.push_back(sws);

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

			sws->appInfo()->setExecutableName("mplayer");
			sws->setSailAppProc(proc);

			//
			///
			//// launch(w) will be called in launch(fsmMessageThread *)
			///
			//
			return sws;
		}

		break;
	}

	case MEDIA_TYPE_WEBURL: {
		// filename is url string
		WebWidget *ww = new WebWidget(_globalAppId, _settings, 0, Qt::Window);
		w = ww;
		ww->setUrl( filename );
		break;
	}

	case MEDIA_TYPE_PDF: {
		PDFViewerWidget *pdfviewer = new PDFViewerWidget(filename, _globalAppId, _settings, 0, Qt::Widget);
		w = pdfviewer;
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
			return 0;
		}
		DummyPluginInterface *dpi = qobject_cast<DummyPluginInterface *>(plugin);
		if (!dpi) {
			qCritical() << "qobject_cast<DummyPluginInterface *>(plugin) failed";
			return 0;
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
			w = new BaseWidget(_globalAppId, _settings);
			w->setProxyWidget(dpi->rootWidget()); // inefficient
			w->setWindowFlags(Qt::Window);
			w->setWindowFrameMargins(0, 0, 0, 0);
		}
		w->appInfo()->setMediaType(MEDIA_TYPE_PLUGIN);
		w->appInfo()->setFileInfo(filename);

		if (w) {
			w->moveBy(20, 50);
		}
		else {
			qDebug() << "Failed to create widget from plugin object";
		}
		break;
	} // end MEDIA_TYPE_PLUGIN

	} // end switch

	return launch(w);
}

BaseWidget * SAGENextLauncher::launch(QString username, QString vncPasswd, int display, QString vncServerIP, int framerate) {
	//	qDebug() << "launch" << username << vncPasswd;
	BaseWidget *w = new VNCClientWidget(_globalAppId, vncServerIP, display, username, vncPasswd, framerate, _settings);
	return launch(w);
}

BaseWidget * SAGENextLauncher::launch(BaseWidget *w) {
	if ( w ) {

		/**
		  Without SAGENextLayoutWidget, applications are added to the scene directly
		  */
//		_scene->addItem(w);

		if ( w->isRegisteredForMouseHover() ) {
			_scene->hoverAcceptingApps.push_back(w);
		}
		_scene->addItemOnTheLayout(w);

		//connect(this, SIGNAL(showInfo()), w, SLOT(drawInfo()));
		++_globalAppId; // increment only when widget is created successfully
		w->setTopmost();
	}
	else {
		qCritical("%s::%s() : Couldn't create an widget", metaObject()->className(), __FUNCTION__);
	}
	return w;
}

BaseWidget * SAGENextLauncher::launch(const QStringList &fileList) {
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
			qDebug("%s::%s() : Opening a video file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));

//			w = new PhononWidget(filename, globalAppId, settings);
//			QFuture<void> future = QtConcurrent::run(qobject_cast<PhononWidget *>(w), &PhononWidget::threadInit, filename);

			return launch((int)MEDIA_TYPE_LOCAL_VIDEO, filename);
		}

		/*!
		  Image
		  */
		else if ( filename.contains(rxImage) ) {
			qDebug("%s::%s() : Opening an image file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
			return launch((int)MEDIA_TYPE_IMAGE, filename);
		}

		/*!
		  PDF
		  */
		else if ( filename.contains(rxPdf) ) {
			qDebug("%s::%s() : Opening a PDF file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
			return launch((int)MEDIA_TYPE_PDF, filename);
		}

		/**
		  * plugin
		  */
		else if (filename.contains(rxPlugin) ) {
			qDebug("%s::%s() : Loading a plugin %s", metaObject()->className(),__FUNCTION__, qPrintable(filename));
			return launch((int)MEDIA_TYPE_PLUGIN, filename);
		}

		/*!
		  session
		  */
		else if (filename.contains(rxSession) ) {
			qDebug("%s::%s() : Loading a session %s", metaObject()->className(),__FUNCTION__, qPrintable(filename));
			launchSavedSession(filename);
			return 0;
		}

		/**
		  Recording
		  */
		else if (filename.contains(rxScenario)) {
			qDebug("%s::%s() : Launching a recording file, %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
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

SAGENextPolygonArrow * SAGENextLauncher::launchPointer(quint64 uiclientid, const QString &name, const QColor &color) {

	SAGENextPolygonArrow *pointer = 0;

	//
	// record NEW_POINTER
	//
	if (_scenarioFile  &&  _settings->value("misc/record_launcher", false).toBool()) {
		if ( _scenarioFile->isOpen() && _scenarioFile->isWritable() ) {
			char record[256];
			sprintf(record, "%lld %d %llu %s %s\n",QDateTime::currentMSecsSinceEpoch(), 1, uiclientid, qPrintable(name), qPrintable(color.name()));
			_scenarioFile->write(record);

			pointer = new SAGENextPolygonArrow(uiclientid, _settings, name, color, _scenarioFile);
		}
		else {
			qDebug() << "Launcher::launchPointer() : Can't write";
		}
	}
	else {
		pointer = new SAGENextPolygonArrow(uiclientid, _settings, name, color);
	}

	_scene->addItem(pointer);
	pointer->setScale(1.3);

	// temporary for scenario
	_pointerMap.insert(uiclientid, pointer);

	return pointer;
}

void SAGENextLauncher::launchSavedSession(const QString &sessionfilename) {
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

		QString file;
		QString user;
		QString pass;
		QString srcaddr;

		BaseWidget *bw = 0;

		if (mtype == MEDIA_TYPE_VNC) {
			in >> srcaddr >> user >> pass;
			bw = launch(user, pass, 0, srcaddr);
		}
		else {
			in >> file;
			bw = launch(mtype, file);
		}
		if (!bw) {
			qDebug() << "Error : can't launch this entry from the session file" << mtype << file << srcaddr << user << pass << scenepos << size << scale;
			continue;
		}

		bw->resize(size);
		bw->setScale(scale);

		_scene->rootLayoutWidget()->addItem(bw, scenepos);
	}

	f.close();
}

void SAGENextLauncher::launchRecording(const QString &scenarioFilename) {
	ScenarioThread *thread = new ScenarioThread(this, scenarioFilename);
	qDebug() << "\nSAGENextLauncher::launchScenario() : START  " << scenarioFilename;
	thread->start();
}















ScenarioThread::ScenarioThread(SAGENextLauncher *launcher, const QString &file, QObject *parent)
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
	qDebug() << "Starting scenario, Time offset:" << offset << "msec";

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


		quint64 pointerid;
		int x,y;
		int button;
		SAGENextPolygonArrow *pointer = 0;



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
			quint64 uiclientid;
			char pname[128];
			char color[16];
			sscanf(line, "%lld %d %llu %s %s", &when, &type, &uiclientid, pname, color);
//			qDebug() << "NEW_POINTER" << uiclientid << pname << color;
			_launcher->launchPointer(uiclientid, QString(pname), QColor(QString(color)));
//			QMetaObject::invokeMethod(_launcher, "launch", Qt::QueuedConnection, Q_ARG(quint64, uiclientid), Q_ARG(QString, QString(pname)), Q_ARG(QColor, QColor(QString(color))));
			break;
		}
		case 11: { // POINTER_UNSHARE
			sscanf(line, "%lld %d %llu", &when, &type, &pointerid);
			pointer = _launcher->_pointerMap.value(pointerid);
			delete pointer;
			break;
		}
		case 2: // move
		case 3: // press
		case 4: // release
		case 5: // click
		case 6: // dbl click
		case 7: // wheel
		{
			sscanf(line, "%lld %d %llu %d %d %d", &when, &type, &pointerid, &x, &y, &button);
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



