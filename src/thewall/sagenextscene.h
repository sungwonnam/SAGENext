#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>



class UiServer;
class ResourceMonitor;
class SchedulerControl;


class SAGENextScene : public QGraphicsScene
{
	Q_OBJECT
public:
	explicit SAGENextScene(QObject *parent = 0);
	~SAGENextScene();

	inline void setUiServer(UiServer *u) {_uiserver = u;}
	inline void setRMonitor(ResourceMonitor *r) {_rmonitor = r;}
	inline void setSchedControl(SchedulerControl *s) {_schedcontrol = s;}

private:
	UiServer *_uiserver;
	ResourceMonitor *_rmonitor;
	SchedulerControl *_schedcontrol;

signals:

public slots:
	void closeAllUserApp();

	/**
	  kills UiServer, fsManager, ResourceMonitor
	  */
	void prepareClosing();

};

#endif // SAGENEXTSCENE_H
