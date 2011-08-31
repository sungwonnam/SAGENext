#ifndef GRAPHICSVIEWMAINWINDOW_H
#define GRAPHICSVIEWMAINWINDOW_H

#include "common/commondefinitions.h"
#include "common/commonitem.h"


typedef struct {
	QFileInfo file;
	MEDIA_TYPE mediaType;
	QRectF geometry;
	int scale;
} SessionItemInfo;

//class SettingDialog;
//class SystemArch;
class UiServer;
class fsManager;
class AffinityInfo;
class ResourceMonitor;
class SchedulerControl;


class ResourceMonitorWidget;

class BaseWidget;

/*!
  This is used in startSageApp() and loadSaveScenario()
  */
typedef struct {
	BaseWidget *bwPtr;
	quint64 gaid;
} SageWidgetInfo;

/*!
  Viewport widget

  Note that this is the widget that receives all the mouseEvents not the individual widgets.
  Create mouseEvent with x,y coordinate mapped to viewport coordinate, post the event targetting this widget.
  */
class GraphicsViewMain : public QGraphicsView
{
	Q_OBJECT
public:
	GraphicsViewMain(const QSettings *s, ResourceMonitor *rm = 0, SchedulerControl *ss = 0);
	~GraphicsViewMain();

protected:
	void paintEvent(QPaintEvent *event);
	void timerEvent(QTimerEvent *);
	void resizeEvent(QResizeEvent *event);

	void mousePressEvent(QMouseEvent *event);

private:
	/*!
	  * global to Application
	  */
	quint64 globalAppId;

	/*!
	  This is to be used in loadSavedScenario.
	  It maps appId in ratko's data to widget pointer
	  */
//	QMap<int, BaseWidget *> WidgetAppIdMap;

	/*!
	  sage app is start by console command (QProcess::start())
	  So there's no way to pass started sage app pointer or id internally to function that invoked sage app.
	  startSageApp() fills this struct after instantiating SageStreamWidget
	  loadSavedScenario polls on this struct
	*/
	SageWidgetInfo sageWidgetInfo;


	/*!
	  * INI file is stored at
	  * $HOME/.config if UserScope or /etc/xdg  if SystemScope in Unix and Mac
	  * %APPDATA% if UserScope or %COMMON_APPDATA% if SystemScope in Windows
	  */
	const QSettings *settings;


	/*!
	  A viewport widget displays QGraphicsScene
	  */
	QGraphicsScene *gscene;



	/*!
	  * This object serves external UI clients
	  * It's TcpServer and fires a msg thread for each UI connection.
	  * serialization of multiple UI msgs is done automatically by queued connection of signal/slots
	  */
	UiServer *extUiServer;

	/*!
	  * when this class instantiate fsm,
	  * pass 'this' to the fsm constructor
	  * to tell this class is a parent
	  */
	fsManager *fsm;


	/*!
	  * timer for timerEvent
	  */
	int timerID;


	int paintingOnCpu;
	SwSimpleTextItem *textItem;

	/*!
	  * Resource Monitor
	  */
	ResourceMonitor *resourceMonitor;

	/*!
	  Scheduler
	  */
	SchedulerControl *scheduler;


	/*!
	  File dialog (CMD or CTRL - O) to open local files
	  */
	QFileDialog *fdialog;


	/*!
	  A resource monitor panel
	  */
	ResourceMonitorWidget *rMonitorWidget;


	QAction *changeBGcolorAction;
	QAction *openMediaAction;
	QAction *showInfoAction;
//	QAction *schedulingAction;

	QAction *saveSessionAction;
	QAction *loadSessionAction;
//	QAction *gridLayoutAction;
	void createActions();


	/*!
	  will be passed to resourceMonitor::printPrelimData() in timerEvent
	  */
	QFile prelimDataFile;
//        bool prelimDataFlag;

signals:
	/*!
	  * connected to BaseWidget::drawInfo()
	  */
	void showInfo();

public slots:
	void on_actionBackground_triggered();
	void on_actionOpen_Media_triggered();
	void on_actionFilesSelected(const QStringList &selected);
	void on_actionShowInfo_triggered();
//	void on_actionScheduling_triggered();

	/*!
	  This function stores windows geometry info into a session file ($HOME/.sagenext/sessions/FILENAME.session)
	  */
	void on_actionSaveSession_triggered();
//	void on_actionGridLayout_triggered();



	/*!
	  * This function creates GraphicsScene/View, UiServer, fsManager objects and provides slots to start widgets, open files/plugins
	  * This function is invoked (directConnection) after settingDialog closed in main.cpp
	  */
	void startWall();

	/*!
	 * fsManager emits sailConnected() when it receives fsManagerMsgThread::sailConnected() signal from a msgThread
	 */
	void startSageApp(const quint64 sageappid,QString appName, int protocol, int port, const QRect initRect);


	/*!
	  * connected to UiServer's registerApp signal.
	  This slot is connected to fsManager::sailConnected(). Thus manually calling this slot will do nothing.
	  */
	void startApp(MEDIA_TYPE type, QString filename, qint64 filesize=0, QString senderIP="127.0.0.1", QString recvIP="", quint16 recvPort=0);


	/*!
	  * draw pointer (QGraphicsPolygonItem) on the scene when UiServer receives POINTER_SHARE message
	  */
	QGraphicsItem * createPointer(quint64 uiclientid, QColor pointerColor, QString pointerName);


	/*!
	  restore geometry (geometry ONLY) of widgets.
	  This function doesn't create apps.
	  */
	void loadSavedSession(const QString sessionFile);

	/*!
	  run a scenario
	  This function should be running in a separate thread
	  */
	void loadSavedScenario(const QString scenarioFile);


	void writePrelimDataHeader(const QString str);

};

#endif // GRAPHICSVIEWMAINWINDOW_H
