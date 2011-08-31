#ifndef SAGENEXTSCHEDULER_H
#define SAGENEXTSCHEDULER_H

#include <QtGui>
#include <QObject>
//#include <QFutureWatcher>
//#include <QList>
#include <QThread>

#include <QTimer>

#include <QReadWriteLock>

class QEvent;
class RailawareWidget;
class QGraphicsView;
class QGraphicsScene;
class ResourceMonitor;
class ProcessorNode;
class AbstractScheduler;

/*!
  General key concepts in multimedia scheduling includes

  Hierarchical scheduling
  Proportional sharing, Fair queueing
  EDF


  In Multiprocessor system,
  allocating multiple processing units to task group
  and applying EDF, proportional sharing algorithm for each group
  */
class SchedulerControl : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool scheduleEnd READ scheduleEnd WRITE setScheduleEnd)
	Q_ENUMS(Scheduler_Type)

public:
	explicit SchedulerControl(ResourceMonitor *rm, QObject *parent = 0);
	~SchedulerControl();

	/*!
	  qApp installs this eventFilter  using qApp->installEventFileter(SagenextScheduler *)
	  This function is used to filter all the events of qApp
	  */
	bool eventFilter(QObject *, QEvent *);

	inline void setGView(QGraphicsView *gv) {gview = gv;}

	inline void setScheduleEnd(bool v) {_scheduleEnd = v;}
	inline bool scheduleEnd() const {return _scheduleEnd;}

	enum Scheduler_Type {DividerWidget, SelfAdjusting, DelayDistribution, SMART};

	inline void setSchedulerType(SchedulerControl::Scheduler_Type st) {schedType = st;}

	int launchScheduler(SchedulerControl::Scheduler_Type st, int msec=1000);
	int launchScheduler(const QString &str, int msec=1000);
	int launchScheduler();

	void killScheduler();

	inline bool isRunning() const {return _isRunning;}

	inline AbstractScheduler * scheduler() {return _scheduler;}

private:
	QGraphicsView *gview;
	ResourceMonitor *resourceMonitor;

	bool _scheduleEnd;

	Scheduler_Type schedType;

	/*!
	  in msec
	  */
	int granularity;

//	QList<AbstractScheduler *> schedList;

	/*!
	  scheduler
	  */
	AbstractScheduler *_scheduler;

	bool _isRunning;


	/*!
	  control panel GUI
	  */
	QFrame *controlPanel;

signals:

public slots:
};


/*!
  Abstract scheduler class
  */
class AbstractScheduler : public QThread {
	Q_OBJECT
//	Q_PROPERTY(bool end READ isEnd WRITE setEnd)

public:
	explicit AbstractScheduler(ResourceMonitor *rmon, int granularity=2, QObject *parent=0) : QThread(parent), proc(0), rMonitor(rmon), _granularity(granularity) {  }
	virtual ~AbstractScheduler();

//	inline bool isEnd() const {return _end;}
//	void setEnd(bool b = true);

	QTimer timer;

	inline int frequency() const {return _granularity;}

protected:
	/*!
	  calls doSchedule every x msec. x is defined by granularity
	  */
	virtual void run();

	/*!
	  rail configuration
	  */
	virtual int configureRail(RailawareWidget *rw, ProcessorNode *pn);

	virtual int configureRail(RailawareWidget *rw);

	virtual int configureRail();

	/*!
	  used for per processor scheduler
	  */
	ProcessorNode *proc;

	/*!
	  pointer to resource monitor
	  */
	ResourceMonitor *rMonitor;
//	bool _end;

	int _granularity; // in microsecond

	QGraphicsScene *scene;

	qint64 currMsecSinceEpoch;

private slots:
	virtual void doSchedule() = 0;
	void applyNewInterval(int);
};





/*!
  Partial implementation of SMART scheduler [Nieh and Lam 2003]
  */
class SMART_EventScheduler : public AbstractScheduler {
	Q_OBJECT
public:
	explicit SMART_EventScheduler(ResourceMonitor *r, int granularity=2, QObject *parent=0);
//	~SMART_EventScheduler();

protected:
//	void run();

private:
	/*!
	  Scheduler always schedule from workingSet->top() until it isn't empty().
	  Item in this container is sorted by deadline (ascending order)
	  */
	QList<RailawareWidget *> workingSet;

	/*!
	  From the candidate set (processorNode->appList()) which is sorted by importance (priority, BVFT tuple),
	  a new job (a new event) is inserted into working set in ascending earlist deadline order
	  only when the new job wouldn't make more important jobs already in the working set miss their deadlines
	  */
	int insertIntoWorkingSet(RailawareWidget *, qreal now);

private slots:
	/*!
	  iterate over workingSet which is sorted by deadline
	  */
	void doSchedule();
};


/*!
  Resources are taken away from widget that has lower pirority than the fiducial widget
  */
class DividerWidgetScheduler : public AbstractScheduler {
	Q_OBJECT
public:
	explicit DividerWidgetScheduler(ResourceMonitor *r, int granularity = 1000, QObject *parent=0);

private:
	/*!
	  How often schedule will check to adjust deviation in millisecond
	  */
	int examine_interval;
	int count;

	qint64 currMsecSinceEpoch;

	/*!
	  readers lock for scheduler
	  writers lock for clients
	  */
	qreal _Q_highest;
	qreal _P_fiducial;
	qreal _Q_fiducial;


	QReadWriteLock rwlock;


	qreal totalRequiredResource;
	qreal totalRedundantResource;


	RailawareWidget *fiducialWidget;

public:
	void setQHighest(qreal qh) ;
	void setPAnchor(qreal pa) ;
	void setQAnchor(qreal qa);

signals:
	void QHighestChanged(qreal);
	void PAnchorChanged(qreal);
	void QAnchorChanged(qreal);


private slots:
	void doSchedule();
};

/*!
  Each widget shall adjust its quality by comparing its quality with others.

  **************************************************
  This is to achieve NON-LINEAR proportional sharing.
  It is more important to make higher priority widget shows better quality
   than proportionally distribute resources to lower priority apps
  ***************************************************

  */
class SelfAdjustingScheduler : public AbstractScheduler {
	Q_OBJECT
public:
	explicit SelfAdjustingScheduler(ResourceMonitor *r, int granularity = 500, QObject *parent=0);

	inline qreal getQTH() const {return qualityThreshold;}
	inline qreal getDecF() const {return decreasingFactor;}
	inline qreal getIncF() const {return increasingFactor;}

private:

	/*!
	  Sensitivity.

	  Scheduler will do nothing if an app's quality meets this quality threshold
	  */
	qreal qualityThreshold;

	/*!
	  Aggression.

	  How much an app will decrease its quality for higher priority apps
	  How much an app will decrease other's quality for itself
	  */
	qreal decreasingFactor;

	/*!
	  Greediness.

	  How much an app will increase its quality when it thinks it seems ok to increase
	  */
	qreal increasingFactor;

	/*!
	  mutex variable to synchronize above variables
	  */
	QReadWriteLock rwlock;

	/*!
	  negative value : decreasing phase
	  positive value : increasing phase
	  */
	int phaseToken;

private slots:

	/*!
	  Implementing pure virtual function declared in the parent
	  */
	void doSchedule();

	/*!
	  An interface to the qualityThreshold
	  */
	void applyNewThreshold(int);

	/*!
	  An interface to the decreasingFactor
	  */
	void applyNewDecF(int);

	/*!
	  An interface to the increasingFactor
	  */
	void applyNewIncF(int);
};







/*!
  Delay is distributed proportional to priority
  Proportional Fairness

  Quality * weight / SUM(weight);
  */
class DelayDistributionScheduler : public AbstractScheduler {
	Q_OBJECT
public:
	explicit DelayDistributionScheduler(ResourceMonitor *r, int granularity = 1000, QObject *parent=0);

private slots:
	void doSchedule();

};


#endif // SAGENEXTSCHEDULER_H
