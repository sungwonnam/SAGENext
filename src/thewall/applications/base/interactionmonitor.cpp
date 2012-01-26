#include "interactionmonitor.h"


SN_Priority::SN_Priority(QObject *parent)
    : QObject(parent)
    , _timeLastIntr(0)
    , _timeFirstIntr(QDateTime::currentMSecsSinceEpoch())
    , _intrCounter(0)
//    , _ipm(0.0)
{
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
