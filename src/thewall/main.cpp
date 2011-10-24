#include <QtGui/QApplication>
#include <QGLWidget>

//#include "settingdialog.h"
#include "settingstackeddialog.h"
#include "sagenextscene.h"
#include "sagenextviewport.h"
#include "sagenextlauncher.h"
#include "mediastorage.h"

#include "uiserver/uiserver.h"
#include "uiserver/fileserver.h"

#include "applications/base/affinityinfo.h"
#include "applications/mediabrowser.h"

#include "system/sagenextscheduler.h"
#include "system/resourcemonitor.h"
#include "system/resourcemonitorwidget.h"

#ifdef Q_OS_LINUX
#include <numa.h>
#endif

#ifdef Q_WS_X11
#include <X11/Xlib.h>
extern void qt_x11_set_global_double_buffer(bool);
#endif

#include <unistd.h>

void setViewAttr(SN_Viewport *view, const QSettings &s);



int main(int argc, char *argv[])
{
#ifdef Q_WS_X11
	XInitThreads(); // to enabled threaded-opengl in 4.8

	//
	// what is this?
	//
//	qt_x11_set_global_double_buffer(true);
#endif

	int errflag = 0;
	int c;
	QString configFile(QDir::homePath() + "/.sagenext/sagenext.ini");
	bool popSettingsDialog = true;
	QString recordingname; // null, empty string

	while ((c = getopt(argc, argv, "c:r:")) != -1) {
		switch(c) {
		case 'c': // config file
		{
			configFile = QString(optarg);
			popSettingsDialog = false;
			break;
		}
		case 'r': // recording
		{
			recordingname = QString(optarg);
			break;
		}
		case '?':
		{
			errflag++;
			break;
		}
		default:
			errflag++;
			break;
		}
	}
	if (errflag) {
		fprintf(stderr, "\n");
		fprintf(stderr, "usage: %s\n", argv[0]);
		fprintf(stderr, "\t -c <config file>: Optional config file name. default is ~/.sagenext/sagenext.ini\n");
		fprintf(stderr, "\t -r <recording filename>: Turn on recording. Recording file is created in ~/.sagenext\n");
		fprintf(stderr, "\t -g <opengl | raster>: graphics backend. opengl backend is still experimental\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Examples\n");
		fprintf(stderr, "./sagenext  (Run sagenext without recording. Configuration dialog will pop up with the default config file\n");
		fprintf(stderr, "./sagenext -c configFile.ini  (Run sagenext without recording. Configuration dialog will NOT pop up\n");
		fprintf(stderr, "./sagenest -c configFile.ini -r myrecord.recording  (Run sagenext with recording enabled. Configuration dialog will NOT pop up\n");
		fprintf(stderr, "\n");
		exit(-1);
	}




//	QApplication::setGraphicsSystem("opengl"); // this is Qt's experimental feature


	/***
	  When using raster backend, For optimal performance only use the format types
      QImage::Format_ARGB32_Premultiplied, QImage::Format_RGB32 or QImage::Format_RGB16
	  And use QImage as a paintdevice.

	  On Linux/X11 this performs very bad
	***/
//	QApplication::setGraphicsSystem( "raster" );


	/***
	  On Linux/X11 platforms which support it, Qt will use glX/EGL texture-from-pixmap extension.
This means that if your QPixmap has a real X11 pixmap backend, we simply bind that X11 pixmap as a texture and avoid copying it.
You will be using the X11 pixmap backend if the pixmap was created with QPixmap::fromX11Pixmap() or you’re using the “native” graphics system.
Not only does this avoid overhead but it also allows you to write a composition manager or even a widget which shows previews of all your windows.


QPixmap, unlike QImage, may be hardware dependent.
On X11, Mac and Symbian, a QPixmap is stored on the server side while a QImage is stored on the client side
(on Windows,these two classes have an equivalent internal representation,
i.e. both QImage and QPixmap are stored on the client side and don't use any GDI resources).

Note that the pixel data in a pixmap is internal and is managed by the underlying window system.
	  ***/
//	QApplication::setGraphicsSystem("native"); // default


	QApplication a(argc, argv);

	qsrand(QDateTime::currentDateTime().toTime_t());

	/*
      create system wide settings object
	 */
	QSettings s(configFile, QSettings::IniFormat);


	if (!recordingname.isNull() && !recordingname.isEmpty()) {
		s.setValue("misc/record_launcher", true);
		s.setValue("misc/record_pointer", true);
		s.setValue("misc/recordingfile", recordingname);
	}
	else {
		s.setValue("misc/record_launcher", false);
		s.setValue("misc/record_pointer", false);
	}


	//
	// load screenLayout from a file and try read previous values
	//
	QMap<QPair<int, int>, int> screenLayout;
	QFile layoutFile(QDir::homePath() + "/.sagenext/screenlayout");
	if (layoutFile.exists()) {
		layoutFile.open(QIODevice::ReadOnly);
		//
		// fill the map with previous values
		QDataStream in(&layoutFile);
		while(! in.atEnd()) {
			int x,y,screenid;
			in >> x >> y >> screenid;
			screenLayout.insert(QPair<int,int>(x,y), screenid);
		}
	}
	layoutFile.close();
	/*
	qDebug() << "Screen layout before setting dialog";
	QMap<QPair<int,int>,int>::iterator it = screenLayout.begin();
	for (; it != screenLayout.end(); it++) {
		qDebug() << it.key() << ": " << it.value();
	}
	*/


        /**
          find out X screen information
          */
	QDesktopWidget *dw = QApplication::desktop();
	qDebug() << "Desktop resolution " << dw->width() << " x " << dw->height();
	qDebug() << "\tPrimary screen: " << dw->primaryScreen();

	s.setValue("graphics/screencount", dw->screenCount());
	qDebug() << "\tScreen count: " << dw->screenCount();

	s.setValue("graphics/isvirtualdesktop", dw->isVirtualDesktop());
	qDebug() << "\tis VirtualDesktop: " << dw->isVirtualDesktop();

	qDebug() << "\tAvailable geometry: " << dw->availableGeometry();
	for (int i=0; i<dw->screenCount(); ++i) {
//		QRect availGeoOfThis = dw->availableGeometry(i);
//		qDebug("GraphicsViewMain::%s() : screen %d, available geometry (%d,%d) %dx%d", __FUNCTION__, i,
//			   availGeoOfThis.x(), availGeoOfThis.y(), availGeoOfThis.width(), availGeoOfThis.height());
		qDebug() << "\t\tScreen " << i << " screenGeometry: " << dw->screenGeometry(i);
	}


	// if user doens't defing size of the wall, use max available
	if ( s.value("general/width", 0).toInt() <= 0  ||  s.value("general/height", 0).toInt() <= 0 ) {
		s.setValue("general/width", dw->width());
		s.setValue("general/height", dw->height());
		//s.setValue("general/offsetx", 0);
		//s.setValue("general/offsety", 0);
	}

//	QPalette palette;
//	palette.setColor(QPalette::Window, Qt::darkGray); // general background role
//	palette.setColor(QPalette::WindowText, Qt:); // foreground role (Painter's pen)
//	QBrush brush(QColor(100, 100, 100, 128), Qt::SolidPattern);
//	palette.setBrush(QPalette::WindowText, brush);
//	palette.setColor(QPalette::Base, Qt::darkGray);
//	palette.setColor(QPalette::Text, Qt::white);
//	QApplication::setPalette(palette);

	QFont font;
	font.setPointSize( s.value("gui/fontpointsize", 20).toInt());
	font.setStyleStrategy(QFont::OpenGLCompatible);
	font.setStyleStrategy(QFont::NoAntialias);
	QApplication::setFont(font);




#if defined(Q_OS_LINUX)
	if (numa_available() < 0 ) {
		s.setValue("system/numa", false);
	}
	else {
		s.setValue("system/numa", true);
		//qDebug() << numa_max_node() << numa_num_configured_nodes();
		s.setValue("system/numnumanodes", numa_num_configured_nodes());

		//qDebug() << AffinityInfo::Num_Numa_Nodes << AffinityInfo::Num_Cpus << "\n\n";

		/*
			int numa_max_possible_node(); // the size of a kernel type nodemask_t (in bits) - 1
			int numa_num_possible_nodes(); // the size of a kernel type nodemask_t

			int numa_max_node(); // the numa node id of the last node (1 if there are two numa nodes)
			int numa_num_configured_node(); // the number of numa node in this system

			int numa_num_thread_cpus();
			int numa_num_thread_ndoes();

			long numa_node_size(int node, 0); // total
		*/
		qDebug() << "Your system supports NUMA API." << "There are " << numa_num_configured_nodes() << "Configured NUMA nodes";
		/*
	  fprintf(stdout, "Max possible node %d, num possible nodes %d\n", numa_max_possible_node(), numa_num_possible_nodes());
	  fprintf(stdout, "Max node %d, num configured node %d\n", numa_max_node(), numa_num_configured_node());
		  fprintf(stdout, "Num thread cpus %d, num thread nodes %d\n", numa_num_thread_cpus(), numa_num_thread_nodes());
		  for ( int i=0; i< numa_num_configured_nodes(); ++i ) {
			  fprintf(stdout, "Node %d : %l MByte of memory\n", i, numa_node_size(i,0) / (1024*1024) );
		  }

		  settings->setValue("system/numnumanodes", numa_num_configured_node());
		  settings->setValue("system/numa", true);
		*/
	}

                //      int ret = sched_getcpu();
                //      qDebug("GraphicsViewMain::%s() : main() is running on cpu %d", __FUNCTION__, ret);

                /* set this process affine to NUMA node .. */
                /***
                struct bitmask *mask = 0;

                QVariant var = s.value("system/mainonnode");
                if ( !var.isNull() && var.isValid() ) {
                        QString str = var.toString().remove(QChar('"'), Qt::CaseInsensitive);
                        qDebug() << "main on node" << str;
                        mask = numa_parse_nodestring(str.toAscii().data());

                        //	numa_bitmask_setbit(mask, var.toInt());
                        if (mask) {
                                if ( numa_run_on_node_mask(mask) != 0 ) {
                                        perror("numa_run_on_node_mask");
                                }
                                numa_free_nodemask(mask);
                        }
                }
                ***/
#endif


	//
	// launch setting dialog if user didn't provide config file
	//
	if (popSettingsDialog) {
		SettingStackedDialog sd(&s, &screenLayout);
		sd.setWindowModality(Qt::ApplicationModal);
		sd.adjustSize();
		sd.exec();
	}


        /**
          set system arch. related parameters
          These values are provided by a user
          */
	bool ok = false;
	int tmp = -1;
	tmp = s.value("system/numnumanodes").toInt(&ok);
	AffinityInfo::Num_Numa_Nodes = ok ? tmp : -1;

	tmp = s.value("system/cpupernumanode").toInt(&ok);
	AffinityInfo::Cpu_Per_Numa_Node = ok ? tmp : -1;

	tmp = s.value("system/threadpercpu").toInt(&ok);
	AffinityInfo::HwThread_Per_Cpu = ok ? tmp : -1;

	tmp = s.value("system/swthreadpercpu").toInt(&ok);
	AffinityInfo::SwThread_Per_Cpu = ok ? tmp : -1;

	tmp = s.value("system/numcpus").toInt(&ok);
	AffinityInfo::Num_Cpus = ok ? tmp : -1;



	/**
      create the scene (This is a QObject)
      */
	qDebug() << "\nCreating the scene" << s.value("general/width").toDouble() << "x" << s.value("general/height").toDouble();
	SN_TheScene *scene = new SN_TheScene(QRectF(0, 0, s.value("general/width").toDouble(), s.value("general/height").toDouble()) ,  &s);



	/**
	  ResourceMonitor , ResourceMonitorWidget
	  Scheduler
      */
	SN_ResourceMonitor *resourceMonitor = 0;
	int rMonitorTimerId = 0;
	ResourceMonitorWidget *rMonitorWidget = 0;
	SN_SchedulerControl *schedcontrol = 0;
	if ( s.value("system/resourcemonitor").toBool() ) {
		qDebug() << "Creating ResourceMonitor";
		resourceMonitor = new SN_ResourceMonitor(&s);

		if ( s.value("system/scheduler").toBool() ) {
			qDebug() << "Creating" << s.value("system/scheduler_type").toString() << "Scheduler";

			schedcontrol = new SN_SchedulerControl(resourceMonitor);

			a.installEventFilter(schedcontrol); // scheduler will monitor(filter) qApp's event

			resourceMonitor->setScheduler(schedcontrol);

			schedcontrol->launchScheduler( s.value("system/scheduler_type").toString(), s.value("system/scheduler_freq").toInt() );
		}

		char *val = getenv("EXP_DATA_FILE");
		if ( val ) {
			qWarning("EXP_DATA_FILE is defined. Performance data will be written by the ResourceMonitor.");
			resourceMonitor->setPrintDataFlag(true);
			resourceMonitor->printPrelimDataHeader();
		}

		resourceMonitor->refresh();

		rMonitorWidget = new ResourceMonitorWidget(resourceMonitor, schedcontrol); // No parent widget
		//		rMonitorWidget->show();

		resourceMonitor->setRMonWidget(rMonitorWidget);

		// this will trigger resourceMonitor->refresh() every 1sec
		rMonitorTimerId = resourceMonitor->startTimer(1000);
	}


	/**
	  create the MediaStorage
	*/
//	SN_MediaStorage *mediaStorage = 0;
	SN_MediaStorage *mediaStorage = new SN_MediaStorage(&s);


	/**
	  create the launcher
	*/
	QFile *recordingFile = 0;
	if (s.value("misc/record_launcher", false).toBool() ||  s.value("misc/record_pointer", false).toBool()) {

		recordingname.append( "__" + s.value("general/wallip").toString() );
		QString filetimestr = QDateTime::currentDateTime().toString("__hh.mm.ss_MM.dd.yyyy_");
		filetimestr.append(".recording");
		recordingname.append(filetimestr);

		qDebug() << "\n SAGENext will record events to" << recordingname;

		recordingFile = new QFile(QDir::homePath() + "/.sagenext/" + recordingname);
		recordingFile->open(QIODevice::ReadWrite);

		qint64 time = QDateTime::currentMSecsSinceEpoch();
		char buffer[64];
		sprintf(buffer, "%lld\n", time);
		recordingFile->write(buffer); // fill the first line
	}
	SN_Launcher *launcher = new SN_Launcher(&s, scene, mediaStorage, resourceMonitor, schedcontrol, recordingFile, scene); // scene is the parent


	/**
	  create the UiServer
	 */
	SN_UiServer *uiserver = new SN_UiServer(&s, launcher, scene);
	scene->setUiServer(uiserver);


	/**
	  create the initial MediaBrowser.
	*/
	SN_MediaBrowser *mediaBrowser = new SN_MediaBrowser(launcher, 0, &s, mediaStorage);
	launcher->launch(mediaBrowser);


	/**
	  create the FileServer
	 */
	SN_FileServer *fileServer = new SN_FileServer(&s, launcher);
//	qDebug() << "FileServer is listening on" << fileServer->fileServerListenPort();


	/*
	qDebug() << "Screen layout after setting dialog";
	QMap<QPair<int,int>,int>::iterator it2 = screenLayout.begin();
	for (; it2 != screenLayout.end(); it2++) {
		qDebug() << it2.key() << ": " << it2.value();
	}
	*/


        /**
          create viewport widget (This is a QWidget)
          */
	qWarning() << "\nCreating SAGENext Viewport";
	SN_Viewport *gvm = 0;

	// This hurts performance !!
	// don't do this
	//QGLFormat glFormat(QGL::DoubleBuffer | QGL::Rgba  | QGL::DepthBuffer | QGL::SampleBuffers);

	if (s.value("graphics/isxinerama").toBool()) {
		gvm = new SN_Viewport(scene, 0, launcher);

		if ( s.value("graphics/openglviewport").toBool() ) {
			//gvm->setViewport(new QGLWidget(glFormat));
			gvm->setViewport(new QGLWidget);
			gvm->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

			//
			// with Xinerama, full screen won't work
			//
//			gvm->setWindowState(Qt::WindowFullScreen);

			qDebug() << "\t[using QGLWidget as the viewport widget]";
		}
		gvm->move(s.value("general/offsetx", 0).toInt(), s.value("general/offsety", 0).toInt());
        gvm->resize(scene->sceneRect().size().toSize());

		setViewAttr(gvm, s);

		gvm->show();
	}
	else {
		for (int i=0; i<screenLayout.size(); i++) {
			//
			// A viewport for each X screen
			//
			gvm = new SN_Viewport(scene, i, launcher, dw->screen(i));

			if ( s.value("graphics/openglviewport").toBool() ) {
				gvm->setViewport(new QGLWidget(dw->screen(i)));
				gvm->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
				gvm->setWindowState(Qt::WindowFullScreen);
				qDebug() << "\t[using QGLWidget as the viewport widget]";

			}
			//
			// set sceneRect to be viewed for each viewport. Bezel size has to be applied to here
			//
			QPair<int,int> pos = screenLayout.key(i); // x,y position of the screen (0,0) as a topleft

//			s.value("graphics/bezelsize");

			gvm->setSceneRect(
			            pos.first * dw->screenGeometry(i).width()
			            ,pos.second * dw->screenGeometry(i).height()
			            ,dw->screenGeometry(i).width()
			            ,dw->screenGeometry(i).height()
			);

			//
			// resize viewport to screen geometry (filling the screen)
			//
			gvm->resize(dw->screenGeometry(i).size());

			setViewAttr(gvm, s);

			gvm->show();
		}
	}



		//launcher->launch("", "evl123", 0, "131.193.77.191", 24);
	//launcher->launch(MEDIA_TYPE_WEBURL, "http://youtube.com");
//		launcher->launch(MEDIA_TYPE_PLUGIN, "/home/sungwon/.sagenext/plugins/libImageWidgetPlugin.so");
//		launcher->launch(MEDIA_TYPE_IMAGE, "/home/sungwon/.sagenext/media/image/DR_map.jpg");
//		launcher->launch(MEDIA_TYPE_PDF, "/home/sungwon/.sagenext/media/pdf/oecc_iocc_2007.pdf");

//	launcher->launchScenario( QDir::homePath() + "/.sagenext/test.scenario" );


	// starts the event loop
	int ret = a.exec();


	if (recordingFile) {
		recordingFile->flush();
		recordingFile->close();
	}



	if (fileServer) delete fileServer;
	//if (uiserver) delete uiserver; // uiserver is deleted by the scene
	//if (launcher) delete launcher; // launcher is a child of scene. So laucher will be delete when scene is deleted automatically
		/*
		if (resourceMonitor) {
			resourceMonitor->killTimer(rMonitorTimerId);
			if (schedcontrol) {
				delete schedcontrol;
			}
			delete resourceMonitor;
		}
		*/
	return ret;
}



void setViewAttr(SN_Viewport *gvm, const QSettings &s) {
	//
	// viewport update mode
	//
	if ( ! s.value("graphics/openglviewport").toBool() ) {
		if ( QString::compare(s.value("graphics/viewportupdatemode").toString(), "minimal", Qt::CaseInsensitive) == 0 ) {
			gvm->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate); // default
		}
		else if ( QString::compare(s.value("graphics/viewportupdatemode").toString(), "smart", Qt::CaseInsensitive) == 0 ) {
			gvm->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
		}
		else if ( QString::compare(s.value("graphics/viewportupdatemode").toString(), "full", Qt::CaseInsensitive) == 0 ) {
			gvm->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
		}
		else if ( QString::compare(s.value("graphics/viewportupdatemode").toString(), "no update", Qt::CaseInsensitive) == 0 ) {
			gvm->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
		}
		else {
			gvm->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate); // default
		}
	}
	/* DON'T DO THIS ! poor performance and not correct. I don't know why. It's worth look into this */
    //gvm->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

	gvm->setFrameStyle(QFrame::NoFrame);
	gvm->setLineWidth(0);
	gvm->setContentsMargins(0, 0, 0, 0);

//	gvm->setRenderHint(QPainter::SmoothPixmapTransform); // this isn't good for performance
	//gvm->setRenderHint(QPainter::HighQualityAntialiasing); // OpenGL
	gvm->setAttribute(Qt::WA_DeleteOnClose);
	gvm->setWindowFlags(Qt::FramelessWindowHint);

	gvm->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	gvm->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	gvm->setDragMode(QGraphicsView::NoDrag); // NoDrag is default

	///
	/// GraphicsView will not protect the painter state when rendering. Changes in painter state (pen, brush,..) will remain changed if set true.
	/// You can call QPainter::setPen() or setBrush() without restoring the state after painting
	///
	gvm->setOptimizationFlag(QGraphicsView::DontSavePainterState, false);
	
	gvm->viewport()->setAttribute(Qt::WA_NoSystemBackground);
}


/**
#if defined(Q_OS_LINUX)
numa_set_bind_policy(1);
struct bitmask *mask = numa_allocate_nodemask();
numa_bitmask_setbit(mask, 0);
// void numa_bind(struct bitmask *nodemask);
// int numa_run_on_node(int node);
// int numa_run_on_node_mask(struct bitmask *);
if ( numa_run_on_node_mask(mask) != 0 ) {
        qDebug("%s() : numa_run_on_node_mask() failed", __FUNCTION__);
}

// void numa_set_membind(struct bitmask *nodemask);
// void numa_bind(struct bitmask *nodemask);
numa_set_membind(mask);

numa_free_nodemask(mask);

mask = numa_allocate_cpumask();
numa_bitmask_setbit(mask, 2);
numa_bitmask_setbit(mask, 3);
// int numa_sched_setaffinity(pid_t pid, struct bitmask *);
if ( numa_sched_setaffinity(0, mask) != 0 ) {
        qDebug("%s() : numa_sched_setaffinity() failed", __FUNCTION__);
}
numa_free_cpumask(mask);

#endif
**/




