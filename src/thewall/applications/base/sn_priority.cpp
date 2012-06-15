#include "sn_priority.h"
#include <QGraphicsScene>

SN_Priority::SN_Priority(SN_BaseWidget *w, QObject *parent)
    : QObject(parent)
    , _priority(-1)
    , _priorityQuantized(0)
    , _timeLastIntr(0)
    , _timeFirstIntr(QDateTime::currentMSecsSinceEpoch())
    , _intrCounter(0)
    , _intrCounterPrev(0)
    , _widget(w)

    , _evr_to_win(0)
    , _evr_to_wall(0)
    , _ipm(0)
{
}

qreal SN_Priority::computePriority(qint64 currTimeEpoch) {
	//	/*******************
	//	 size of Effective Visible Region
	//	 *****************/
	//	qreal weight1 = 1.0;
	//	QRegion effectiveRegion = effectiveVisibleRegion();

	//	int effectiveVisibleSize  = 0;
	//	for (int i=0; i<effectiveRegion.rectCount(); ++i ) {
	//		QRect rect = effectiveRegion.rects().at(i);
	//		effectiveVisibleSize += (rect.size().width() * rect.size().height());
	//	}

	//	if (isObscured()) {
	//		//_priority = 0;
	//	}
	//	else {
	//		//_priority = ratioToTheWall()  * (1 + zValue()); // ratio is more important

	//		_priority = weight1  *  effectiveVisibleSize / (scene()->width() * scene()->height());
	//	}


	//	/****
	//	Time passed since last touch
	//	****/
	//	qreal weight2 = 0.4;
	//	qint64 lastTouch = _intMon->timeLastInteracted();
	//	if ( ctepoch > 0 &&  lastTouch > 0) {

	//		// _lastTouch is updated in mousePressEvent
	//		qreal T_lt = (qreal)(ctepoch - lastTouch); // msec passed since last touch (current time in epoch - last touched time in epoch)
	//		T_lt /= 1000.0; // to second

	//		//qDebug() << "[[ Widget" << _globalAppId << weight2 << "/" << T_lt << "=" << weight2/T_lt;
	//		_priority += (weight2 / T_lt);
	//	}

	//	//qDebug() << "[[ Widget" << _globalAppId << "priority" << _priority << "]]";

    Q_UNUSED(currTimeEpoch);

	computeEvrInfo();

    //
    // if app is not revealing itself much
    // then its priority will lowered much
    //
    qreal weight_evrwin = 1.0;
    if (_evr_to_win < 40) {
        weight_evrwin = (qreal)_evr_to_win / 100;
    }

	qreal visualfactor =  (qreal)_evr_to_win  +  (qreal)_evr_to_wall;

    _priority = weight_evrwin * visualfactor + 200.0f * (_intrCounter - _intrCounterPrev);

//    qDebug() << "computePriority()" << weight_evrwin << visualfactor << ipm();

    _intrCounterPrev = _intrCounter;

	return _priority;
}

qreal SN_Priority::ipm() const {
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	qreal durationInMin = qreal(now - _timeFirstIntr) / (60.0 * 60.0); // minute
	return qreal(_intrCounter) / durationInMin;
}

void SN_Priority::setLastInteraction(SN_Priority::IntrType t /* = NOINTR */, qint64 time /* = 0 */) {
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


void SN_Priority::computeEvrInfo(void) {

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
	if (winsize > 0) {
		_evr_to_win = (100 * _evrsize) / winsize; // quint16 : unsigned short
	}

	if (_widget->scene()) {
		_evr_to_wall = (100 * _evrsize) / (_widget->scene()->width() * _widget->scene()->height()); // quint16 : unsigned short
	}
}
