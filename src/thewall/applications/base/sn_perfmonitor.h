#ifndef PERFMONITOR_H
#define PERFMONITOR_H

//#include "applications/base/sn_basewidget.h"

#include <QtGlobal>
#include <QtCore>

class SN_BaseWidget;

class SN_PerfMonitor : public QObject
{
	Q_OBJECT

public:
	explicit SN_PerfMonitor(QObject *parent=0);
//	~PerfMonitor();

	/*!
      This function is called by the widget.
      It assumes that this function is called at the end of every frame is received by the receiving thread of the widget.

      @param byteread : Byte received
      @param actualtime_sec : The time spent (in second), including any overhead in the receiving thread, in receiving the amount of 'byteread' Byte.
      @param cputime_sec : CPU time spent (in second) for receiving
	  */
//	void updateDataWithLatencies(ssize_t byteread, qreal actualtime_sec, qreal cputime_sec);

    /*!
      The application keeps updating _cumulativeByteReceived by calling this function.
      Optionally, the time(and CPU time) spent receiving a frame can be passed to calculate
      FPS and CPU usage.

      The _cumulativeByteReceived will be used to find out
      the current BW usage (in Mbps)
      */
    void addToCumulativeByteReceived(quint64 byte, qreal actualtime_sec = 0, qreal cputim_sec = 0);


    /*!
      This function is called by the resource monitor periodically.
      It assumes the widget keeps updating the _cumulativeByteReceived.

      This function calculates current BW and calls _updateBWdata() with the value
      */
    void updateDataWithCumulativeByteReceived(qint64 timestamp);


    /*!
      returns Current BW / Required BW or -1 if Required BW == 0
      */
    qreal observedQuality_Rq() const;

    /*!
      returns Current BW / Demanded BW by scheduler or -1 if Demanded BW == 0
      */
    qreal observedQuality_Dq() const;


	/*!
	  Increments updateCount and measure currDispFps, calculate avgDispFps
	  */
	void updateDispFps();


	/*!
      This function is used to measure drawing latency (in paint() function)
      Note than drawTimer.start() must be called before calling this function

	  It increments drawCount, update the currDrawLatency and the avgDrawLatency.

	  @return currDrawLatency on success, -1.0 on error
	  */
	qreal updateDrawLatency();

	/*!
      This function is used to measure widget's contents updating latency.
      Such as coping pixels to GPU memory (in SN_SageStreamWidget)
      reading new pdf page and render the page (in SN_PDFViewerWidget)

      Note that updateTimer must be started before calling this function.
	  */
	qreal updateUpdateDelay();


    /*!
      event queuing delay.
      This is not used
      */
	qreal updateEQDelay();



	/*!
	  return -1 if requested fps is too low
	  return 1 if requested fps is too high (higher than expected fps)
	  return 0 on success
	  */
	qreal setAdjustedFps(qreal f);

	/*!
	  This function is used in sagePixelReceiver to determine delay
	  */
	inline qreal getAdjustedFps() const {return adjustedFps;}


	void printData() const;



	/*!
	  resets all data except expectedFps
	  */
	void reset();

//	inline void setWidgetType(SN_BaseWidget::Widget_Type wt) {widgetType = wt;}


    /*!
      If the widget provides a priori
      */
    inline void setPriori(bool b = true) {_priori = b;}
    inline bool priori() const {return _priori;}

    inline quint64 cumulativeByteReceived() const {return _cumulativeByteRecved;}

    inline void setMeasureStartTime(qint64 t) {_measureStartTime = t;}
    inline qint64 measureStartTime() const {return _measureStartTime;}
    inline void setMeasureEndTime(qint64 t) {_measureEndTime = t;}
    inline qint64 measureEndTime() const {return _measureEndTime;}


//	inline QTime & getRecvTimer() { return recvTimer; }
	inline QTime & getDrawTimer() { return drawTimer; }
	inline QTime & getDispTimer() { return dispTimer; }
	inline QTime & getUpdtTimer() { return updateTimer; }
	inline QTime & getEqTimer() {return eqTimer;}

	inline quint64 getRecvCount() const {return _recvFrameCount;}
	inline quint64 getDrawCount() const {return drawCount;}

    /*!
      This is network latency (recv() syscalls)
      */
	inline qreal getCurrRecvLatency() const { return currRecvLatency; }
	inline qreal getAvgRecvLatency() const { return avgRecvLatency; }

    /*!
      This is 1 / (network latency + app's whatever delay)
      */
	inline qreal getCurrEffectiveFps() const { return _currEffectiveFps; }
	inline qreal getAvgEffectiveFps() const { return _avgEffectiveFps; }
//	inline qreal getPeakFps() const { return peakRecvFps; }

	inline void setExpectedFps(qreal f) {expectedFps = f;}
	inline qreal getExpetctedFps() const {return expectedFps;}



//	inline qreal getCurrAbsDeviation() const {return currAbsDeviation;}
//	inline qreal getAvgAbsDeviation() const {return avgAbsDeviation;}

//	inline qreal getCurrAdjDeviation() const {return currAdjDeviation;}

//	inline qreal getRecvFpsVariance() const {return recvFpsVariance;}
//	inline qreal getRecvFpsStdDeviation() const {return recvFpsStdDeviation;}


	inline qreal getCurrDispFps() const { return currDispFps;}
	inline qreal getAvgDispFps() const {return avgDispFps;}


    ///
    /// Parameters affecting Rw
    ///
    inline int wakeUpGuessFps() const {return _wakeUpGuessFps;}
    inline qreal overPerformMult() const {return _overPerformMultiplier;}
    inline qreal normPerformMult() const {return _normPerformMultiplier;}
    inline int underPerformEndur() const {return _underPerformEndurance;}

    inline void setWakeUpGuessFps(int fps) {_wakeUpGuessFps = fps;}
    inline void setOverPerformMult(qreal m) {_overPerformMultiplier = m;}
    inline void setNormPerformMult(qreal m) {_normPerformMultiplier = m;}
    inline void setUnderPerformEndur(int e) {_underPerformEndurance = e;}


    ///
    /// The Bandwidth
    ///

	inline qreal getCurrBW_Mbps() const {return _currEffectiveBW_Mbps;}

    /*!
      Override the value calculated in updateDataWithLatencies()
      */
//    inline void overrideCurrBW_Mbps(qreal r) {_currEffectiveBW_Mbps = r;}

    /*!
      Returns the required bandwidth to ensure the percentage of expected quality of the application.
      If percentage == 1.0 then the return value indicates the bandwidth required to make the application runs at full quality

      @param percentage must be between 0.0 <= percentage <= 1.0
      */
    qreal getRequiredBW_Mbps(qreal percentage = 1.0) const;


    /*!
      image streaming (SAGE) app : The image width * height * bpp * framerate (in Mbps)

      An interactive app (best-effort type) will set this to 0 when there's no interaction thereby the app is sitting idle
      */
    inline void setRequiredBW_Mbps(qreal b) {_requiredBW_Mbps = b;}

    inline void setMaxAchievedBW_Mbps(qreal b) {_maxBWachieved = b;}


    inline bool isInteracting() const {return _isInteracting;}
    inline void setInteracting(bool b=true) {_isInteracting = b;}





	inline qreal getCurrDrawLatency() const { return currDrawLatency; }
	inline qreal getAvgDrawLatency() const { return avgDrawLatency; }

	inline qreal getCurrUpdateDelay() const {return currUpdateDelay;}
	inline qreal getAvgUpdateDelay() const {return avgUpdateDelay;}

	inline qreal getCurrEqDelay() const {return currEqDelay;}
	inline qreal getAvgEqDelay() const {return avgEqDelay;}


	/*!
	  returns deadline missed
	  */
//	qreal set_ts_currframe();
//	qreal set_ts_currframe(qreal now);
//	inline void set_ts_nextframe(qreal r) {_ts_nextframe = r;}
//	inline qreal ts_currframe() const {return _ts_currframe;}
	inline qreal ts_nextframe() const {return _ts_nextframe;}
//	inline qreal deadline_miseed() const {return _deadline_missed;}

    inline qreal getCpuTimeSpent_sec() const {return _cpuTimeSpent_sec;}

	inline qreal getCpuUsage() const {return cpuUsage;}
//	inline long getStartNvcsw() const {return rustart_nvcsw;}
//	inline long getStartNivcsw() const {return rustart_nivcsw;}
//	inline long getStartInblock() const {return rustart_inblock;}
	inline long getEndNvcsw() const {return ruend_nvcsw;}
	inline long getEndNivcsw() const {return ruend_nivcsw;}
//	inline long getEndInblock() const {return ruend_inblock;}
	inline long getEndMaxrss() const {return ruend_maxrss;}
	inline long getEndMinflt() const {return ruend_minflt;}

private:
//	SN_BaseWidget::Widget_Type widgetType;
	SN_BaseWidget *_widget;

	qreal _overPerformAvg;
	qreal _overPerformAgg;
	qreal _overPerformCnt;

    /*!
      initial frame rate estimated when an application woken up from idling.

      Default 30
      */
    int _wakeUpGuessFps;

    /*!
      When an application over-performing (Rc > Rw), increase Rw
      Rw = _overPerformMultiplier * Rc

      Default 1.2
      */
    qreal _overPerformMultiplier;

    /*!
      When an application perform normal (Rc <= Rw), and its Dq is 1 then try increase Rw
      Rw = _normPerformMultiplier * Rc

      Default 1.1
      */
    qreal _normPerformMultiplier;

    /*!
      How many under-performance will be tolerated before lowering its Rw
      if (_underPerformCnt > _underPerformEndurance) then decrease Rw

      Default 4
      */
    int _underPerformEndurance;


	int _underPerformCnt;


    /*!
      Updates
      _currEffectiveBW
      _requiredBW
      _maxBWachieved
      */
    void _updateBWdata(qreal bw);


    /*!
      Whether this app provided a priori.
      constant frame rate periodic app will have this
      */
    bool _priori;

    qint64 _measureStartTime;
    qint64 _measureEndTime;


	/*!
	  * This counter increments whenever a frame received. This tells how many frames have received from network.
	  */
	quint64 _recvFrameCount;

	/*!
	  * network receiving delay (Only recv() latency) in second
	  */
	qreal currRecvLatency;
	qreal avgRecvLatency;
	qreal aggrRecvLatency;

	/*!
	  Tells frame receiving rate. This might not actual display frame rate.
	  This FPS is calculated with ObservedDelay.
	  ObservedDelay = network recv() delay + any other delay during frame receive
	  */
	qreal _currEffectiveFps;
	qreal _aggrEffectiveFps;
	qreal _avgEffectiveFps;

	/*!
	  This is calculated from observed frame rate. (Mbps)

	  The current bandwidth is calculated using byteRead (frame size in Byte) and ObservedDelay.
	  ObservedDelay = network recv() delay + any other delay during frame receive
	  */
	qreal _currEffectiveBW_Mbps;


    /*!
      The max BW achieved so far
      */
    qreal _maxBWachieved;



    /*!
      A required bandwidth to run the application in full quality.
      A image streaming application (such as SAGE app) that has desired fps can have this. This is called a priori.

      This can't be known for a best-effort application.
      */
    qreal _requiredBW_Mbps;

    /*!
      If Rq is increased, then we need to give the app some time to use the resources...
      */
    qint64 _isRqIncreased;


    /*!
      Cumulative Byte received
      */
    quint64 _cumulativeByteRecved;

    quint64 _prevByteReceived;
    qint64 _prevTimestamp;

    /*!
      key timestamp
      value cumulative Byte
      */
    QList< QPair<qint64, quint64> > _cumulativeByteRecvedList;

    /*!
      true if the widget is being interacted by user
      */
    bool _isInteracting;



	/*!
	  A QTimer object to measure BaseWidget::paint() latency of the widget.
	  This doesn't include image to pixmap converting delay in streaming app.
	  */
	QTime drawTimer;

	/*!
	  * This counter increments whenever paintEvent is called. This tells how many times a frame has drawn.
	  * Note that a single frame can be drawn multiple times because of user interaction.
	  */
	quint64 drawCount;

	/*!
	  * latency of drawing using the QPainter in the QGraphicsItem::paint()
	  */
	qreal currDrawLatency;
	qreal avgDrawLatency;
	qreal aggrDrawLatency;






	/*!
	  A QTimer object to measure the RATE at which item's(widget's) paint() is called.
	  In other words, it is how often event dispatcher dispatches item's update event.
	  */
	QTime dispTimer;

	/*!
	  This counter increment whenever SageStreamWidget::updateWidget() is called.
	  This tells how many times the update event has queued.
	  Note that Qt can merge multiple update event into one.
	  */
	quint64 updateDispCount;

	/*!
	  This tells the rate at which widget's(streaing app's) update event is DISPATCHED from main event queue through event dispatcher
	  */
	qreal currDispFps;
	qreal aggrDispFps;
	qreal avgDispFps;






	/*!
      Basically, it's content updating delay.

	  If OpenGL it's used to measure texture uploading delay.
	  If OpenGL PBO it's used to measure texture binding plus buffer mapping delay.
	  If no OpenGL it's used to measure the delay of converting QImage to QPixmap.

	  Note that in X11, QPixmap is stored at the X Server thus drawing QPixmap is cheap.
	  So, converting QImage to QPixmap means uploading image to X Server
	  */
	QTime updateTimer;

	quint64 updateCount;
	qreal currUpdateDelay;
	qreal aggrUpdateDelay;
	qreal avgUpdateDelay;




	/*!
	  Event queueing delay.

	  SageStreamWidget::updateWidget() queue update event after pixmap conversion.
	  */
	QTime eqTimer;

	quint64 eqCount;
	qreal currEqDelay;
	qreal aggrEqDelay;
	qreal avgEqDelay;











	/*!
	  framerate field in the sail register msg sets the expectedFps.
      This is what sender wants, so this can be different from the actual sending rate.
	  */
	qreal expectedFps;

	/*!
	  FPS adjusted(demanded) by the scheduler.
	  */
	qreal adjustedFps;

	/*!
	  The current absolute deviation from the expected fps.
	  The observed(measured) fps is from currRecvFps (which is not display frame rate)
	  */
//	qreal currAbsDeviation;
//	qreal aggrAbsDeviation;
//	qreal avgAbsDeviation;

	/*!
	  current frame rate deviation from the adjusted fps
	  */
//	qreal currAdjDeviation;





	/*!
	  Recv FPS variance, standard deviation
	  */
//	qreal aggrRecvFpsVariance;
//	qreal recvFpsVariance;
//	qreal recvFpsStdDeviation;




	/*!
	  timestamp in seconds
	  */
	qreal _ts_currframe;

	/*!
	  deadline for the next frame
	  */
	qreal _ts_nextframe;

	/*!
	  how many seconds missed
	  */
	qreal _deadline_missed;



    /*!
      Cpu time spent in kernel mode + Cpu time spent in user mode
      SysTime + UserTime (in second)
      */
    qreal _cpuTimeSpent_sec;

    /*!
      The CPU time required (in microsecond) to achieve 100% performance
      */
    qreal _cpuTimeRequired;

	/*!
      (Systime + UsrTime ) / observed_delay

	  * please multiply this by 100 to get percentage
	  */
	qreal cpuUsage;
//	long rustart_nvcsw;
//	long rustart_nivcsw;
//	long rustart_inblock;

	/**
	  * The number of times processes voluntarily invoked a context switch (usually to wait for some service).
	  */
	long ruend_nvcsw;

	/**
	  * The number of times an involuntary context switch took place (because a time slice expired, or another process of higher priority was scheduled).
	  */
	long ruend_nivcsw;

	/**
	  * The maximum resident set size used, in kilobytes. That is, the maximum number of kilobytes of physical memory that processes used simultaneously.
	  */
	long ruend_maxrss;

	/**
	  * The number of page faults which were serviced without requiring any I/O.  (page reclaims)
	  */
	long ruend_minflt;

	/**
	  * The number of page faults which were serviced by doing I/O.
	  */
//	long ruend_majflt;

	/**
	  * The number of times the file system had to read from the disk on behalf of processes.
	  */
//	long ruend_inblock;

	/**
	  *  The number of times the file system had to write to the disk on behalf of processes
	  */
//	long ruend_oublock;

	/**
	  * # of IPC messages sent
	  */
//	long ruend_msgsnd;

	/**
	  * # of IPC messages received
	  */
//	long ruend_msgrcv;

//	long ruend_nsignals;
};

#endif // PERFMONITOR_H
