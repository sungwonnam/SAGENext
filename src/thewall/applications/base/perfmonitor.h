#ifndef PERFMONITOR_H
#define PERFMONITOR_H

#include "basewidget.h"

#include <QtGlobal>
#include <QtCore>


class PerfMonitor : public QObject
{
	Q_OBJECT

public:
	explicit PerfMonitor( QObject *parent=0);
//	~PerfMonitor();

	/*!
	  * This function measures delay between subsequent frame receivings
	  * recvTimer.restart() is called here
	  * recvTimer.start() must be called somewhere before calling this function
	  * It is converted to second and
	  * all the private member variable is set
	  */
//	void updateRecvLatency(ssize_t read, struct rusage rus, struct rusage rue);


	/*!
	  physical recv latency + delay enforced by scheduler
	  This is observed frame recv rate
	  */
	void updateObservedRecvLatency(ssize_t read, qreal netlatency, struct rusage rus, struct rusage rue);

	/*!
	  Increments updateCount and measure currDispFps, calculate avgDispFps
	  */
	void updateDispFps();


	/*!
	  Increments drawCount, and measure currDrawLatency, calculate avgDrawLatency.
	  Returns currDrawLatency on success, -1.0 on error
	  */
	qreal updateDrawLatency();

	qreal updateConvDelay();

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






	/*!
	  resets all data except expectedFps
	  */
	void reset();

	inline void setWidgetType(BaseWidget::Widget_Type wt) {widgetType = wt;}





	inline QTime & getRecvTimer() { return recvTimer; }
	inline QTime & getDrawTimer() { return drawTimer; }
	inline QTime & getDispTimer() { return dispTimer; }
	inline QTime & getConvTimer() { return convTimer; }
	inline QTime & getEqTimer() {return eqTimer;}

	inline quint64 getRecvCount() const {return recvFrameCount;}
	inline quint64 getDrawCount() const {return drawCount;}

	inline qreal getCurrRecvLatency() const { return currRecvLatency; }
	inline qreal getAvgRecvLatency() const { return avgRecvLatency; }

	inline qreal getCurrRecvFps() const { return currRecvFps; }
	inline qreal getAvgRecvFps() const { return avgRecvFps; }
//	inline qreal getPeakFps() const { return peakRecvFps; }

	inline void setExpectedFps(qreal f) {expectedFps = f;}
	inline qreal getExpetctedFps() const {return expectedFps;}



	inline qreal getCurrAbsDeviation() const {return currAbsDeviation;}
	inline qreal getAvgAbsDeviation() const {return avgAbsDeviation;}

	inline qreal getCurrAdjDeviation() const {return currAdjDeviation;}

	inline qreal getRecvFpsVariance() const {return recvFpsVariance;}
	inline qreal getRecvFpsStdDeviation() const {return recvFpsStdDeviation;}


	inline qreal getCurrDispFps() const { return currDispFps;}
	inline qreal getAvgDispFps() const {return avgDispFps;}

	inline qreal getCurrBandwidthMbps() const {return currBandwidth;}

	inline qreal getCurrDrawLatency() const { return currDrawLatency; }
	inline qreal getAvgDrawLatency() const { return avgDrawLatency; }

	inline qreal getCurrConvDelay() const {return currConvDelay;}
	inline qreal getAvgConvDelay() const {return avgConvDelay;}

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
	BaseWidget::Widget_Type widgetType;

	/*!
	  A QTimer object to measure delay of recv() in stream receiver.
	  This doesn't include swapBuffer delay
	  */
	QTime recvTimer;

	/*!
	  * This counter increments whenever a frame received. This tells how many frames have received from network.
	  */
	quint64 recvFrameCount;

	/*!
	  * network receiving delay (Only recv() latency) in second
	  */
	qreal currRecvLatency;
	qreal avgRecvLatency;
	qreal aggrRecvLatency;

	/*!
	  * Tells frame receiving rate. This is not actual display frame rate.
	  */
	qreal currRecvFps;
	qreal aggrRecvFps;
	qreal avgRecvFps;

	/*!
	  This is calculated from observed frame rate.
	  in Mbps
	  */
	qreal currBandwidth;






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
	  * latency of drawing using the painter in the paintEvent
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
	quint64 updateCount;

	/*!
	  This tells the rate at which widget's(streaing app's) update event is DISPATCHED from main event queue through event dispatcher
	  */
	qreal currDispFps;
	qreal aggrDispFps;
	qreal avgDispFps;






	/*!
	  A QTime object to measure pixmap conversion delay in streaming widget.
	  Pixels are downloaded into QImage buffer by stream receiver (SagePixelReceiver).
	  It is then converted to QPixmap (SageStreamWidget::updateWidget()) to minimize painting latency.

	  Note that this conversion latency corresponds to texture downloading (from main mem to VRAM) delay.
	  */
	QTime convTimer;

	quint64 convCount;
	qreal currConvDelay;
	qreal aggrConvDelay;
	qreal avgConvDelay;




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
	  framerate field in the sail register msg.
This is what sender wants ideally, so this can be different from the actual sending rate.
	  */
	qreal expectedFps;
	qreal adjustedFps;

	/*!
	  The current absolute deviation from the expected fps.
	  The observed(measured) fps is from currRecvFps (which is not display frame rate)
	  */
	qreal currAbsDeviation;
	qreal aggrAbsDeviation;
	qreal avgAbsDeviation;

	/*!
	  current frame rate deviation from the adjusted fps
	  */
	qreal currAdjDeviation;





	/*!
	  Recv FPS variance, standard deviation
	  */
	qreal aggrRecvFpsVariance;
	qreal recvFpsVariance;
	qreal recvFpsStdDeviation;




	/*!
	  timestamp in seconds
	  */
	qreal _ts_currframe;
	qreal _ts_nextframe;

	/*!
	  how many seconds missed
	  */
	qreal _deadline_missed;




	/**
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
