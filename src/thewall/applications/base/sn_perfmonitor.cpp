#include "applications/base/sn_perfmonitor.h"
#include "applications/base/sn_appinfo.h"
#include "applications/base/sn_basewidget.h"
#include "system/sn_scheduler.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <stdio.h>


SN_PerfMonitor::SN_PerfMonitor(QObject *parent)
    : QObject(parent)
    , _widget(static_cast<SN_BaseWidget *>(parent))

	, _overPerformAvg(0)
	, _overPerformAgg(0)
	, _overPerformCnt(0)


    , _wakeUpGuessFps(30)
    , _overPerformMultiplier(1.2)
    , _normPerformMultiplier(1.1)
    , _underPerformEndurance(4)
    , _underPerformCnt(0)

    , _priori(false)

    , _measureStartTime(0)
    , _measureEndTime(0)

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
    , _isRqIncreased(0)
    , _cumulativeByteRecved(0)
    , _prevByteReceived(0)
    , _prevTimestamp(0)
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

void SN_PerfMonitor::printData() const {
	// avgRecvLatency,  avgConvDelay,  avgDrawLatency,  avgRecvFps, avgDispFps
//	qDebug() << "PerfMonitor::printData()" << avgRecvLatency << avgUpdateDelay << avgDrawLatency << _avgEffectiveFps << avgDispFps;
}

void SN_PerfMonitor::_updateBWdata(qreal bwtemp) {

    ///
    // If app provided resource required (e.g. constant framerate streaming such as Sage app)
    // then I know current BW is always less than or equal to required BW and
    // its requiredBW will never change
    //
    if (_priori) {
        if (bwtemp == 0) {
            _currEffectiveBW_Mbps = bwtemp;
            // _requiredBW_Mbps = 0; // how to restore this ???
            _currEffectiveFps = 0;
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
        _currEffectiveBW_Mbps = bwtemp;

        if (_currEffectiveBW_Mbps == 0) {
            _requiredBW_Mbps = 0;
            _currEffectiveFps = 0;
        }
        else {
            if ( _currEffectiveBW_Mbps > _requiredBW_Mbps) {
                if ( _requiredBW_Mbps == 0) {
                    _requiredBW_Mbps = qMax(_maxBWachieved, (_widget->appInfo()->frameSizeInByte() * 8 * _wakeUpGuessFps) / 1e+6);
                }
                else {
                    _requiredBW_Mbps = _currEffectiveBW_Mbps;
                }
            }
            else if ( _currEffectiveBW_Mbps == _requiredBW_Mbps ) {
                _requiredBW_Mbps = _overPerformMultiplier * _currEffectiveBW_Mbps;
            }
            else {
                _requiredBW_Mbps = _currEffectiveBW_Mbps;
            }
        }
    }

    _maxBWachieved = qMax(_maxBWachieved, _currEffectiveBW_Mbps);
}

/*****************************************
void PerfMonitor::_updateBWdata(qreal bwtemp) {

    ///
    // If app provided resource required (e.g. constant framerate streaming such as Sage app)
    // then I know current BW is always less than or equal to required BW and
    // its requiredBW will never change
    //
    if (_priori) {
        if (bwtemp == 0) {
            _currEffectiveBW_Mbps = bwtemp;
            // _requiredBW_Mbps = 0; // how to restore this ???
            _currEffectiveFps = 0;
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
//        qint64 now = QDateTime::currentMSecsSinceEpoch();

        _currEffectiveBW_Mbps = bwtemp; // have to believe current measurement

//        _requiredBW_Mbps = bwtemp;
//        return;

        //
        // the widget isn't consuming resource, so update requiredBW so that scheduler won't count this widget
        //
        if ( _currEffectiveBW_Mbps == 0 ) {

            //
            ////////////////// DECREASE Rq immediately
            //
            _requiredBW_Mbps = 0;
            _currEffectiveFps = 0;
        }

        //
        // The observedQ_Rq > 1
        // The required bw is set too low (incorrect value), so update Rq
        //
        // Note that a non-periodic widget's required BW starts with 0
        //
        // Also, this can easily happen when Dq is set to 1
        // in that case the app will run best-effort w/o extra delay
        // so, Rq can be updated with correct value which is currentBW
        //
        else if (_currEffectiveBW_Mbps > _requiredBW_Mbps) {

            //
            // The app was idling. Now it's woken up
            // So try some big Rq
            //
            if ( _requiredBW_Mbps == 0) {
                //
                /////////////////////// Woken up : GUESS Rq /////////////////////////////
                //
                _requiredBW_Mbps = qMax(_maxBWachieved, (_widget->appInfo()->frameSizeInByte() * 8 * _wakeUpGuessFps) / 1e+6);

                //qDebug() << "PerfMonitor::_updateBWdata() :" << _widget->globalAppId() << "GUESS Rq when woken up" << _requiredBW_Mbps;
				//qDebug() << "___Woken up:" << _requiredBW_Mbps;
            }
            else {
                // It was set too low..
                ////////////////////// Over Performing INCREASE Rq /////////////////////////////
                //
				//
//                _overPerformCnt += 1;
//				_overPerformAgg += _currEffectiveBW_Mbps;
//				_overPerformAvg = _overPerformAgg / _overPerformCnt;

                _requiredBW_Mbps = _overPerformMultiplier * _currEffectiveBW_Mbps;
				//qDebug() << "___Over performing avg BW" << _overPerformAvg << "Rq is now:" << _requiredBW_Mbps;
            }
        }

        //
        // The observedQ_Rq <= 1 This is normal operation.
        //
        else if (_currEffectiveBW_Mbps <= _requiredBW_Mbps) {
            //
            // it's quite consuming what the scheduler demanded.
            // Oq ~= Dq
            //
            if ( observedQuality_Dq() >= 0.9 ) {
                //
                // If this is high priority app then demandedQuality could be closer to 1.0
                // (Even if this is low priority app, with high enough TotalResource demandedQuality could be closer to 1.0)
                //
                // This could mean
                // - High priority, low resource OR
                // - Low priority, high enough TotalResource
				// So by increasing Rq, Maybe I can steal more from other lower priority widgets
                //
                if ( _widget->demandedQuality() >= 1.0) {

					//
                	// Then let's increase Rq and see what happens because it may be able to consume more
					//
                    _requiredBW_Mbps = _normPerformMultiplier * _currEffectiveBW_Mbps;

					//qDebug() << "___Maybe I can use more" << _requiredBW_Mbps;
                }

                //
				// I can consume most of what Scheduler demands
				// But scheduler demands my quality be lower than 1.0
				//
                // Why would scheduler demand lower quality if there's enough resource ?
                // So, this GUARANTEES the TotalResource is not enough -> SYSTEM OVERLOADED => no more Pareto improvement can be made
                //
                else {
                    //
                    // Resource is scarce. lower Rq so that app can obtain higher Dq from the scheduler ==> GREEDY !
                    // If my currBW is increased as a result of high Dq then my Rq will be adjusted anyway
                    //
                    // _requiredBW_Mbps = _currEffectiveBW_Mbps;
                }
            }

            //
            // It's not even consuming the amount the scheduler allowed. ObservedQ < DemandedQ
            // so lower the requiredBW a bit
            //
            else {
				++_underPerformCnt;

				if ( _widget->demandedQuality() > 0.95) {
					if (_underPerformCnt > _underPerformEndurance) {
						_underPerformCnt = 0;
						qreal delta = _requiredBW_Mbps - _currEffectiveBW_Mbps;
						_requiredBW_Mbps -= (delta / 2);
						//_requiredBW_Mbps = _currEffectiveBW_Mbps;
						//qDebug() << "___Under performing High Dq (Oq<Dq): " << _requiredBW_Mbps;
					}
				}
				else {
					//
					// under performing so lower the Rq
					//
                	_requiredBW_Mbps = _currEffectiveBW_Mbps;
					//qDebug() << "___Under performing Low Dq (Oq<Dq): " << _requiredBW_Mbps;
				}

//                if (_isRqIncreased && now - _isRqIncreased > 3000) {
//                    _requiredBW_Mbps = 1.2 * _currEffectiveBW_Mbps;
//                    _isRqIncreased = 0;
//                }
            }
        }
    }

    _maxBWachieved = qMax(_maxBWachieved, _currEffectiveBW_Mbps);
}
*/

//
// Resource monitor 's refresh() calls this function
//
void SN_PerfMonitor::updateDataWithCumulativeByteReceived(qint64 timestamp) {

    if (_prevByteReceived > 0 ) {
        quint64 deltaByte = _cumulativeByteRecved - _prevByteReceived;
        qint64 deltaMsec = timestamp - _prevTimestamp;

        qreal bwtemp = (1e-6 * 8.0f * (qreal)deltaByte) / ((qreal)deltaMsec / 1000.0f); // Mbps
        _updateBWdata( bwtemp );
    }
    _prevByteReceived = _cumulativeByteRecved;
    _prevTimestamp = timestamp;

/*
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
*/
}

//
// The widget updates this
//
void SN_PerfMonitor::addToCumulativeByteReceived(quint64 byte, qreal actualtime_sec /* 0 */, qreal cputime_sec /* 0 */) {

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



qreal SN_PerfMonitor::observedQuality_Rq() const {
    if ( _requiredBW_Mbps > 0) {
        return _currEffectiveBW_Mbps / _requiredBW_Mbps; // Qcur = Rcur / Ropt
    }
    else if (_requiredBW_Mbps == 0) {
        return 0;
    }
    else {
        return -1;
    }
}

qreal SN_PerfMonitor::observedQuality_Dq() const {
    if (_widget) {
        qreal demandedBW = _widget->demandedQuality() * _requiredBW_Mbps;
        if (demandedBW > 0) {
            return _currEffectiveBW_Mbps / demandedBW;
        }
        else if (demandedBW == 0) {
//            qDebug() << "PerfMonitor::observedQuality_Dq() : demanded is 0";
            return 1.0;
        }
    }
    else {
        qDebug() << "PerfMonitor::observedQuality_Dq() : _widget is null";
    }

    return -1;
}



qreal SN_PerfMonitor::setAdjustedFps(qreal f) {
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
void SN_PerfMonitor::updateDispFps() {
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
qreal SN_PerfMonitor::updateDrawLatency() {
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

qreal SN_PerfMonitor::updateUpdateDelay() {
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

qreal SN_PerfMonitor::updateEQDelay() {
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

void SN_PerfMonitor::reset() {
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


qreal SN_PerfMonitor::getRequiredBW_Mbps(qreal percentage /* = 1.0 */) const {
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



