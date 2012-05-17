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
    , currRecvLatency(-1)
    , avgRecvLatency(-1)
    , aggrRecvLatency(0.0)

    , _currEffectiveFps(0.0)
    , _aggrEffectiveFps(0.0)
    , _avgEffectiveFps(0.0)

    , _currEffectiveBW_Mbps(0.0)
    , _maxBWachieved(0.0)
    , _requiredBW_Mbps(0.0)
    , _cumulativeByteRecved(0)
    , _isInteracting(false)

	, drawCount(0)
    , currDrawLatency(-1)
    , avgDrawLatency(-1)
    , aggrDrawLatency(0.0)

    , updateDispCount(0)
    , currDispFps(0.0)
    , aggrDispFps(0.0)
    , avgDispFps(0.0)

    , updateCount(0)
    , currUpdateDelay(0.0)
    , aggrUpdateDelay(0.0)
    , avgUpdateDelay(0.0)

    , eqCount(0)
    , currEqDelay(0.0)
    , aggrEqDelay(0.0)
    , avgEqDelay(0.0)

    , expectedFps(24) /* set default value just in case */
	, adjustedFps(24)
//	currAbsDeviation(0.0),
//	aggrAbsDeviation(0.0),
//	avgAbsDeviation(0.0),

//	aggrRecvFpsVariance(0.0),
//	recvFpsVariance(0.0),
//	recvFpsStdDeviation(0.0),

	, _ts_currframe(0.0)
    , _ts_nextframe(0.0)
    , _deadline_missed(0.0)

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
	// avgRecvLatency,  avgConvDelay,  avgDrawLatency,  avgRecvFps, avgDispFps
//	qDebug() << "PerfMonitor::printData()" << avgRecvLatency << avgUpdateDelay << avgDrawLatency << _avgEffectiveFps << avgDispFps;
}

void PerfMonitor::_updateBWdata(qreal bwtemp) {

    ///
    // If app provided resource required (e.g. constant framerate streaming such as Sage app)
    // then I know current BW is always less than or equal to required BW
    //
    if (_priori) {
        if (bwtemp == 0) {
            _currEffectiveBW_Mbps = bwtemp;
//                _requiredBW_Mbps = 0; // how to restore this ???
        }
        else if (bwtemp <= _requiredBW_Mbps) {
            _currEffectiveBW_Mbps = bwtemp;
        }
        else {
            // perhaps measurement error so discard the data measured
            _currEffectiveBW_Mbps = _requiredBW_Mbps;
        }
    }

    //
    // No priori
    //
    else {
        _currEffectiveBW_Mbps = bwtemp; // have to believe current measurement

        //
        // the widget isn't consuming resource, so update requiredBW so that scheduler won't count this widget
        //
        if ( _currEffectiveBW_Mbps == 0 ) {
            _requiredBW_Mbps = 0;
        }

        //
        // The observedQ_Rq > 1
        // The required bw is set too low, so update required BW
        //
        else if (_currEffectiveBW_Mbps > _requiredBW_Mbps) {
            _requiredBW_Mbps = _currEffectiveBW_Mbps; // INCREASE Rq
        }

        //
        // The observedQ_Rq <= 1
        //
        else if (_currEffectiveBW_Mbps <= _requiredBW_Mbps) {

            if ( observedQuality_Dq() >= 0.9 ) {
                //
                // it's following what the scheduler demanded
                //

                //
                // If this is high priority app then demandedQuality could be closer to 1.0
                // (Even if this is low priority app, with high TotalResource demandedQuality could be closer to 1.0)
                // This means observedQ_Rq ~= observedQ_Dq ~= 1.0
                // Then let's increase Rq and see what happens !
                // because it may be able to consume more
                //
                if ( _widget->demandedQuality() >= 0.9) {
                    _requiredBW_Mbps = 1.2f * _requiredBW_Mbps; // INCREASE Rq
                }

                //
                // The low demandedQuality means
                // The TotalResource is low enough and this app is somewhat low priority
                //
                else {
                    // do nothing
                }
            }
            else if (observedQuality_Dq() > 1) {
                // it's consuming more than the scheduler demanded. !!
            }
            else if (observedQuality_Dq() < 0.9) {
                // it's not even consuming the amount the scheduler allowed
                // so lower the requiredBW a bit

                _requiredBW_Mbps = _currEffectiveBW_Mbps; // DECREASE Rq
            }
        }
    }

    _maxBWachieved = qMax(_maxBWachieved, _currEffectiveBW_Mbps);
}

//
// Resource monitor calls this function
//
void PerfMonitor::updateDataWithCumulativeByteReceived(qint64 timestamp) {

    if (!_cumulativeByteRecvedList.isEmpty()) {
        const QPair<qint64, quint64> recentValue = _cumulativeByteRecvedList.front();
        quint64 deltaByte = _cumulativeByteRecved  -  recentValue.second;
        qint64 deltaMsec = timestamp - recentValue.first;

        //
        // current BW usage
        //
        qreal bwtemp = (1e-6 * 8.0f * (qreal)deltaByte) / ((qreal)deltaMsec / 1000.0f); // Mbps

        _updateBWdata( bwtemp );
    }

    //
    // update value & maintain 10 data points
    //
    _cumulativeByteRecvedList.push_front( QPair<qint64, quint64>(timestamp, _cumulativeByteRecved) );
    if (_cumulativeByteRecvedList.size() > 10) {
        _cumulativeByteRecvedList.pop_back();
    }
}

//
// The widget updates this
//
void PerfMonitor::addToCumulativeByteReceived(quint64 byte, qreal actualtime_sec /* 0 */, qreal cputime_sec /* 0 */) {

     ++_recvFrameCount;

    _cumulativeByteRecved += byte;

    if ( actualtime_sec > 0) {

        // this is handled by rMonitor
//        _updateBWdata(bwtemp);

        //
        // FPS that user perceives
        //
        _currEffectiveFps = 1.0 / actualtime_sec;
        //peakRecvFps = currRecvFps;

        // deviation from expectedFps
        /*
        qreal temp = expectedFps - _currEffectiveFps;
        currAbsDeviation = (temp > 0) ? temp : 0;
        //fprintf(stderr, "%.2f\n", currAbsDeviation);
        temp = adjustedFps - _currEffectiveFps;
        currAdjDeviation = (temp > 0) ? temp : 0;
        */


        unsigned int skip = 200;
        if ( _recvFrameCount > skip ) {
    //		aggrRecvLatency += currRecvLatency;
    //		avgRecvLatency = aggrRecvLatency / (qreal)(_recvFrameCount-skip);

            //
            // observed average fps
            //
            _aggrEffectiveFps += _currEffectiveFps;
            _avgEffectiveFps = _aggrEffectiveFps / (qreal)(_recvFrameCount - skip);

            /* average absolute deviation */
    //		aggrAbsDeviation += currAbsDeviation;
    //		avgAbsDeviation = aggrAbsDeviation / (qreal)(_recvFrameCount- skip);


            /* variance Var(X0 = E[(X-u)2] */
    //		aggrRecvFpsVariance += pow(_currEffectiveFps - _avgEffectiveFps, 2);
    //		recvFpsVariance = aggrRecvFpsVariance / (qreal)(_recvFrameCount - skip);

            /* standard deviation */
    //		recvFpsStdDeviation = sqrt(recvFpsVariance);
        }

    }

    if (cputime_sec > 0) {

        //
        // CPU time
        //
        _cpuTimeSpent_sec = cputime_sec;
        cpuUsage = cputime_sec / actualtime_sec;
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



qreal PerfMonitor::observedQuality_Rq() const {
    if ( _requiredBW_Mbps > 0) {
        return _currEffectiveBW_Mbps / _requiredBW_Mbps;
    }
    else {
        return -1;
    }
}

qreal PerfMonitor::observedQuality_Dq() const {
    if (_widget) {
        qreal demandedBW = _widget->demandedQuality() * _requiredBW_Mbps;
        if (demandedBW > 0) {
            return _currEffectiveBW_Mbps / demandedBW;
        }
    }
    else {
        qDebug() << "PerfMonitor::observedQuality_Dq() : _widget is null";
    }

    return -1;
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
  The time spent in paint()
  */
qreal PerfMonitor::updateDrawLatency() {
	if ( drawTimer.isNull() ) {
		qWarning("PerfMonitor::%s() : drawTimer is null. Please start it first.", __FUNCTION__);
		return -1.0;
	}
//	if ( ! drawTimer.isValid()) {
//		qWarning("PerfMonitor::%s() : drawTimer object isn't valid", __FUNCTION__);
//		return;
//	}

	/* to measure painting delay */
	++drawCount;

    //
    // drawTimer.start() must be called by the widget
    //
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
        qWarning("PerfMonitor::%s() : updateTimer is null. Please start it first.", __FUNCTION__);
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
        qWarning("PerfMonitor::%s() : eqTimer is null. Please start it first.", __FUNCTION__);
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
	_currEffectiveBW_Mbps = 0.0;

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
//	currAbsDeviation = 0.0;
//	aggrAbsDeviation = 0.0;
//	avgAbsDeviation = 0.0;

//	aggrRecvFpsVariance = 0.0;
//	recvFpsVariance = 0.0;
//	recvFpsStdDeviation = 0.0;

	_ts_currframe = 0.0;
	_ts_nextframe = 0.0;
	_deadline_missed = 0.0;

	cpuUsage = 0.0;
	ruend_nvcsw = 0;
	ruend_nivcsw = 0;
	ruend_maxrss = 0;
	ruend_minflt = 0;
}


qreal PerfMonitor::getRequiredBW_Mbps(qreal percentage /* = 1.0 */) const {
    if (percentage > 1) {
        return _requiredBW_Mbps;
    }

    return _requiredBW_Mbps * percentage;
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



