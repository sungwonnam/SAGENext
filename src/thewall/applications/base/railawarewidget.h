#ifndef RAILAWAREWIDGET_H
#define RAILAWAREWIDGET_H

#include "basewidget.h"

class AffinityInfo;
class AffinityControlDialog;

class SN_SchedulerControl;

class QMutex;
class QWaitCondition;

class SN_RailawareWidget : public SN_BaseWidget
{
	Q_OBJECT
//	Q_PROPERTY(qreal priority READ priority WRITE setPriority)

public:
        /*!
          It is user's responsibility to create Affinity related object by calling createAffInstances() when using default constructor
          */
	SN_RailawareWidget();

        /*!
          It is higly recommended to you this constructor instead of default one
          */
	SN_RailawareWidget(quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rmonitor, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);

	virtual ~SN_RailawareWidget();


        /*!
          In this function, AffinityInfo::cpuOfMineChanged() signal is connected to ResourceMonitor::updateAffInfo() slot if ResourceMonitor object exist. This connection ensures resourceMonitor to maintain up to date info on which app is affine to which processor.

          If scheduler also exists, Scheduler::assignProcessor() followed by AffinityInfo::setReadyBit() are called.
          */
	void createAffInstances();


        /*!
          This will determine delay in stream loop
          */
//	int setQuality(qreal newQuality);

		/**
		  Returns *absolute* observed quality which is based on expected quality set by a user
		  */
//	qreal observedQuality();

		/**
		  Returns *relative* observed quality which is based on adjusted quality
		  */
//	qreal observedQualityAdjusted();


	inline bool isScheduled() const {return _scheduled;}
	inline void setScheduled(bool b) { _scheduled = b;}


	inline bool widgetClosed() const {return _widgetClosed;}
	inline void setWidgetClosed(bool b = true) {_widgetClosed = b;}

        /*!
          SMART scheduler
          */
	int failToSchedule;


protected:
        /*!
          AffinityInfo class. Only railaware widget will instantiate this
          */
	AffinityControlDialog *affCtrlDialog;

        /**
          QAction that connects context menu's item to showAffCtrlDialog()
          */
	QAction *_affCtrlAction;

//        SchedulerControl *_scheduler;


        /**
          When a scheduler is running, schedulable widget should be deleted by the scheduler.
		  The scheduler copies widget list from the resource monitor at every scheduling event.
          Because of this, the scheduler can still have pointer to this widget after this widget has closed.
          */
	bool _widgetClosed;


	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);


        /**
          A flag for SMART scheduler.
		  This is to prevent the widget from scheduled when previously scheduled frame hasn't displayed.
		  Sage widget unset this flag in the scheduleUpdate()
          */
	bool _scheduled;

signals:

public slots:
	void showAffCtrlDialog();

		/**
		  Railaware widget will usually have a worker thread. For sage widget it is pixel receiving thread.
		  The worker thread signals when a frame is ready to be displayed. And this signal can be connected to
		  scheduleUpdate() to do additional process before scheduling update()
		  */
	virtual void scheduleUpdate() { update(); }

	virtual void scheduleReceive() {}
};

#endif // ANIMATIONWIDGET_H
