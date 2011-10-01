#ifndef SAGENEXTLAUNCHER_H
#define SAGENEXTLAUNCHER_H

#include <QtCore>
#include <QtGui>

#include "common/commondefinitions.h"

class SAGENextScene;
class fsManager;
class fsManagerMsgThread;
class QSettings;

class ResourceMonitor;
class SchedulerControl;

class QGraphicsItem;

class BaseWidget;
class SageStreamWidget;
class SAGENextPolygonArrow;

class SAGENextLauncher : public QObject
{
        Q_OBJECT
public:
        explicit SAGENextLauncher(const QSettings *s, SAGENextScene *scene, ResourceMonitor *rm = 0, SchedulerControl *sc = 0, QFile *scenarioFile = 0, QObject *parent = 0);
        ~SAGENextLauncher();

	/**
	  This is temporary for scenario stuff
	  Use UiServer's container instead
	  */
	QMap<quint64, SAGENextPolygonArrow *> _pointerMap;

private:
        const QSettings *_settings;

        /*!
          * global
          */
        quint64 _globalAppId;

        /**
          The pointer to the scene
          */
        SAGENextScene *_scene;

        /**
          fsServer::checkClient()
          */
        fsManager *_fsm;

        /**
          Launcher creates sageWidget before firing SAIL application. So the sageWidget is waiting for actual SAIL connection to run.
          This is the queue for those sageWidget waiting SAIL connection. This queue is accessed only in this class.

          Be CAREFUL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          Let's say sageWidget_1 and sageWidget_2 are waiting in the queue.
          The SAIL_1 , SAIL_2 have been fired as well. But SAIL_2's connection got to the fsManager before SAIL_1
          Then SAIL_2 will be blindly assigned to sageWidget_1.

          This is because there's no way to co-relate sageWidget to SAIL connection...
          !!!!!!!!!!!!!!!!!!!!!!!!!!!!
          */
        QList<SageStreamWidget *> _sageWidgetQueue;

        void createFsManager();

        ResourceMonitor *_rMonitor;

        SchedulerControl *_schedCtrl;

		/**
		  To record new widget/pointer starts.
		  This will be passed to polygonArrows
		  */
		QFile *_scenarioFile;


public slots:
        /**
          This slot is invoked by the signal incomingSail in fsManager::incomingConnection
          */
        BaseWidget * launch(fsManagerMsgThread *);

        /**
          this is general launch function
          */
        BaseWidget * launch(int mediatype, QString filename, qint64 filesize=0, QString senderIP="127.0.0.1", QString recvIP="", quint16 recvPort=0);

        /**
          just for VNC widget
          */
        BaseWidget * launch(QString username, QString vncPasswd, int display, QString vncServerIP, int framerate = 10);

        /**
          The widget is added to the scene in here.
          _globalAppId is incremented by 1 in here
          */
        BaseWidget * launch(BaseWidget *);


		SAGENextPolygonArrow * launchPointer(quint64 uiclientid, const QString &name, const QColor &color);


		/**
		  This slot should be running in a separate thread
		  */
		void launchScenario(const QString &scenarioFilename);
};














class ScenarioThread : public QThread {
	Q_OBJECT
public:
	ScenarioThread(SAGENextLauncher *launcher, const QString &file, QObject *parent = 0);
	~ScenarioThread();

protected:
	void run();

private:
	SAGENextLauncher *_launcher;

	QFile _scenarioFile;

};

#endif // SAGENEXTLAUNCHER_H
