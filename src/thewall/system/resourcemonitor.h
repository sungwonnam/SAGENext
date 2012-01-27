#ifndef RESOURCEMONITOR_H
#define RESOURCEMONITOR_H

//#include "../common/commondefinitions.h"
//#include "../common/commonitem.h"


#include <QtGui>
#include <QList>
#include <QVector>

#include "prioritygrid.h"

class QSettings;
class AffinityInfo;
class SN_SimpleTextItem;
class SN_BaseWidget;
class SN_RailawareWidget;
class SN_SchedulerControl;
//class GraphicsViewMain;
class ResourceMonitorWidget;

typedef struct {
	quint64 numa_hit; // request made by me served correctly
	quint64 numa_miss; // request made to others but I served
	quint64 numa_foreign; // request that I made served by others
	quint64 local_node;
	quint64 other_node;

	quint64 hit;
	quint64 miss;
	quint64 foreign;
	quint64 local;
	quint64 other;

} Numa_Info;



/*!
  ProcessorNode class maintains a list of pointers to railaware widgets on a processor that this class represents
  */
class SN_ProcessorNode
{
public:
	SN_ProcessorNode(int type, int id);
	~SN_ProcessorNode();

	inline void setNodeType(int t) {nodeType = t;}
	inline void setID(int i) {id = i;}

		inline void setSMTSiblingID(int i) {smt_sibling_id = i;}

//	int addChild(ProcessorNode *c);
//	int addSibling(ProcessorNode *s);

	inline int getNodeType() const {return nodeType;}
	inline int getID() const {return id;}
		inline int getSMTSiblingID() const {return smt_sibling_id;}

	inline void setCpuUsage(qreal r) {cpuUsage = r;}
	inline qreal getCpuUsage() const {return cpuUsage;}

	inline void setBW(qreal r) {bandwidth=r;}
	inline qreal getBW() const {return bandwidth;}

	int getNumWidgets() const;

	inline QList<SN_RailawareWidget *> * appList() {return _appList;}

	void addApp(SN_RailawareWidget *app);
	bool removeApp(quint64 appid);
	bool removeApp(SN_RailawareWidget *);

	void refresh();

	/*!
	  returns SUM(_priorityQuantized) of all apps on this processor.
	  This function assumes BaseWidget::_priorityQuantized had been already set !
	  */
	int prioritySum() const;

	/*!
	  print every information on this processor
	  */
	void printOverhead() const;


	enum PROC_NODE_TYPE { NULL_NODE, ROOT_NODE, NUMA_NODE, PHY_CORE, LOG_CORE, SMT_CORE, HPT_CORE };
private:
	int nodeType;

		/*!
	  * core id seen by OS
	  */
	int id;

		/*!
		  If it's SMT core there must be sibling who shares L1 cache
		  */
		int smt_sibling_id;

		/*!
	  * list of apps running on this cpu
	  */
	QList<SN_RailawareWidget *> *_appList;

	/*!
	  ::refresh() updates below members with perf->curr____()
	  */
	qreal cpuUsage;
	qreal bandwidth;
};






/*!
  ResourceMonitor has list of ProcessorNode classes and responsible for updating widget's processor affinity info.
  */
class SN_ResourceMonitor : public QObject
{
	Q_OBJECT
public:
//	explicit ResourceMonitor( const quint64 gaid,const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wf = 0);
	explicit SN_ResourceMonitor(const QSettings *s, QGraphicsScene *scene, QObject *parent=0);
	~SN_ResourceMonitor();

	inline QVector<SN_ProcessorNode *> * getProcVec() const {return procVec;}
	inline Numa_Info * getNumaInfo() const {return numaInfo;}

//	inline void addROIRectF(ROIRectF *f) {roiRectFs.push_back(f);}


	/*!
	  Calls ProcessorNode::refresh() for each processorNode object. This function is called periodically in GraphicsViewMain::TimerEvent()
	  */
	void refresh();

//	inline QTreeWidget * getProcTree() {return procTree;}

	/*!
	  returns current most underloaded processor
	  */
	SN_ProcessorNode * getMostUnderloadedProcessor();

	SN_ProcessorNode * processor(int pid);

	/*!
	  writer's lock
	  */
	void addSchedulableWidget(SN_RailawareWidget *rw);

	/*!
	  writer's lock
	  */
	void removeSchedulableWidget(SN_RailawareWidget *rw);

	/*!
	  return a widget with earliest deadline in the list
	  reader's lock
	  */
	SN_RailawareWidget * getEarliestDeadlineWidget();

	/*!
	  returns a copy of list
	  */
	inline QList<SN_RailawareWidget *> getWidgetList() {return widgetList;}

	inline void setScheduler(SN_SchedulerControl *sc) {schedcontrol = sc;}

	inline void setRMonWidget(ResourceMonitorWidget *rmw) {_rMonWidget = rmw;}
	inline ResourceMonitorWidget * rMonWidget() {return _rMonWidget;}


	/*!
	  this was for prelim exam
	  */
	inline bool printDataFlag() const {return _printDataFlag;}
	inline void setPrintDataFlag(bool b = true) {_printDataFlag = b;}


	/*!
	  Heat map of the wall
	  */
	PriorityGrid *_priorityGrid;

protected:
	/*!
	  calls this->refresh()
	  and rmonitorWidget->refresh()

	  The timer of this object if started at the main.cpp
	  */
	void timerEvent(QTimerEvent *);


private:
	/*!
	  A list of processors (seen by OS) in this system. Initialized by buildProcVector()
	  */
	QVector<SN_ProcessorNode *> *procVec;

	const QSettings *settings;

	QGraphicsScene *_theScene;

	/*!
	  A pointer to the scheduler control object
	  */
	SN_SchedulerControl *schedcontrol;


//	ProcessorNode * findNode(quint64 appid);

	Numa_Info *numaInfo;
//	QGraphicsSimpleTextItem *numaInfoTextItem;

//	void buildProcTree();

	/*!
	  An array index represents cpu id seen by OS
	  */
	void buildProcVector();


	/*!
	  A list of all schedulable widgets.
	  Accessing to this list is protected by rwlock
	  */
	QList<SN_RailawareWidget *> widgetList;

	/*!
	  read/write lock for accessing widgetList
	  */
	QReadWriteLock widgetListRWlock;


	/*!
	  ROI rectangle container
	  */
//	QList<ROIRectF *> roiRectFs;


	/**
	  Widget to show data
	  */
	ResourceMonitorWidget *_rMonWidget;


	/**
	  write all data to a file ?
	  This was for the exp. data for the prelim exam
	  */
	bool _printDataFlag;

	/**
	  print prelimData... writes on this
	  */
	QFile _dataFile;


signals:
	/*!
	  This signal is emiited in ResourceMonitor::removeApp()
	  And is connected to the slot SagenextScheduler::loadBalance()
	  */
	void appRemoved(int procid);

public slots:
	/*!
	  This function is called at RailawareWidget::fadeOutClose(), it calls ProcessorNode::removeApp()
	  The signal appRemoved(int) is emitted in this function
	  */
	void removeApp(SN_RailawareWidget *);

	/*!
	  AffinityInfo class has functions that will emit affInfoChanged signal. AffinityInfo::setCpuOfMine(), AffinityInfo::applyNewParameter()
	  This signal is connected to this slot at the GraphicsViewMain::startSageApp()

	  Using the affinity info an app passed, app pointer is added/removed to/from processorNode object
	  */
	void updateAffInfo(SN_RailawareWidget *, int oldvalue, int newvalue);


	/*!
	  BaseWidget::priorityChanged signal is connected with this slot in RailawareWidget constructor
	  */
//	void rearrangeWidgetMultiMap(BaseWidget *, int oldpriority);

	/*!
	  Assign a processor pn to rw
	  */
	int assignProcessor(SN_RailawareWidget *rw, SN_ProcessorNode *pn);

	/*!
	  Assign the most under loaded processor to rw.
	  returns assigned processor id, -1 on error.
	  */
	int assignProcessor(SN_RailawareWidget *rw);

	/*!
	  overloaded function.
	  assign a processor for each widget in the widgetList
	  returns number of assigned widgets -1 on error
	  */
	int assignProcessor();


	/*!
	  set processor affinity for rw to FFFFFFFFF
	  */
	void resetProcessorAllocation(SN_RailawareWidget *rw);

	/*!
	  overloaded function.
	  Assign FFFF for all widgets
	  */
	void resetProcessorAllocation();


	void loadBalance();

	/**
	  print perf data header
	  This slot shoud be called once
	  */
	void printPrelimDataHeader();

	/*!
	  print perf data for SAGENext paper.
	  This slot should be called periodically
	  */
	void printPrelimData();
};

#endif // RESOURCEMONITOR_H
