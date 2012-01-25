#include "interactionmonitor.h"


InteractionMonitor::InteractionMonitor(QObject *parent)
    : QObject(parent)
    , _timeLast(0)
    , _timeFirst(QDateTime::currentMSecsSinceEpoch())
    , _counter(0)
//    , _ipm(0.0)
{
}

qreal InteractionMonitor::ipm() const {
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	qreal durationInMin = qreal(now - _timeFirst) / (60.0 * 60.0); // minute
	return qreal(_counter) / durationInMin;
}

void InteractionMonitor::setLastInteraction(InteractionMonitor::IntrType t /* = NOINTR */, qint64 time /* = 0 */) {
	++_counter;

	if (time > 0) {
		_timeLast = time;
	}
	else {
		_timeLast = QDateTime::currentMSecsSinceEpoch();
	}

	InteractionMonitor::IntrType type;
	if ( t == InteractionMonitor::NOINTR) {
		type = _typeLast;
	}
	else {
		type = t;
	}

	_history.insert(_timeLast, type);

//	_ipm = qreal(_counter)/ (qreal(_timeLast - _timeFirst) / (60.0 * 60.0)); // per minute

//	qDebug() << "type" << type << "occurred at" << _timeLast << "freq: " << _frequency << "per minute";
}
