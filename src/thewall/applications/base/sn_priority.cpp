#include "sn_priority.h"
#include <QGraphicsScene>

SN_Priority::SN_Priority(SN_BaseWidget *w, QObject *parent)
    : QObject(parent)
    , _priority(0.5)
    , _priorityQuantized(0)
    , _timeLastIntr(0)
    , _timeFirstIntr(QDateTime::currentMSecsSinceEpoch())
    , _intrCounter(0)
    , _widget(w)

    , _evr_to_win(0)
    , _evr_to_wall(0)
{
}

qreal SN_Priority::priority(qint64 currTimeEpoch) {
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

	computeEvrInfo();

	qreal visualfactor = 0.3 * (qreal)_evr_to_win +  0.7 * (qreal)_evr_to_wall;

	_priority = visualfactor;

	return _priority;
}

qreal SN_Priority::ipm() const {
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	qreal durationInMin = qreal(now - _timeFirstIntr) / (60.0 * 60.0); // minute
	return qreal(_intrCounter) / durationInMin;
}

void SN_Priority::setLastInteraction(SN_Priority::IntrType t /* = NOINTR */, qint64 time /* = 0 */) {
	++_intrCounter;

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

//	_ipm = qreal(_counter)/ (qreal(_timeLast - _timeFirst) / (60.0 * 60.0)); // per minute

//	qDebug() << "type" << type << "occurred at" << _timeLast << "freq: " << _frequency << "per minute";
}


void SN_Priority::computeEvrInfo(void) {

	Q_ASSERT(_widget);

	QRegion evr = _widget->effectiveVisibleRegion();
	quint64 evrsize = 0;
	foreach(QRect r, evr.rects()) {
		evrsize += (r.width() * r.height()); // quint64 : unsigned long long int
	}

	QSizeF effectiveSize = _widget->size() * _widget->scale();
	quint64 winsize = effectiveSize.width() * effectiveSize.height(); // quint64 : unsigned long long int

	// ratio of EVR to window size (%)
	_evr_to_win = (100 * evrsize) / winsize; // quint16 : unsigned short

	Q_ASSERT(_widget->scene());
	_evr_to_wall = (100 * evrsize) / (_widget->scene()->width() * _widget->scene()->height()); // quint16 : unsigned short
}
