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
#include "applications/base/sagestreamwidget.h"

#include "applications/sn_fittslawtest.h"
#include "applications/sn_sagestreammplayer.h"
#include "applications/pdfviewerwidget.h"
#include "applications/pixmapwidget.h"
#include "applications/vncwidget.h"
#include "applications/webwidget.h"
#include "applications/sn_checker.h"

#include "applications/base/SN_plugininterface.h"


/*
  this is for launchRatko..()

  sagewidget is invoked using QMetaObject::invokeMethod() from launchRatko..() thread
  And I can't get return value using invokeMethod()....

  So, temporarily store recently launched widget pointer here
  */
SN_BaseWidget *theLastLaunched;


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
	_loadPlugins();

	// start listening for sage message
	_createFsManager();
}

SN_Launcher::~SN_Launcher() {
	/* kills fsManager */
	if (_fsm) _fsm->close(); _fsm->deleteLater();

	qDebug("%s::%s()" , metaObject()->className(), __FUNCTION__);
}

void SN_Launcher::_loadPlugins() {

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

quint64 SN_Launcher::_getUpdatedGlobalAppId(quint64 gaid) {
    quint64 retval = 0;

    if (gaid == 0) {
        retval = _globalAppId;
        _globalAppId += 1; // normal update
        return retval;
    }



    if ( _globalAppId < gaid) {
        // update _globalAppId to catch up
        _globalAppId = (gaid + 1);

        // allow users to set the GID manually
        retval = gaid;
    }
    else if (_globalAppId == gaid) {
        // gID override is safe
        _globalAppId += 1; // same as normal update
        retval = gaid;
    }
    else {
        // there could be a widget with gaid already
        if ( _scene->getUserWidget(gaid)) {
            qDebug() << "SN_Launcher::updateGlobalAppId() : there's a widget with GID" << gaid;
            retval = 0;
        }
        else {
            // seems safe to use the gaid
            // but _globalAppId won't change
            retval = gaid;
        }
    }

    return retval;
}


void SN_Launcher::_createFsManager() {
        /**
          QTcpServer::listen() will be called in constructor
          */
	_fsm = new fsManager(_settings, this);

    if (_fsm) {
        if ( ! QObject::connect(_fsm, SIGNAL(sageAppConnectedToFSM(QString,QString,fsManagerMsgThread*)), this, SLOT(launch(QString,QString,fsManagerMsgThread*))) ) {
            qDebug() << "SN_Launcher::_createFsManager() : failed to connect sageAppConnectedToFSM signal to the launch()";
        }
    }
    else {
        qDebug() << "SN_Launcher::_createFsManager() : failed to create the fsManager";
    }
}

SN_BaseWidget * SN_Launcher::launch(const QString &sageappname, const QString &mediafilepath, fsManagerMsgThread *fsmThread) {
	SN_SageStreamWidget *sw = 0;
	QPointF pos;

	//
	// This means the SAGE application was NOT started by the launcher but manually by running a SAGE application.
	// For instance, running a mplayer in console terminal will make the fsManager to emit incomingSail(fsmThread *) before SN_Launcher launches a SageStreamWidget for it.
	// In this case, fsManagerThread's incomingSail signal will trigger this slot.
	//
	if (_sageWidgetQueue.isEmpty()) {
//        qDebug() << "SN_Launcher::launch() : SAGE app fired manually. fsManager led me here.";

		quint64 GID = _getUpdatedGlobalAppId();

		// create new sageWidget
        if (sageappname == "mplayer") {
            sw = new SN_SageStreamMplayer(GID, _settings, _rMonitor);
            sw->appInfo()->setCmdArgs("-vo sage -nosound -loop 0 -sws 4 -quiet -framedrop");
        }
        else if (sageappname == "fittslawtest") {
            qDebug() << "SN_Launcher::launch() : SN_FittsLawTest";
            sw = new SN_SageFittsLawTest(GID, _settings, _rMonitor, 0, Qt::Window);
        }
        else {
            sw = new SN_SageStreamWidget(GID, _settings, _rMonitor); // 127.0.0.1 ??????????
        }
        sw->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_SAGE_STREAM);
        sw->appInfo()->setExecutableName(sageappname);
        sw->appInfo()->setFileInfo(mediafilepath);

		fsmThread->setGlobalAppId(GID);

		/*
		  for launchRatko..()
          this is not used.
		  */
		theLastLaunched = sw;
	}

	//
	// This means a SAGE application was started by the SN_Launcher.
	// For instance, through mediaBrowser, or drag&drop on sageNextPointer's drop frame.
	// This is a typical way to launch a sage app.
	//
	else {
//        qDebug() << "SN_Launcher::launch() : SAGE app fired by SAGENext. launchSageApp led me here.";

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

		QObject::connect(sw, SIGNAL(destroyed()), fsmThread, SLOT(sendSailShutdownMsg()));

        //
        // Now I'm sure there exist a SN_SageStreamWidget for this fsmThread so signal the thread so that the fsmThread wakes up
        // and the SN_SageStreamWidget::doInitReceiver() can be invoked in the fsmThread.
        //
        // Note that the doInitReceiver() will run SN_SageStreamWidget::waitForStreamerConnection() in a separate thread
        //
        // ! Possible race condition !
        //   1. fsmThread has null _sageWidget pointer.
        // 1.5. fsmThread->signalSageWidgetCreated called in here.
        //   2. fsmThread waits for the condition variable.
        //
        if ( ! QMetaObject::invokeMethod(fsmThread, "signalSageWidgetCreated", Qt::QueuedConnection) ) {
            qDebug() << "SN_Launcher::launch() : failed to invoke fsmThread->signalSageWidgetCreated.";
        }
	}
    else {
        qDebug() << "SN_Launcher::launch() : fsManagerMsgThread is waiting for receiver. But there's no SageStreamWidget to receive images ..";
        return 0;
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




SN_BaseWidget * SN_Launcher::launchSageApp(int mtype, const QString &filename, const QPointF &scenepos, const QString &senderIP, const QString &args, const QString &sageappname, quint64 gaid /* 0 */) {

	SN_SageStreamWidget *sws = 0;

	QString cmd;

    bool isSSH = false;

    quint64 GID = _getUpdatedGlobalAppId(gaid);

	switch (mtype) {

	//
	// This is a general sage stream. SN_SageStreamWidget is used to receive/display
	// ssh is used to fire sage application at the source address
	//
	// Args needed : senderIP and optional filename
	//
	case SAGENext::MEDIA_TYPE_SAGE_STREAM: {
		if (senderIP.isNull() || senderIP.isEmpty())  {
			qWarning() << "SN_Launcher::launch() : source IP addr is required for MEDIA_TYPE_SAGE_STREAM";
			return 0;
		}

        isSSH = true;

        if (sageappname == "mplayer") {
            sws = new SN_SageStreamMplayer(GID, _settings, _rMonitor);
            sws->appInfo()->setFileInfo(filename);
        }
        else if (sageappname == "fittslawtest") {
            sws = new SN_SageFittsLawTest(GID, _settings, _rMonitor, 0, Qt::Window);
            isSSH = false;
        }
        else {
            sws = new SN_SageStreamWidget(GID, _settings, _rMonitor);
        }

		sws->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_SAGE_STREAM);



        cmd = "ssh -fx";
        cmd.append(" "); cmd.append(senderIP);
        cmd.append(" "); cmd.append(sageappname);

        if (!args.isEmpty()) {
            cmd.append(" "); cmd.append(args);
            sws->appInfo()->setCmdArgs(args);
        }

        if (!filename.isEmpty()) {
            cmd.append(" "); cmd.append(filename);
            sws->appInfo()->setFileInfo(filename);
        }

		break;
	}

	//
	// Play media file in my local storage using SAIL (mplayer)
	// Args needed : filename
	//
	// mplayer is running in this machine
	//
	case SAGENext::MEDIA_TYPE_LOCAL_VIDEO:
	{
		//qDebug("%s::%s() : MEDIA_TYPE_LOCAL_VIDEO %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));

		if ( filename.isNull() || filename.isEmpty() ) {
			qWarning() << "SN_Launcher::launch() : I need filename to play..";
			return 0;
		}

		/**
		  create sageWidget
		  */
		sws = new SN_SageStreamMplayer(GID, _settings, _rMonitor);
        sws->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_LOCAL_VIDEO);
		sws->appInfo()->setFileInfo(filename);

			//
			// mplayer converts image frame to RGB24 using CPU
			// Use this if viewport isn't OpenGL widget !
			//
//		QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
//		args << "-vo" << "sage" << "-nosound" << "-loop" << "0" << "-sws" << "4" << "-identify" << "-vf" << "format=rgb24" << filename;

			//
			// with current mplayer-svn.zip, UYUV will be used
			// conversion at the mplayer is cheaper
			// and image is converted to RGB using shader program
			//
        QString arg = "-vo sage -nosound -loop 0 -sws 4 -framedrop -quiet";
        sws->appInfo()->setCmdArgs(arg);

        cmd = "mplayer";
        cmd.append(" "); cmd.append(arg);
		cmd.append(" "); cmd.append(filename);

		break;
	}
	}

	qDebug() << "SN_Launcher::launchSageApp() : The command for QProcess is" << cmd;

	sws->appInfo()->setExecutableName(sageappname);
	sws->appInfo()->setSrcAddr(senderIP);



    /**
      add the widget to the queue
      */
	_sageWidgetQueue.push_back(sws);
	_sageWidgetPosQueue.push_back(scenepos);



    if (isSSH) {
        if ( system(qPrintable(cmd)) == -1) {
            perror("system");
            _sageWidgetQueue.pop_back();
            _sageWidgetPosQueue.pop_back();
            sws->deleteLater();
            return 0;
        }
        else {
            return sws;
        }
    }
    else {
        QProcess *proc = new QProcess(this);
        //proc->setWorkingDirectory("$SAGE_DIRECTORY");
        //proc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        sws->setSailAppProc(proc);

        //
        // SAGE app will now connects to the fsManager (QTcpServer)
        //
        proc->start(cmd);

        if (!proc->waitForStarted(-1)) {
            qCritical() << "SN_Launcher::launch() : Failed to start remote process !!" << proc->workingDirectory();
            qDebug() << proc->readAllStandardError();
            qDebug() << proc->readAllStandardOutput();

            _sageWidgetQueue.pop_back();
            _sageWidgetPosQueue.pop_back();

            sws->deleteLater();

            return 0;
        }
        else {
        //
        ///
        //// launch(w) will be called in launch(fsmMessageThread *)
        //// Because of this, scenepos in savedSession has no effect !!! That's why _sageWidgetScenePosQueue has to be maintained
        //
            return sws;
        }
    }
}


/**
  * UiServer triggers this slot
  */
SN_BaseWidget * SN_Launcher::launch(int type, const QString &filename, const QPointF &scenepos/* = QPointF(30,30)*/, quint64 gaid/* 0 */) {
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

    quint64 GID = _getUpdatedGlobalAppId(gaid);

	SN_BaseWidget *w = 0;

	switch(type) {
	case SAGENext::MEDIA_TYPE_IMAGE: {
		//
		// streaming from ui client. This isn't used.
		//
		/***
		if ( fsize > 0 && !recvIP.isEmpty() && recvPort > 0) {

			// fire file receiving function
			//QFuture<bool> future = QtConcurrent::run(this, &SAGENextLauncher::fileReceivingFunction, type, filename, fsize, senderIP, recvIP, recvPort);

			w = new SN_PixmapWidget(fsize, senderIP, recvIP, recvPort, _globalAppId++, _settings);

			if(_mediaStorage)
				_mediaStorage->insertNewMediaToHash(filename);
		}
		***/

		//
		// from local storage
		// Args needed : filename
		//
		if ( !filename.isEmpty() ) {

//			qDebug("%s::%s() : MEDIA_TYPE_IMAGE %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
			w = new SN_PixmapWidget(filename, GID, _settings);

			if(_mediaStorage)
				_mediaStorage->insertNewMediaToHash(filename);
		}
		else
			qCritical("%s::%s() : MEDIA_TYPE_IMAGE can't open", metaObject()->className(), __FUNCTION__);

		break;
	}


	//
	// QtWebKit module is needed for SN_WebWidget
	// Args needed : filename
	//
	// filename is used for web url string
	//
	case SAGENext::MEDIA_TYPE_WEBURL: {

		SN_WebWidget *ww = new SN_WebWidget(GID, _settings, 0, Qt::Window);
		w = ww;
		ww->setUrl( filename );
		break;
	}

		//
		// poppler-qt4 library is needed for SN_PDFViewerWidget
		// Args needed : filename
		//
	case SAGENext::MEDIA_TYPE_PDF: {
		SN_PDFViewerWidget *pdfviewer = new SN_PDFViewerWidget(filename, GID, _settings, 0, Qt::Widget);
		w = pdfviewer;
		break;
	}

	case SAGENext::MEDIA_TYPE_AUDIO: {
		break;
	}
	case SAGENext::MEDIA_TYPE_UNKNOWN: {
		break;
	}

		//
		// Args needed : filename
		//
	case SAGENext::MEDIA_TYPE_PLUGIN: {
//		qDebug() << "MEDIA_TYPE_PLUGIN" << filename;
		SN_PluginInterface *dpi = _pluginMap.value(filename);
		if (dpi) {
			w = dpi->createInstance();

//			qDebug() << "SN_Launcher launching a plugin" << w;
			w->setSettings(_settings); // SN_Priority will be created in here if system/scheduler is set
			w->setGlobalAppId(GID);
			w->appInfo()->setMediaType(SAGENext::MEDIA_TYPE_PLUGIN);
			w->appInfo()->setFileInfo(filename);

		}
		else {
			qDebug() << "SN_Launcher::launch() : MEDIA_TYPE_PLUGIN : dpi is null";
		}

		break;
	}

	} // end switch

	return launch(w, scenepos);
}

SN_BaseWidget * SN_Launcher::launch(const QString &username, const QString &vncPasswd, int display, const QString &vncServerIP, int framerate, const QPointF &scenepos /*= QPointF()*/, quint64 gaid /* 0 */) {
	//	qDebug() << "launch" << username << vncPasswd;
    quint64 GID = _getUpdatedGlobalAppId(gaid);
	SN_BaseWidget *w = new SN_VNCClientWidget(GID, vncServerIP, display, username, vncPasswd, framerate, _settings);
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


        if ( w->isSchedulable() ) {
            if (_rMonitor) {
                w->setRMonitor(_rMonitor);
                _rMonitor->addSchedulableWidget(w);
            }
        }

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

			return launchSageApp((int)SAGENext::MEDIA_TYPE_LOCAL_VIDEO, filename);
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
		  Run Ratko data
		  */
		else if (filename.contains(rxRatkoData) ) {
			QtConcurrent::run(this, &SN_Launcher::launchRatkoUserStudyData, filename, QString("127.0.0.1"), QString());
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

SN_PolygonArrowPointer * SN_Launcher::launchPointer(quint32 uiclientid, UiMsgThread *msgthread, const QString &name, const QColor &color, const QPointF &scenepos /*= QPointF()*/) {

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

			pointer = new SN_PolygonArrowPointer(uiclientid, msgthread, _settings, _scene, name, color, _scenarioFile);
		}
		else {
			qDebug() << "Launcher::launchPointer() : Can't write";
		}
	}
	//
	///////////////////////////////////////

	else {
		pointer = new SN_PolygonArrowPointer(uiclientid,msgthread, _settings, _scene, name, color);
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
	QString sageAppIp = "127.0.0.1";
	int checkerfps = 24;

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

		qApp->sendPostedEvents();

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
				case 0: { // checker
					//
					// Below launches checker manually
					//
					// assumes checker is in $PATH at the srcaddr

					QString arg = "0 ";
					arg.append(QString::number(resx)); arg.append(" ");
					arg.append(QString::number(resy)); arg.append(" ");
					arg.append(QString::number(checkerfps));
//					bw = launchSageApp(SAGENext::MEDIA_TYPE_SAGE_STREAM, "", QPointF(left,top), sageAppIp, arg, "checker");

					if ( ! QMetaObject::invokeMethod(this, "launchSageApp", Qt::QueuedConnection
					                          , Q_ARG(int, (int)SAGENext::MEDIA_TYPE_SAGE_STREAM)
					                          , Q_ARG(QString, "")
					                          , Q_ARG(QPointF, QPointF(left,top))
					                          , Q_ARG(QString, sageAppIp)
					                          , Q_ARG(QString, arg)
					                          , Q_ARG(QString, "checker")) ) {
						qDebug() << "\n !! Failed to invoke sage app";
					}
					while(!theLastLaunched) {
						::usleep(10000); // 10 msec
					}
					bw = theLastLaunched;
					theLastLaunched = 0;

					break;
				}
				case 1: { // mplayer
					//
					// Below launches mplayer
					//
					// SN_BaseWidget * launch(int mediatype, QString filename, const QPointF &scenepos = QPointF(30,30), qint64 filesize=0, QString senderIP="127.0.0.1", QString recvIP="", quint16 recvPort=0);
					bw = launch(SAGENext::MEDIA_TYPE_LOCAL_VIDEO, mediafile, QPointF(left, top));
					break;
				}
				case 2: { // sagenext checker
//					SN_CheckerGL_Old *cgw = new SN_CheckerGL_Old(/* usePbo */ false, QSize(resx, resy), 24, _globalAppId++, _settings, _rMonitor);
					SN_CheckerGL *cgw = new SN_CheckerGL(GL_RGB, QSize(resx, resy), 24, _globalAppId++, _settings, _rMonitor);

					/*
					  The widget needs to have valid opengl context
					  So move it to main GUI thread
					  */
					cgw->moveToThread(QApplication::instance()->thread());

					bw = launch(cgw, QPointF(left, top));
//					QMetaObject::invokeMethod(this, "launch", Qt::QueuedConnection, Q_ARG(void *, bw), Q_ARG(QPointF, QPointF(left,top)));
//					while (! cgw->scene()) {
//						::usleep(10000);
//					}
					QMetaObject::invokeMethod(cgw, "doInit", Qt::QueuedConnection);
					break;
				}
				}

				Q_ASSERT(bw);
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
			if (bw) {
				bw->setPos(left, top);
				bw->setScale((qreal)width / (qreal)resx);
			}
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
			SN_PolygonArrowPointer *pointer = _launcher->launchPointer(uiclientid, 0 ,QString(pname), QColor(QString(color)));
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

