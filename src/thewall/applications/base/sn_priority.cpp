#include "applications/base/sn_priority.h"
#include "applications/base/sn_basewidget.h"
#include "system/sn_resourcemonitor.h"
#include <QGraphicsScene>

SN_Priority::SN_Priority(SN_BaseWidget *w, QObject *parent)
    : QObject(parent)
    , _priority(-1)
    , _Pvisual(-1)
    , _Pinteract(-1)
    , _Ptemp(0)
    , _weight_visual(1.0)
    , _weight_interact(10.0)
    , _weight_temp(0.0)

    , _timeLastIntr(0)
    , _timeFirstIntr(QDateTime::currentMSecsSinceEpoch())
    , _intrCounter(0)
    , _intrCounterPrev(0)
    , _widget(w)
    , _priorityGrid(0)

    , _evr_to_win(0.0)
    , _evr_to_wall(0)
    , _ipm(0)
{
    setWeights(1.0, 10.0, 0); // default values
}

qreal SN_Priority::computePriority(quint64 sum_EVS, quint64 sum_intr) {

    // Pvisual
	m_computePvisual(sum_EVS);

    // Pinteract
    if (!sum_intr) _Pinteract = 0;
    else _Pinteract = (qreal)_intrIncrements / (qreal)sum_intr;

    _priority = _weight_visual * _Pvisual + _weight_interact * _Pinteract;

    //
    // Update pGrid cell value with my Pvisual and Pinteract
    //
    if (_priorityGrid && _priorityGrid->isEnabled()) {
        _priorityGrid->addPriorityOfApp(_widget->sceneBoundingRect().toRect(), _priority);
    }

    //
    // if app is not revealing itself much
    // then its priority will lowered much
    //
    /*
    qreal weight_evrwin = 1.0;
    if (_evr_to_win < 40) {
        weight_evrwin = (qreal)_evr_to_win / 100;
    }
	qreal visualfactor =  (qreal)_evr_to_win  +  (qreal)_evr_to_wall;
    _priority = weight_evrwin * visualfactor + 100.0f * (_intrCounter - _intrCounterPrev);
    */

	return _priority;
}

qreal SN_Priority::ipm() const {
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	qreal durationInMin = qreal(now - _timeFirstIntr) / (60.0 * 60.0); // minute
	return qreal(_intrCounter) / durationInMin;
}

void SN_Priority::setLastInteraction(SN_Priority::IntrType t /* = NOINTR */, qint64 time /* = 0 */) {

    Q_UNUSED(t);
    Q_UNUSED(time);

	++_intrCounter;

    /*
	if (time > 0) {
		_timeLastIntr = time;
	}
	else {
		_timeLastIntr = QDateTime::currentMSecsSinceEpoch();
	}

	SN_Priority::IntrType type;
	if ( t == SN_Priority::NOINTR) {
		type = _typeLastIntr;
	}
	else {
		type = t;
	}

	_intrHistory.insert(_timeLastIntr, type);

	_ipm = qreal(_intrCounter)/ (qreal(_timeLastIntr - _timeFirstIntr) / (60.0 * 60.0)); // per minute
    */

//	qDebug() << "type" << type << "occurred at" << _timeLast << "freq: " << _frequency << "per minute";
}


void SN_Priority::m_computePvisual(quint64 sumevs) {
	Q_ASSERT(_widget);

    /**
     *This assumes that _widget->computeEVS() was called before
     */
	_evrsize = _widget->EVS(); // no computation just read the value. O(1)

	QSizeF effectiveWinSize = _widget->size() * _widget->scale();
	quint64 winsize = effectiveWinSize.width() * effectiveWinSize.height(); // quint64 : unsigned long long int
    if (winsize <= 0) return;

    _evr_to_win = (qreal)_evrsize / (qreal)winsize;
    _Pvisual = _evr_to_win * (qreal)_evrsize / (qreal)sumevs;

//    qDebug() << "\t" << _widget->globalAppId() << "EVS: " << _evrsize << "EVS ratio" << (qreal)_evrsize/(qreal)sumevs << "Pvisual: " << _Pvisual;
}

void SN_Priority::countIntrIncrements() {
    _intrIncrements = _intrCounter - _intrCounterPrev;
    _intrCounterPrev = _intrCounter;
}
