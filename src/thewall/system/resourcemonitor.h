#ifndef RESOURCEMONITOR_H
#define RESOURCEMONITOR_H

//#include "../common/commondefinitions.h"
//#include "../common/commonitem.h"

#include <QtGui>

class QSettings;
class AffinityInfo;
class SN_TheScene;
class SN_SimpleTextItem;
class SN_BaseWidget;
class SN_RailawareWidget;
class SN_SchedulerControl;
class SN_PriorityGrid;
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
	int prioritySum();

	/*!
	  print every information on this processor
	  */
	void printOverhead();


	enum PROC_NODE_TYPE { NULL_NODE, ROOT_NODE, NUMA_NODE, PHY_CORE, LOG_CORE, SMT_CORE, HPT_CORE };
private:
	int nodeType;

	/*!
	  core id seen by OS
	  */
	int id;

	/*!
	  If it's SMT core there must be sibling who shares L1 cache
	  */
	int smt_sibling_id;

	/*!
	  list of apps running on this cpu
	  */
	QList<SN_RailawareWidget *> *_appList;

	/*!
	  ::refresh() updates below members with perf->curr____()
	  */
	qreal cpuUsage;

	qreal bandwidth;

	QReadWriteLock _rwlock;
};









class SN_SimpleProcNode : public QObject
{
	Q_OBJECT
public:
	SN_SimpleProcNode(int procnum, QObject *parent=0);

	inline int procNum() const {return _procNum;}

	inline qreal cpuUsage() const {return _cpuUsage;}
	inline qreal netBWUsage() const {return _netBWusage;}
	inline int numWidgets() const {return _numWidgets;}

	inline void setCpuUsage(qreal v) {_cpuUsage = v;}
	inline void setNetBWUsage(qreal v) {_netBWusage = v;}
	inline void setNumWidgets(int v) {_numWidgets = v;}

	qreal addCpuUsage(qreal v) {_cpuUsage += v; return _cpuUsage;}
	qreal addNetBWUsage(qreal v) {_netBWusage += v; return _netBWusage;}
	int addNumWidgets(int i) {_numWidgets += i; return _numWidgets;}

private:
	/*!
	  physical processor id
	  */
	int _phyid;

	/*!
	  physical core id
	  A quad core processor will have four cores
	  And a core will have two processors if it's SMT
	  */
	int _coreid;

	/*!
	  The processor number seen by OS
	  */
	int _procNum;

	int _numWidgets;

	/*!
	  aggregated % cpu usage of worker threads of applications on this node
	  */
	qreal _cpuUsage;

	/*!
	  aggregated network bandwidth usage of applications on this node
	  */
	qreal _netBWusage;
};












/*!
  ResourceMonitor has a list of ProcessorNode classes and responsible for updating widget's processor affinity info.
  */
class SN_ResourceMonitor : public QObject
{
	Q_OBJECT
public:
//	explicit ResourceMonitor( const quint64 gaid,const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wf = 0);
	explicit SN_ResourceMonitor(const QSettings *s, SN_TheScene *scene, QObject *parent=0);
	~SN_ResourceMonitor();

	inline QVector<SN_ProcessorNode *> * getProcVec() const {return procVec;}
	inline QList<SN_SimpleProcNode *> getSimpleProcList() const {return _simpleProcList;}

	inline Numa_Info * getNumaInfo() const {return numaInfo;}


	void setScheduler(SN_SchedulerControl *sc);

	inline void setPriorityGrid(SN_PriorityGrid *p) {_pGrid = p;}

	void setRMonWidget(ResourceMonitorWidget *rmw);
	inline ResourceMonitorWidget * rMonWidget() {return _rMonWidget;}

	/*!
	  returns a copy of the widget list.
	  */
	inline QList<SN_BaseWidget *> getWidgetList() const {return widgetList;}

    /*!
      returns a reference of the list
      */
    inline QList<SN_BaseWidget *> * getWidgetListRef() {return &widgetList;}

	inline QReadWriteLock * getWidgetListRWLock() {return &_widgetListRWlock;}


    inline qreal totalBandwidthMbps() const {return _totalBWAchieved_Mbps;}



	/*!
	  Calls ProcessorNode::refresh() for each processorNode object. This function is called periodically in GraphicsViewMain::TimerEvent()
	  */
	void refresh();

	/*!
	  returns current most underloaded processor
	  */
	SN_ProcessorNode * getMostUnderloadedProcessor();

	/*!
	  returns the processNode with given processorId
	  */
	SN_ProcessorNode * processor(int pid);

	/*!
	  writer's lock needs to be acquired
	  */
	void addSchedulableWidget(SN_BaseWidget *rw);

	/*!
	  writer's lock needs to be acquired
	  */
	void removeSchedulableWidget(SN_BaseWidget *rw);

	/*!
	  return a widget with earliest deadline in the list
	  reader's lock
	  */
//	SN_RailawareWidget * getEarliestDeadlineWidget();


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

	QList<SN_SimpleProcNode *> _simpleProcList;

	const QSettings *settings;

	SN_TheScene *_theScene;

	/*!
	  A pointer to the scheduler control object
	  */
	SN_SchedulerControl *_schedcontrol;

	/*!
	  Priority Grid aka priority heatmap
	  */
	SN_PriorityGrid *_pGrid;


//	ProcessorNode * findNode(quint64 appid);

	Numa_Info *numaInfo;
//	QGraphicsSimpleTextItem *numaInfoTextItem;

//	void buildProcTree();


    /*!
      The sum of the bandwidth achieved by all the schedulable applications at given time.
      So, this is Max of the sum of the rw->_perfMon->getCurrBandwidthMbps()
      */
    qreal _totalBWAchieved_Mbps;


	/*!
	  An array index represents cpu id seen by OS
	  */
//	void buildProcVector();

	void buildSimpleProcList();


	/*!
	  A list of all schedulable widgets.
	  Accessing to this list is protected by rwlock
	  */
	QList<SN_BaseWidget *> widgetList;

	/*!
	  The items in a QMap is sorted by a key (Ascendant order)

      Key is globalAppId
	  */
	QMap<quint64, SN_BaseWidget *> _widgetMap;

	/*!
	  read/write lock for accessing widgetList
	  */
	QReadWriteLock _widgetListRWlock;


	/*!
	  Widget to show data
	  */
	ResourceMonitorWidget *_rMonWidget;


	/*!
	  write all data to a file ?
	  This was for the exp. data for the prelim exam
	  */
	bool _printDataFlag;

	/*!
	  print prelimData... writes on this
	  */
	QFile _dataFile;

    QTextStream _dataTextOut;

signals:
	/*!
	  This signal is emiited in ResourceMonitor::removeApp()
	  And is connected to the slot SagenextScheduler::loadBalance()
	  */
	void appRemoved(int procid);

    void dataRefreshed();

public slots:
	/*!
	  This function is called at RailawareWidget::fadeOutClose(), it calls ProcessorNode::removeApp()
	  The signal appRemoved(int) is emitted in this function.
	  */
//	void removeApp(SN_BaseWidget *);

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
//	int assignProcessor(SN_RailawareWidget *rw, SN_ProcessorNode *pn);

	/*!
	  Assign the most under loaded processor to rw.
	  returns assigned processor id, -1 on error.
	  */
//	int assignProcessor(SN_BaseWidget *rw);

	/*!
	  overloaded function.
	  assign a processor for each widget in the widgetList
	  returns number of assigned widgets -1 on error
	  */
//	int assignProcessor();


	/*!
	  set processor affinity for rw to FFFFFFFFF
	  */
	void resetProcessorAllocation(SN_BaseWidget *rw);

	/*!
	  overloaded function. Assign FFFF for all widgets.

	  This function will acquire _widgetListRWlock for read
	  */
	void resetProcessorAllocation();


	/*!
	  This is experimental and incomplete
	  */
//	void loadBalance();

	/*!
	  print perf data header
	  This slot shoud be called once
	  */
	void printPrelimDataHeader();


    /*!
      Create a file
      and set the _printDataFlag so that data can be written to the file
      */
    bool setPrintFile(const QString &filepath);

    void stopPrintData();

	/*!
	  print perf data for SAGENext paper.
	  This slot should be called periodically
	  */
	void printPrelimData();


    /*!
      It prints data (priority|winsize) for each app separated by ','
      Note that each column X in a line represents priority and window size of the application with global app id X
      */
	void printData_AppPerColumn(QTextStream &tout);


	void closeDataFile();
};

#endif // RESOURCEMONITOR_H
