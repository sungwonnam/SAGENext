/*
 *
 *
 *
 *
 *
 * this is test
 *
 *
 *
 *
 */

#include <QtGui/QApplication>
#include <QGLWidget>

#include "settingdialog.h"
#include "sagenextscene.h"
#include "sagenextviewport.h"
#include "sagenextlauncher.h"

#include "uiserver/uiserver.h"

#include "applications/base/affinityinfo.h"

#include "system/sagenextscheduler.h"
#include "system/resourcemonitor.h"
#include "system/resourcemonitorwidget.h"

#ifdef Q_OS_LINUX
#include <numa.h>
extern void qt_x11_set_global_double_buffer(bool);
#endif



int main(int argc, char *argv[])
{
//	qt_x11_set_global_double_buffer(false);

        /***
          When using raster backend, For optimal performance only use the format types QImage::Format_ARGB32_Premultiplied, QImage::Format_RGB32 or QImage::Format_RGB16

          And use QImage as a paintdevice
          ***/
        QApplication::setGraphicsSystem("raster");

        QApplication a(argc, argv);

        qsrand(QDateTime::currentDateTime().toTime_t());

        /*
          create system wide settings object
          */
        QSettings s(QDir::homePath() + "/.sagenext/sagenext.ini", QSettings::IniFormat);

        /**
          find out x screen information
          */
        QDesktopWidget *dw = QApplication::desktop();
        qDebug() << "Desktop resolution " << dw->width() << " x " << dw->height();
        qDebug() << "\tPrimary screen: " << dw->primaryScreen();
        qDebug() << "\tScreen count: " << dw->screenCount();
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
//		s.setValue("general/offsetx", 0);
//		s.setValue("general/offsety", 0);
        }


        /*
        QPalette palette;
        palette.setColor(QPalette::Window, Qt::darkGray);
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, Qt::darkGray);
        palette.setColor(QPalette::Text, Qt::white);
        QApplication::setPalette(palette);
        */

        QFont font;
        font.setPointSize( s.value("general/fontpointsize", 20).toInt());
        font.setStyleStrategy(QFont::OpenGLCompatible);
        font.setStyleStrategy(QFont::NoAntialias);
        QApplication::setFont(font);




#if defined(Q_OS_LINUX)
        if (numa_available() < 0 ) {
                s.setValue("system/numa", false);
        }
        else {
                s.setValue("system/numa", true);
                //			qDebug() << numa_max_node() << numa_num_configured_nodes();
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



        // launch setting dialog
        SettingDialog sd(&s);
        sd.setWindowModality(Qt::ApplicationModal);
        sd.exec();


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
        qDebug() << "Creating SAGENext Scenes";
        SAGENextScene *scene = new SAGENextScene;
        scene->setBackgroundBrush(QColor(20, 20, 20));
        scene->setSceneRect(0, 0, s.value("general/width").toDouble(), s.value("general/height").toDouble());
        /*  This approach is ideal for dynamic scenes, where many items are added, moved or removed continuously. */
        scene->setItemIndexMethod(QGraphicsScene::NoIndex);

        /*
          attach close button on the scene. if clicked, scene->deleteLater will be called
          */
        QPixmap closeIcon(":/resources/close_over.png");
        PixmapCloseButtonOnScene *closeButton = new PixmapCloseButtonOnScene(closeIcon.scaledToWidth(scene->width() * 0.02));
        scene->addItem(closeButton);
        closeButton->setX(scene->width() - closeButton->boundingRect().width() - 1);








        /**
          ResourceMonitor , ResourceMonitorWidget
          Scheduler
          */
        ResourceMonitor *resourceMonitor = 0;
        int rMonitorTimerId = 0;
        ResourceMonitorWidget *rMonitorWidget = 0;
        SchedulerControl *schedcontrol = 0;
        if ( s.value("system/resourcemonitor").toBool() ) {
                qDebug() << "Creating ResourceMonitor";
                resourceMonitor = new ResourceMonitor(&s);

                if ( s.value("system/scheduler").toBool() ) {
                        qDebug() << "Creating" << s.value("system/scheduler_type").toString() << "Scheduler";

                        schedcontrol = new SchedulerControl(resourceMonitor);

                        a.installEventFilter(schedcontrol); // scheduler will monitor(filter) qApp's event

                        resourceMonitor->setScheduler(schedcontrol);

                        schedcontrol->launchScheduler( s.value("system/scheduler_type").toString(), s.value("system/scheduler_freq").toInt() );
                }

                char *val = 0;
                if ( val = getenv("EXP_DATA_FILE") ) {
                        qDebug("EXP_DATA_FILE is defined");
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
          create the launcher
          */
        SAGENextLauncher *launcher = new SAGENextLauncher(&s, scene, resourceMonitor, schedcontrol, scene); // scene is the parent




        /**
          create the UiServer
          */
        UiServer *uiserver = new UiServer(&s, launcher, scene);




        /**
          create viewport widget (This is a QWidget)
          */
        qDebug() << "Creating SAGENext Viewport";

        SAGENextViewport *gvm = new SAGENextViewport;

        gvm->setLauncher(launcher);

        if ( s.value("system/openglviewport").toBool() ) {
                gvm->setViewport(new QGLWidget());
                qDebug() << "[ using QGLWidget as the viewport ]";
        }

        if ( QString::compare(s.value("system/viewportupdatemode").toString(), "full", Qt::CaseInsensitive) == 0 ) {
                gvm->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        }
        else if ( QString::compare(s.value("system/viewportupdatemode").toString(), "minimal", Qt::CaseInsensitive) == 0 ) {
                gvm->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate); // default
        }
        else if ( QString::compare(s.value("system/viewportupdatemode").toString(), "smart", Qt::CaseInsensitive) == 0 ) {
                gvm->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
        }
        else if ( QString::compare(s.value("system/viewportupdatemode").toString(), "no update", Qt::CaseInsensitive) == 0 ) {
                gvm->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
        }
        else {
                gvm->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate); // default
        }
        /* DON'T DO THIS ! poor performance and not correct. I don't know why. It's worth look into this */
        //	gvm->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);


        gvm->setRenderHint(QPainter::SmoothPixmapTransform);
//	gvm->setRenderHint(QPainter::HighQualityAntialiasing);
        gvm->setAttribute(Qt::WA_DeleteOnClose);
        gvm->setWindowFlags(Qt::FramelessWindowHint);
        gvm->setWindowState(Qt::WindowFullScreen);
        gvm->setDragMode(QGraphicsView::RubberBandDrag);
        gvm->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);

        gvm->setScene(scene);

        gvm->move(s.value("general/offsetx", 0).toInt(), s.value("general/offsety", 0).toInt());
        gvm->resize(scene->sceneRect().size().toSize());

        gvm->show();


//	launcher->launch("evl123", 0, "131.193.77.191", 24);


        /**
          system-wide media cache
          key : "HOST:PATH/FILENAME"
          value : QPixmap object (this shouldn't be too big)
          **/
        QHash<QString, QPixmap> mediaHash;
        QReadWriteLock mediaHashRWLock;


        // starts event loop
        int ret = a.exec();





        if (uiserver) delete uiserver;
//	if (launcher) delete launcher; // launcher is a child of scene. So laucher will be delete when scene is deleted automatically
        if (resourceMonitor) {
                resourceMonitor->killTimer(rMonitorTimerId);
                if (schedcontrol) {
                        delete schedcontrol;
                }
                delete resourceMonitor;
        }
        return ret;
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
