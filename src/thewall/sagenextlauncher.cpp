#include "sagenextlauncher.h"
#include "sagenextscene.h"
#include "mediastorage.h"

#include "common/commonitem.h"
#include "common/sn_sharedpointer.h"

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
#include "applications/sn_checker.h"

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

	qDebug() << "\nSN_Launcher : Loading plugins";
	loadPlugins();

	// start listening for sage message
	createFsManager();


//	QTimer::singleShot(1000, this, SLOT(runRatkoSlot()));
}

SN_Launcher::~SN_Launcher() {
	/* kills fsManager */
	if (_fsm) _fsm->close(); _fsm->deleteLater();

	qDebug("%s::%s()" , metaObject()->className(), __FUNCTION__);
}

void SN_Launcher::loadPlugins() {

	QDir pluginDir(QDir::homePath() + "/.sagenext/media/plugins");

	foreach (QString filename, pluginDir.entryList(QDir::Files)) {
		QPluginLoader loader(pluginDir.absoluteFilePath(filename));
		QObject *plugin = loader.instance();
		if (plugin) {
			SN_PluginInterface *dpi = qobject_cast<SN_PluginInterface *>(plugin);
			if (!dpi) {
				qCritical() << "qobject_cast<SN_PluginInterface *>(plugin) failed for" << filename;
			}
			else {
				qDebug() << "SN_Launcher::loadPlugins() : " << filename << "has added";
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
	QPointF pos;

	if (_sageWidgetQueue.isEmpty()) {
		// This means the SAGE application was NOT started by the launcher but manually by a user.
		// For instance, running mplayer in console terminal will make the fsManager to emit incomingSail(fsmThread *)
		// which is connected to this slot

		// create new sageWidget
		sw = new SN_SageStreamWidget("", _globalAppId++, _settings, "127.0.0.1", _rMonitor); // 127.0.0.1 ??????????
	}
	else {
		// This means the the SAGE application was started by the launcher.
		// For instances, through mediaBrowser, or drag&drop on sageNextPointer's drop frame.
		// So this is typical way to launch sage app

		// Therefore, there's already a SN_SageStreamWidget waiting for SAIL connection
		sw = _sageWidgetQueue.front();
		_sageWidgetQueue.pop_front();

		pos = _sageWidgetPosQueue.front();
		_sageWidgetPosQueue.pop_front();
	}

	// give fsmThread the sagewidget
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
		// Below won't be necessary because BaseWidget::fadeOutClose() takes care widget removal from resource monitor
		// It's moved to fadeOutClose() because removal has to be handled early
		//connect(sageWidget->getAffInfoPtr(), SIGNAL(destroyed(QObject*)), resourceMonitor, SLOT(removeApp(QObject *)));

		/*******
		  SAGE specific !!
		  This is used to send affinity info to sender (sail) so that sender can set proper affinity based on receivers affinity setting. This should be connected only when mplayer is run localy
		  *********/
		QObject::connect(sw->affInfo(), SIGNAL(streamerAffInfoChanged(AffinityInfo*, quint64)), fsmThread, SLOT(sendSailSetRailMsg(AffinityInfo*,quint64)));

		// assign most underloaded processor
		//resourceMonitor->assignProcessor(sageWidget);

		// update QList<RailawareWidget *> ResourceMonitor::widgetList
//		_rMonitor->addSchedulableWidget(sw);
		//resourceMonitor->updateAffInfo(sageWidget, -1, -1);
	}

	return launch(sw, pos);
}


/**
  * UiServer triggers this slot
  */
SN_BaseWidget * SN_Launcher::launch(int type, QString filename, const QPointF &scenepos/* = QPointF()*/, qint64 fsize /* 0 */, QString senderIP /* 127.0.0.1 */, QString recvIP /* "" */, quint16 recvPort /* 0 */) {
	//qDebug("%s::%s() : filesize %lld, senderIP %s, recvIP %s, recvPort %hd", metaObject()->className(), __FUNCTION__, fsize, qPrintable(senderIP), qPrintable(recvIP), recvPort);

//	qDebug() << "SN_Launcher::launch() :" << type << filename << scenepos;


	////////////////////////////////////////
	//
	// record event (WIDGET_NEW : 0)
	//
	if (_scenarioFile  &&  _settings->value("misc/record_launcher", false).toBool()) {
		if ( _scenarioFile->isOpen() && _scenarioFile->isWritable() ) {
			char record[256];
			sprintf(record, "%lld %d %d %s %d %d\n",QDateTime::currentMSecsSinceEpoch(), 0, (int)type, qPrintable(filename), scenepos.toPoint().x(), scenepos.toPoint().y());
			_scenarioFile->write(record);
		}
		else {
			qDebug() << "SN_Launcher::launch() : can't write the launching event";
		}
	}
	//
	//////////////////////////////////////////





	SN_BaseWidget *w = 0;
	switch(type) {
	case SAGENext::MEDIA_TYPE_IMAGE: {
		//
		// streaming from ui client. This isn't used.
		//
		if ( fsize > 0 && !recvIP.isEmpty() && recvPort > 0) {

			// fire file receiving function
			//QFuture<bool> future = QtConcurrent::run(this, &SAGENextLauncher::fileReceivingFunction, type, filename, fsize, senderIP, recvIP, recvPort);

			w = new SN_PixmapWidget(fsize, senderIP, recvIP, recvPort, _globalAppId++, _settings);

			if(_mediaStorage)
				_mediaStorage->insertNewMediaToHash(filename);
		}

		//
		// from local storage
		//
		else if ( !filename.isEmpty() ) {

//			qDebug("%s::%s() : MEDIA_TYPE_IMAGE %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
			w = new SN_PixmapWidget(filename, _globalAppId++, _settings);

			if(_mediaStorage)
				_mediaStorage->insertNewMediaToHash(filename);
		}
		else
			qCritical("%s::%s() : MEDIA_TYPE_IMAGE can't open", metaObject()->className(), __FUNCTION__);

		break;
	}

		//
		//
		// This doesn't work !!! Don't use it.
		//
		//
	case SAGENext::MEDIA_TYPE_VIDEO:
	{
		// Assumes that the remote side (senderIP) has maplyer (compiled with SAIL) already
		// So, this is the case where media file is not in local disk

		if (!filename.isEmpty() && !senderIP.isEmpty()) {
			SN_SageStreamWidget *sws = new SN_SageStreamWidget(filename, _globalAppId++, _settings, senderIP, _rMonitor);
			sws->appInfo()->setFileInfo(filename);
			w = sws;

			//
			// add widget to the queue for launch(fsmThread *)
			//
			_sageWidgetQueue.push_back(sws);
			_sageWidgetPosQueue.push_back(scenepos);

			// invoke sail remotely
			QProcess *proc1 = new QProcess(this);
			QStringList args1;
			//	args1 << "-xf" << senderIP << "\"cd $HOME/.sageConfig;$SAGE_DIRECTORY/bin/mplayer -vo sage -nosound -loop 0\"";
			args1 << "-xf" << senderIP << "$SAGE_DIRECTORY/bin/mplayer" <<  "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-sws" <<  "4" << filename;

			sws->appInfo()->setCmdArgs(args1);

			//
			// this will invoke sail (outside of SAGENext) which will trigger fsManager::incomingSail(fsmThread *) signal which is connected to launch(fsmThread *)
			//
			proc1->start("ssh",  args1);
			if (!proc1->waitForStarted(-1) ) {
				qDebug() << "Failed to start mplayer process" << proc1->workingDirectory();
				qDebug() << proc1->readAllStandardError();
				qDebug() << proc1->readAllStandardOutput();
			}
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


	case SAGENext::MEDIA_TYPE_LOCAL_VIDEO:
	{
		// media file is in my disk
		// mplayer is running in this machine
//		qDebug("%s::%s() : MEDIA_TYPE_LOCAL_VIDEO %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));

		if ( ! filename.isEmpty() ) {
			/**
			  create sageWidget
			*/
			SN_SageStreamWidget *sws = new SN_SageStreamWidget(filename, _globalAppId++, _settings, "127.0.0.1", _rMonitor);
			sws->appInfo()->setFileInfo(filename);
			w = sws;

			/**
		      add the widget to the queue
			*/
			_sageWidgetQueue.push_back(sws);
			_sageWidgetPosQueue.push_back(scenepos);

			/**
	          initiate SAIL process
			*/
			QProcess *proc = new QProcess(this);
//			proc->setWorkingDirectory("$SAGE_DIRECTORY");
//			proc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

			QStringList args;

			//
			// mplayer converts image frame to RGB24 using CPU
			//
//			args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-sws" << "4" << "-identify" << "-vf" << "format=rgb24" << filename;

			//
			// with current mplayer-svn.zip, UYUV will be used
			// conversion at the mplayer is cheaper
			// and image is converted to RGB using shader program
			//
			args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-sws" << "4" << "-identify" << filename;

			sws->appInfo()->setCmdArgs(args);

			//
			// this will invoke sail (outside of SAGENext) which will trigger fsManager::incomingSail(fsmThread *) signal which is connected to launch(fsmThread *)
			//
//			qDebug() << proc->environment();


//			proc->setWorkingDirectory(qApp->applicationDirPath());
//			qDebug() << qApp->applicationDirPath();


			proc->start("mplayer",  args);

			if (!proc->waitForStarted(-1)) {
				qDebug() << "Failed to start mplayer process" << proc->workingDirectory();
				qDebug() << proc->readAllStandardError();
				qDebug() << proc->readAllStandardOutput();
			}


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
		//
		// filename is web url string
		//
		SN_WebWidget *ww = new SN_WebWidget(_globalAppId++, _settings, 0, Qt::Window);
		w = ww;
		ww->setUrl( filename );
		break;
	}

	case SAGENext::MEDIA_TYPE_PDF: {
		SN_PDFViewerWidget *pdfviewer = new SN_PDFViewerWidget(filename, _globalAppId++, _settings, 0, Qt::Widget);
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

			qDebug() << "SN_Launcher launching a plugin" << w;
			w->setSettings(_settings);
			w->setGlobalAppId(_globalAppId++);
			w->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_PLUGIN);
			w->appInfo()->setFileInfo(filename);
		}
		else {
			qDebug() << "SN_Launcher::launch() : MEDIA_TYPE_PLUGIN : dpi is null";
		}

		break;
	} // end MEDIA_TYPE_PLUGIN

	} // end switch

	return launch(w, scenepos);
}

SN_BaseWidget * SN_Launcher::launch(QString username, QString vncPasswd, int display, QString vncServerIP, int framerate, const QPointF &scenepos /*= QPointF()*/) {
	//	qDebug() << "launch" << username << vncPasswd;
	SN_BaseWidget *w = new SN_VNCClientWidget(_globalAppId++, vncServerIP, display, username, vncPasswd, framerate, _settings);
	return launch(w, scenepos);
}

/**
  Note that pos is in SN_BaseWidget's parent coordinate
  If w has no parent then it's in scene coordinate
  */
SN_BaseWidget * SN_Launcher::launch(SN_BaseWidget *w, const QPointF &scenepos) {
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
//		++_globalAppId; // increment only when widget is created successfully
//		qDebug() << "gaid is now" << _globalAppId;
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

	////////////////////////////////////////
	//
	// record event NEW_POINTER : 1
	//
	if (_scenarioFile  &&  _settings->value("misc/record_launcher", false).toBool()) {
		if ( _scenarioFile->isOpen() && _scenarioFile->isWritable() ) {
			char record[256];
			sprintf(record, "%lld %d %u %s %s\n",QDateTime::currentMSecsSinceEpoch(), 1, uiclientid, qPrintable(name), qPrintable(color.name()));
			_scenarioFile->write(record);

			pointer = new SN_PolygonArrowPointer(uiclientid, _settings, _scene, name, color, _scenarioFile);
		}
		else {
			qDebug() << "Launcher::launchPointer() : Can't write";
		}
	}
	//
	///////////////////////////////////////

	else {
		pointer = new SN_PolygonArrowPointer(uiclientid, _settings, _scene, name, color);
	}

	//
	// pointer is added directly to the scene, not in the SN_LayoutWidget
	//
	_scene->addItem(pointer);
	if(!scenepos.isNull()) {
		pointer->setPos(scenepos);
	}

	return pointer;
}

void SN_Launcher::launchSavedSession(const QString &sessionfilename) {
	QFile f(sessionfilename);
	if (!f.exists()) {
		qDebug() << "SN_Launcher::launch session: file not exist" << sessionfilename;
		return;
	}
	if(!f.open(QIODevice::ReadOnly)) {
		qDebug() << "SN_Launcher::launch session: can't open file" << sessionfilename;
		return;
	}

	QDataStream in(&f);

	_scene->loadSession(in, this);

	f.close();

	SN_SimpleTextWidget *text = new SN_SimpleTextWidget(_settings->value("gui/fontpointsize",20).toInt() * 4, QColor(Qt::white), QColor(64, 64, 64, 128));
	text->setText("Saved session\n" + sessionfilename + "\nhas loaded");
	QPropertyAnimation *anim = new QPropertyAnimation(text, "opacity", text);
	anim->setStartValue(1.0);
	anim->setEndValue(0.0);
	anim->setEasingCurve(QEasingCurve::InExpo);
	anim->setDuration(1500);
	connect(anim, SIGNAL(finished()), text, SLOT(close()));

	_scene->addItem(text);
	text->setPos( (_scene->width()-text->size().width())/2 , (_scene->height()-text->size().height())/2 );
	anim->start();
}

void SN_Launcher::launchRecording(const QString &scenarioFilename) {
	ScenarioThread *thread = new ScenarioThread(this, scenarioFilename);
	qDebug() << "\nSN_Launcher::launchRecording() : START  " << scenarioFilename;
	thread->start();
}



/*!
  SECTION data is wrong. Don't use it.
  WINDOW NEW is generated by Ratko.
  SAGE only generated WINDOW CHANGE
  */
void SN_Launcher::launchRatkoUserStudyData(const QString &file, const QString &srcaddr/*=127.0.0.1*/, const QString &mediafile/*=""*/) {

	::usleep(5000000);

	QFile scenarioFile(file);
	if (!scenarioFile.exists()) return;
	if (!scenarioFile.open(QFile::ReadOnly)) return;

	int timeShrink = 10; // how much we want to fast-forward the entire scenario

	//
	// wall dimension when Ratko conducted this user study
	//
	int walldimx = 8160;
	int walldimy = 2304;

	qreal scalex = _scene->width() / (qreal)walldimx;
	qreal scaley = _scene->height() / (qreal)walldimy;

	/*
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

	process->start(cmd);
	*/


	//
	// which app to launch ?
	//
	// 0 - checker (locally)
	// 1 - mplayer (locally)
	// 2 - sagenext verion of checker (SAGENext internal)
	int application;
	QString program = "";

	if ( srcaddr.isEmpty() && mediafile.isEmpty() ) {
		// then it's SN_Checker
		application = 2;
		program = "SN_Checker";
	}
	else if (!mediafile.isEmpty()) {
		// then it's mplayer (SN_SageStreamWidget)
		application = 1;
		program = "mplayer";
//		cmd << "ssh" << "-xf" << srcaddr << "$SAGE_DIRECTORY/bin/mplayer" << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-sws" <<  "4" << mediafile;
//		qDebug() << cmd;
	}
	else if (!srcaddr.isEmpty() && mediafile.isEmpty()) {
		// then it's checker (SN_SageStreamWidget)
		application = 0;
		program = "checker";
	}
	qDebug() << "PROGRAM will be" << program;

//	int fps = 12;
	QString header(file);
	header.append(" ");
	header.append(srcaddr);
	header.append(" ");
	header.append(program);
//	header.append(QString(" fps ").append(QString::number(fps)));
	header.append(QString(" timeShrink ").append(QString::number(timeShrink)));
	header.append("\n");

	//
	// maps data's Appid to globalAppId
	//
	QMap<int, SN_BaseWidget *> appMap;
	appMap.clear();

	QMap<int, QProcess *> procMap;
	procMap.clear();


//	QRegExp windowNew("WINDOW NEW");
	QRegExp windowRemove("WINDOW REMOVE");
	QRegExp windowChange("WINDOW CHANGE");
	QRegExp windowZ("WINDOW Z");
	QRegExp window("WINDOW");
//	QRegExp starttime("^\\d+\\.\\d+$");

	char buff[2048]; // line buffer for data file
	int left,right,bottom,top,width,height,resx,resy,zvalue,sageappid,value;

	bool hasStartTime = false;
	qint64 startTime; // initial timestamp : scenario start time
	qint64 timestamp;
	char ts[128]; // to temporarily store ratko's timestamp value (e.g. 1298501258.85)

	char dummy[1024];
	char dummyChar; // to skip '[' and ']'
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

		//
		// WINDOW - gets the timestamp of the event and simulate the delay
		//
		if (line.contains(window)) {
			sscanf(buff, "%c %s", &dummyChar, ts);
			QString tsstring(ts);
			ok = false;

			//
			// get the scenario start time
			//
			if (!hasStartTime) {
				startTime = 1000 * tsstring.toDouble(&ok); // to msec
				if (ok) {
					hasStartTime = true;
					ok = false; // reset flag
				}
				else {
					qDebug() << "failed to convert the starting timestamp string to double. skipping line:" << line;
					continue;
				}
			}

			//
			// timestamp of this data line
			//
			timestamp = 1000 * tsstring.toDouble(&ok); // to msec
			if (!ok) {
				qDebug() << "failed to convert the starting timestamp string to double. skipping line:" << line;
				continue;
			}

			//
			// The delay between this event and the previous event
			//
			qint64 sleeptime = (timestamp - startTime) / timeShrink;
//			qDebug() << "actual sleep" << timestamp-startTime << " shrinked to" << sleeptime;
			usleep(sleeptime * 1000); // microsec
			QThread::yieldCurrentThread();
			startTime = timestamp; // update startTime with current event timestamp
		}


		//
		// WINDOW REMOVE
		//
		if (line.contains(windowRemove)) {
			// [ xxx.xx ] WINDOW REMOVE x
			sscanf(buff, "%c %s %c %s %s %d", &dummyChar,ts,&dummyChar,dummy,dummy,&sageappid);

//			qDebug("%llu) REMOVING appid %d\n", linenumber, sageappid);
			if ( appMap.find(sageappid) != appMap.end() ) {
				SN_BaseWidget *bw = appMap.value(sageappid);
				Q_ASSERT(bw);
				QMetaObject::invokeMethod(bw, "close", Qt::QueuedConnection);

//				if (procMap.find(sageappid) != procMap.end() ) {
//					QProcess *p = procMap.value(sageappid);
//					Q_ASSERT(p);
//					p->kill();
//				}
			}
			appMap.remove(sageappid);
//			procMap.remove(sageappid);
		}

		//
		// WINDOW CHANGE
		//
		else if (line.contains(windowChange)) {
			sscanf(buff, "%c %s %c %s %s %s %d %d %d %d %d %d %d %d %d %d %s %d %d %d", &dummyChar, ts, &dummyChar, dummy, dummy, dummy, &sageappid, &left, &right, &bottom, &top, &value, &zvalue, &value, &value, &value, dummy, &resx, &resy, &value);

			// native size (resx, resy)
			// window size (width, height)

			right *= scalex;
			left *= scalex;
			top *= scaley;
			bottom *= scaley;

			resx *= scalex;
			resy *= scaley;

			width = right - left;
			height = top - bottom;

//			qDebug() << "Scaled window rect" << left << top << width << height << " and resolution" << resx << resy;
//			qDebug() << "The rect (L,R,T,B) in sage coordinate" << left << right << top << bottom;

			// SAGENExt (0,0) is top left
			top = (walldimy * scaley) - top;
			bottom = top + height;

//			qDebug() << "The rect (L,R,T,B) in sagenext coordinate" << left << right << top << bottom;

			SN_BaseWidget *bw = 0;

			if ( appMap.find(sageappid) == appMap.end() ) {
				//
				// App doesn't exist
				//
				qDebug() << linenumber << ") -------------invoking new app";

				switch(application) {
				/*
				case 0: { // checker
					//
					// Below launches checker manually
					//
					QStringList args;
					args << "-xf" << srcaddr << "$SAGE_DIRECTORY/bin/checker" << "0" << QString::number(resx) << QString::number(resy) << "24";
					QProcess *proc = new QProcess(this);
					proc->start("ssh", args);
					if (!proc->waitForStarted(-1)) {
						qDebug() << "Failed to start checker process";
					}
					break;
				}
				*/
				case 1: { // mplayer
					//
					// Below launches mplayer
					//
					// SN_BaseWidget * launch(int mediatype, QString filename, const QPointF &scenepos = QPointF(30,30), qint64 filesize=0, QString senderIP="127.0.0.1", QString recvIP="", quint16 recvPort=0);
					bw = launch(SAGENext::MEDIA_TYPE_LOCAL_VIDEO, mediafile, QPointF(left, top));
					break;
				}
				case 2: { // sagenext checker
					bw = new SN_Checker(/* usePbo */ false, QSize(resx, resy), 24, 0, _settings, _rMonitor);
					QMetaObject::invokeMethod(this, "launch", Qt::QueuedConnection, Q_ARG(void *, bw), Q_ARG(QPointF, QPointF(left,top)));
//					launch(bw , QPointF(left, top));
					qApp->sendPostedEvents();
					break;
				}
				}

				appMap.insert(sageappid, bw); // sageappid in Ratko's data file and corresponding SN_BaseWidget pointer

				qDebug() << linenumber << ") --------------sage app started";
			}
			else {
				//
				// App had created already
				// and is now being interacted. manually update lastTouch
				//
				bw = appMap.value(sageappid);
//				bw->setLastTouch();
			}

//			qDebug("%llu) CHANGE : App id %d, WxH (%d x %d), native (%d x %d), scale %.2f", linenumber, sageappid,width,height,resx,resy, (qreal)width / (qreal)resx);

			// change geometry ( window move, window resize )
			bw->setPos(left, top);
			bw->setScale((qreal)width / (qreal)resx);
		}

		//
		// WINDOW Z
		//
		else if (line.contains(windowZ)) {
			QStringList strlist = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

			int offset = 6;
			int numapp = strlist.at(5).toInt();

			// following is to retrieve (appid, zvalue) pair
			for (int i=0; i<numapp; ++i) {
				int index = i * 2 + offset; // array index to find exact string
				int appid = strlist.at(index).toInt();
				if ( appMap.find(appid) != appMap.end() ) {
					SN_BaseWidget *bw = appMap.value(appid);
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
//	process.start("ssh -xf 127.0.0.1 killall -9 checker");
//	process.waitForFinished();
	qDebug() << "\n\n------------ Scenario finished in" << (simulend - simulstart) / 1000 << "sec. TimeShrink was" << timeShrink << "Total" << linenumber << "lines.\n\n";
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

		case 0: // NEW WIDGET
		{
			int mtype;
			char filename[256];
			int x,y;
			sscanf(line, "%lld %d %d %s %d %d", &when, &type, &mtype, filename, &x, &y);
//			qDebug() << "NEW_WIDGET" << mtype << filename;
//			_launcher->launch(mtype, QString(filename));
			QMetaObject::invokeMethod(_launcher, "launch", Qt::QueuedConnection, Q_ARG(int, mtype), Q_ARG(QString, QString(filename)), Q_ARG(QPointF, QPointF(x,y)));
			break;
		}
		case 1: // NEW POINTER
		{
			quint32 uiclientid;
			char pname[128];
			char color[16];
			sscanf(line, "%lld %d %u %s %s", &when, &type, &uiclientid, pname, color);
			qDebug() << "NEW_POINTER" << uiclientid << pname << color;
			SN_PolygonArrowPointer *pointer = _launcher->launchPointer(uiclientid, QString(pname), QColor(QString(color)));
			_launcher->_pointerMap.insert(uiclientid, pointer);
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

	qDebug() << "Scenario Thread finished. \n";
}

