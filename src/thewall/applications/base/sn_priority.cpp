#include "sn_priority.h"
#include "../../system/resourcemonitor.h"
#include <QGraphicsScene>

SN_Priority::SN_Priority(SN_BaseWidget *w, QObject *parent)
    : QObject(parent)
    , _priority(-1)
    , _Pvisual(-1)
    , _Pinteract(-1)
    , _Ptemp(0)
    , _timeLastIntr(0)
    , _timeFirstIntr(QDateTime::currentMSecsSinceEpoch())
    , _intrCounter(0)
    , _intrCounterPrev(0)
    , _widget(w)
    , _priorityGrid(0)

    , _evr_to_win(0)
    , _evr_to_wall(0)
    , _ipm(0)
{
}

qreal SN_Priority::computePriority(qint64 currTimeEpoch) {
    Q_UNUSED(currTimeEpoch);

    //
    // P visual and P interact
    //
	m_computePvisual();
    m_computePinteract();

    _priority = _Pvisual + 100.0f * _Pinteract;

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


void SN_Priority::m_computePvisual(void) {

	Q_ASSERT(_widget);

	QRegion evr = _widget->effectiveVisibleRegion(); // evr is in scene coordinate

	_evrsize = 0;
	QVector<QRect>::const_iterator it;
	QVector<QRect> rectsInRegion = evr.rects();
	for (it=rectsInRegion.constBegin(); it!=rectsInRegion.constEnd(); it++) {
		const QRect &r = (*it);
		_evrsize += (r.width() * r.height()); // quint64 : unsigned long long int
	}

	QSizeF effectiveSize = _widget->size() * _widget->scale();
	quint64 winsize = effectiveSize.width() * effectiveSize.height(); // quint64 : unsigned long long int

//	qDebug() << _widget->size() << _widget->scale();

	// ratio of EVR to window size (%)
//	Q_ASSERT(winsize > 0);
    /*
	if (winsize > 0) {
		_evr_to_win = (100 * _evrsize) / winsize; // quint16 : unsigned short
	}

	if (_widget->scene()) {
		_evr_to_wall = (100 * _evrsize) / (_widget->scene()->width() * _widget->scene()->height()); // quint16 : unsigned short
	}
    */

    _evr_to_win = _evrsize / winsize;
    _Pvisual = _evrsize * _evr_to_win;
}

void SN_Priority::m_computePinteract() {
    _Pinteract = _intrCounter - _intrCounterPrev;
    _intrCounterPrev = _intrCounter;
}
