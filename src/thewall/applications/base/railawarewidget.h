#ifndef RAILAWAREWIDGET_H
#define RAILAWAREWIDGET_H

#include "basewidget.h"


class AffinityInfo;
class AffinityControlDialog;

class SchedulerControl;

class QMutex;
class QWaitCondition;

class RailawareWidget : public BaseWidget
{
	Q_OBJECT
//	Q_PROPERTY(qreal priority READ priority WRITE setPriority)

public:
	/*!
	  It is user's responsibility to create Affinity related object by calling createAffInstances() when using default constructor
	  */
	RailawareWidget();

	/*!
	  It is higly recommended to you this constructor instead of default one
	  */
	RailawareWidget(quint64 globalappid, const QSettings *s, ResourceMonitor *rm,  QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);
		virtual ~RailawareWidget();

	/*!
	  Resource Monitor may want this to reschedule an app
	  */
	inline AffinityInfo * affInfo() {return _affInfo;}

	/*!
	  In this function, AffinityInfo::cpuOfMineChanged() signal is connected to ResourceMonitor::updateAffInfo() slot if ResourceMonitor object exist. This connection ensures resourceMonitor to maintain up to date info on which app is affine to which processor.

	  If scheduler also exists, Scheduler::assignProcessor() followed by AffinityInfo::setReadyBit() are called.
	  */
	void createAffInstances();


	/*!
	  This will determine delay in stream loop
	  */
	int setQuality(qreal newQuality);
	qreal observedQuality();
	qreal observedQualityAdjusted();

	qreal unitValue();


	bool isScheduled() const {return _scheduled;}
	void setScheduled(bool b) { _scheduled = b;}


	/*!
	  SMART scheduler
	  */
	int failToSchedule;


protected:
	/*!
	  AffinityInfo class. Only animation widget will instantiate this
	  */
//	AffinityInfo *affInfo;
	AffinityControlDialog *affCtrlDialog;

	QAction *_affCtrlAction;

	SchedulerControl *scheduler;





	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);


	/*!
	  flag for scheduler
	  */
	bool _scheduled;

signals:

public slots:
	void showAffCtrlDialog();

	/*!
	  overrides BaseWidget::fadeOutClose()
	  */
	virtual void fadeOutClose();


	/*!
	  */
	virtual void scheduleUpdate() { update(); }

	virtual void scheduleReceive() {}
};

#endif // ANIMATIONWIDGET_H
