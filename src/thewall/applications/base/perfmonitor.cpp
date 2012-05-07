#include "perfmonitor.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>

#include <stdio.h>


PerfMonitor::PerfMonitor(QObject *parent)
    : QObject(parent)
    , _widget(static_cast<SN_BaseWidget *>(parent))
    , _priori(false)

	, _recvFrameCount(0)
    , currRecvLatency(0.0)
    , avgRecvLatency(0.0)
    , aggrRecvLatency(0.0)

    , _currEffectiveFps(0.0)
    , _aggrEffectiveFps(0.0)
    , _avgEffectiveFps(0.0)

    , _currEffectiveBW(0.0)
    , _requiredBandwidth(0.0)

	, drawCount(0),
	currDrawLatency(0.0),
	avgDrawLatency(0.0),
	aggrDrawLatency(0.0),

	updateDispCount(0),
	currDispFps(0.0),
	aggrDispFps(0.0),
	avgDispFps(0.0),

	updateCount(0),
	currUpdateDelay(0.0),
	aggrUpdateDelay(0.0),
	avgUpdateDelay(0.0),

	eqCount(0),
	currEqDelay(0.0),
	aggrEqDelay(0.0),
	avgEqDelay(0.0),

	expectedFps(24), /* set default value just in case */
	adjustedFps(24),
	currAbsDeviation(0.0),
	aggrAbsDeviation(0.0),
	avgAbsDeviation(0.0),

	aggrRecvFpsVariance(0.0),
	recvFpsVariance(0.0),
	recvFpsStdDeviation(0.0),

	_ts_currframe(0.0),
	_ts_nextframe(0.0),
	_deadline_missed(0.0)

    , _cpuTimeSpent_sec(0.0)
    , _cpuTimeRequired(0.0)
    , cpuUsage(0.0)
    , ruend_nvcsw(0)
    , ruend_nivcsw(0)
    , ruend_maxrss(0)
    , ruend_minflt(0)
{
	if ( _widget->widgetType() == SN_BaseWidget::Widget_RealTime) {
		// set initial avgRecvLatency
		avgRecvLatency = 1.0 / expectedFps; // second
	}
	else {
		expectedFps = 0;
		adjustedFps = 0;
	}
}

void PerfMonitor::printData() const {
	// avgRecvLatency,  avgConvDelay,  avgDrawLatency,  avgRecvFps, avgDispFps, recvFpsVariance , avgAbsDeviation, recvFpsStdDeviation
	qDebug() << "PerfMonitor::printData()" << avgRecvLatency << avgUpdateDelay << avgDrawLatency << _avgEffectiveFps << avgDispFps  << recvFpsVariance << avgAbsDeviation << recvFpsStdDeviation;
}

void PerfMonitor::updateDataWithLatencies(ssize_t byteread, qreal actualtime_sec, qreal cputime_sec) {

    /*
	if ( recvTimer.isNull() ) {
		qWarning("PerfMonitor::%s() : recvTimer is null", __FUNCTION__);
		return;
	}
	if ( ! recvTimer.isValid()) {
		qWarning("PerfMonitor::%s() : recvTimer isn't valid", __FUNCTION__);
		return;
	}
    */

    ++_recvFrameCount;

    /***
	  The netlatency is network pixel receive latency only (recv() system call).
	  Therefore this can't be used to calculate observed frame rate.
	****/
//	currRecvLatency = netlatency; // in second


    /**
      *
	  * ASSUMES recvTimer.start() had been called once
      *
      */
//	int elapsed = recvTimer.restart(); // millisecond
//	int elapsed = recvTimer.elapsed(); // millisecond


    /**
      This is the total delay of a single iteration of a worker thread.
	  Therefore, this could be the FPS that user perceives.
	  **/
//	qreal observed_delay = (qreal)elapsed * 0.001; // to second



    //
	// The bandwidth (in Mbps) I'm currently consuming
    //
    qreal bwtemp = (byteread * 8.0) / (actualtime_sec * 1000000.0); // Mbps

    if (_priori) {
        if (bwtemp <= _requiredBandwidth)
            _currEffectiveBW = bwtemp;
        else {
            // perhaps measurement error so discard the data measured
            _currEffectiveBW = _requiredBandwidth;
        }
    }
    else {
         _currEffectiveBW = bwtemp;
         if (_currEffectiveBW > _requiredBandwidth) {
             _requiredBandwidth = _currEffectiveBW;
         }
    }



    //
    // FPS that user perceives
    //
	_currEffectiveFps = 1.0 / actualtime_sec;
    //peakRecvFps = currRecvFps;

	// deviation from expectedFps
	qreal temp = expectedFps - _currEffectiveFps;
	currAbsDeviation = (temp > 0) ? temp : 0;
	//fprintf(stderr, "%.2f\n", currAbsDeviation);
	temp = adjustedFps - _currEffectiveFps;
	currAdjDeviation = (temp > 0) ? temp : 0;



    //
    // CPU time
    //
    _cpuTimeSpent_sec = cputime_sec;
    cpuUsage = cputime_sec / actualtime_sec;


    /****
	struct timeval tv;
	gettimeofday(&tv, 0);
	qreal now = (qreal)(tv.tv_sec) + 0.000001 * (qreal)(tv.tv_usec); // seconds

	// set current timestamp
	_ts_currframe = now;

	if ( _ts_nextframe > 0 )
		_deadline_missed = _ts_currframe - _ts_nextframe;

	// update _ts_nextframe
	if (expectedFps > 0) {
//		_ts_nextframe = _ts_currframe + 1.0 / expectedFps;
		_ts_nextframe = _ts_currframe + 1.0 / adjustedFps;
	}
    ***/


	unsigned int skip = 200;
	if ( _recvFrameCount > skip ) {
//		aggrRecvLatency += currRecvLatency;
//		avgRecvLatency = aggrRecvLatency / (qreal)(_recvFrameCount-skip);

		/* observed fps */
		_aggrEffectiveFps += _currEffectiveFps;
		_avgEffectiveFps = _aggrEffectiveFps / (qreal)(_recvFrameCount- skip);

		/* average absolute deviation */
//		aggrAbsDeviation += currAbsDeviation;
//		avgAbsDeviation = aggrAbsDeviation / (qreal)(_recvFrameCount- skip);


		/* variance Var(X0 = E[(X-u)2] */
//		aggrRecvFpsVariance += pow(_currEffectiveFps - _avgEffectiveFps, 2);
//		recvFpsVariance = aggrRecvFpsVariance / (qreal)(_recvFrameCount - skip);

		/* standard deviation */
//		recvFpsStdDeviation = sqrt(recvFpsVariance);
	}




	// Time spent in operating system code on behalf of processes.
//	double systime = ((double)ruend.ru_stime.tv_sec + 0.000001*(double)ruend.ru_stime.tv_usec) - ((double)rustart.ru_stime.tv_sec + 0.000001 * (double)rustart.ru_stime.tv_usec);

	// Time spent executing user instructions.
//	double usrtime = ((double)ruend.ru_utime.tv_sec + 0.000001*(double)ruend.ru_utime.tv_usec) - ((double)rustart.ru_utime.tv_sec + 0.000001 * (double)rustart.ru_utime.tv_usec);

	// ratio of time spent actually doing something to the total time spent
//    _cpuTimeSpent = 1000000 * (systime + usrtime); // in microsecond

//	ruend_nvcsw = ruend.ru_nvcsw;
//	ruend_nivcsw = ruend.ru_nivcsw;
//	ruend_maxrss = ruend.ru_maxrss;
//	ruend_minflt = ruend.ru_minflt;
}



qreal PerfMonitor::setAdjustedFps(qreal f) {
	if ( f < 1 ) {
		adjustedFps = 1;
		return -1;
	}
	else if ( f > expectedFps ) {
		adjustedFps = expectedFps;
		return 1;
	}
	else {
		adjustedFps = f;
		return 0;
	}
}


/*!
  The rate at which widget's update() event is dispatched
  */
void PerfMonitor::updateDispFps() {
	if ( dispTimer.isNull() ) {
		dispTimer.start();
	}
	else {
		++updateDispCount;

		qreal elap = (qreal)(dispTimer.restart()) * 0.001; // sec
		currDispFps = 1.0 / elap;
		aggrDispFps += currDispFps;
		avgDispFps = aggrDispFps / (qreal)updateDispCount;
	}
}

/*!
  Pixmap painting delay
  */
qreal PerfMonitor::updateDrawLatency() {
	if ( drawTimer.isNull() ) {
		qWarning("PerfMonitor::%s() : drawTimer object is null", __FUNCTION__);
		return -1.0;
	}
//	if ( ! drawTimer.isValid()) {
//		qWarning("PerfMonitor::%s() : drawTimer object isn't valid", __FUNCTION__);
//		return;
//	}

	/* to measure painting delay */
	++drawCount;
	int elapsed = drawTimer.elapsed();
	currDrawLatency  = (qreal)elapsed * 0.001; // to second

	if ( drawCount > 100) {
		aggrDrawLatency += currDrawLatency;
		avgDrawLatency = aggrDrawLatency / (qreal)(drawCount-100);
	}

	return currDrawLatency;
}

qreal PerfMonitor::updateUpdateDelay() {
	if (updateTimer.isNull()) {
		return -1.0;
	}

	// convTimer.start() has called in SageStreamWidget::updateWidget()
	 currUpdateDelay = (qreal)(updateTimer.elapsed()) * 0.001; // to second
	++updateCount;

	if (updateCount > 100) {
		aggrUpdateDelay += currUpdateDelay;
		avgUpdateDelay = aggrUpdateDelay / (qreal)(updateCount - 100);
	}

	return currUpdateDelay;
}

qreal PerfMonitor::updateEQDelay() {
	if (eqTimer.isNull()) {
		return -1.0;
	}

	currEqDelay = (qreal)(eqTimer.elapsed()) * 0.001;
	++eqCount;

	if(eqCount > 100) {
		aggrEqDelay += currEqDelay;
		avgEqDelay = aggrEqDelay / (qreal)(eqCount - 100);
	}
	return currEqDelay;
}

void PerfMonitor::reset() {
	_recvFrameCount = 0;
	currRecvLatency =0.0;
	avgRecvLatency = 1.0 / expectedFps; // set default
	aggrRecvLatency = 0.0;
	_currEffectiveFps = 0.0;
	_aggrEffectiveFps = 0.0;
	_avgEffectiveFps = 0.0;
	_currEffectiveBW = 0.0;

	drawCount = 0;
	currDrawLatency = 0.0;
	avgDrawLatency = 0.0;
	aggrDrawLatency = 0.0;

	updateDispCount = 0;
	currDispFps = 0.0;
	aggrDispFps = 0.0;
	avgDispFps = 0.0;

	updateCount = 0;
	currUpdateDelay = 0.0;
	aggrUpdateDelay = 0.0;
	avgUpdateDelay = 0.0;

	eqCount = 0;
	currEqDelay = 0.0;
	aggrEqDelay = 0.0;
	avgEqDelay = 0.0;

//	expectedFps = 0.0;
	if (_widget->widgetType() == SN_BaseWidget::Widget_RealTime)
		adjustedFps = expectedFps;
	currAbsDeviation = 0.0;
	aggrAbsDeviation = 0.0;
	avgAbsDeviation = 0.0;

	aggrRecvFpsVariance = 0.0;
	recvFpsVariance = 0.0;
	recvFpsStdDeviation = 0.0;

	_ts_currframe = 0.0;
	_ts_nextframe = 0.0;
	_deadline_missed = 0.0;

	cpuUsage = 0.0;
	ruend_nvcsw = 0;
	ruend_nivcsw = 0;
	ruend_maxrss = 0;
	ruend_minflt = 0;
}


qreal PerfMonitor::getReqBandwidthMbps(qreal percentage /* = 1.0 */) const {
    if (_requiredBandwidth <= 0) {
        return 0.0;
    }

    if (percentage > 1) {
        return _requiredBandwidth;
    }

    return _requiredBandwidth * percentage;
}


//qreal PerfMonitor::set_ts_currframe() {

//	struct timeval tv;
//	gettimeofday(&tv, 0);
//	qreal temp = (qreal)(tv.tv_sec) + 0.000001 * (qreal)(tv.tv_usec); // seconds

//	if (_ts_currframe > 0 ) {
//		currRecvFps  = 1.0 / (temp - _ts_currframe); // rate at which scheduleUpdate() is called

//		qreal temp = expectedFps - currRecvFps;
//		currAbsDeviation = (temp > 0) ? temp : 0;
//	}
//	_ts_currframe = temp;

//	unsigned int skip = 200;
//	if ( recvFrameCount > skip ) {
//		aggrRecvFps += currRecvFps;
//		avgRecvFps = aggrRecvFps / (qreal)(recvFrameCount- skip);

//		/* average absolute deviation */
//		aggrAbsDeviation += currAbsDeviation;
//		avgAbsDeviation = aggrAbsDeviation / (qreal)(recvFrameCount- skip);


//		/* variance Var(X0 = E[(X-u)2] */
//		aggrRecvFpsVariance += pow(currRecvFps - avgRecvFps, 2);
//		recvFpsVariance = aggrRecvFpsVariance / (qreal)(recvFrameCount - skip);

//		/* standard deviation */
//		recvFpsStdDeviation = sqrt(recvFpsVariance);
//	}

//	/*
//	  negative value means earlier than expected deadline
//	  positive value means seconds delayed
//	  */
//	if ( _ts_nextframe > 0 )
//		_deadline_missed = _ts_currframe - _ts_nextframe;

//	if ( expectedFps > 0 ) {
//		// update next expected frame delievery time
//		_ts_nextframe = _ts_currframe + 1.0 / (qreal)expectedFps;
//	}

//	return _deadline_missed;
//}

//qreal PerfMonitor::set_ts_currframe(qreal now) {
//	/*
//	  negative value means earlier than expected deadline
//	  positive value means seconds delayed
//	  */

//	_ts_currframe = now;
//	if ( _ts_nextframe > 0 )
//		_deadline_missed = _ts_currframe - _ts_nextframe;

//	if ( expectedFps > 0 ) {
//		// update next expected frame delievery time
//		_ts_nextframe = _ts_currframe + 1.0 / (qreal)expectedFps;
//	}

//	return _deadline_missed;
//}



