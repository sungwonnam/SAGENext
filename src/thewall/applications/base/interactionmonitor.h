#ifndef INTERACTIONMONITOR_H
#define INTERACTIONMONITOR_H

#include <QtCore>

class InteractionMonitor : public QObject
{
	Q_OBJECT
	Q_ENUMS(IntrType)
public:
	explicit InteractionMonitor(QObject *parent = 0);

	enum IntrType {NOINTR, MOVE, RESIZE, CLICK};

	/*!
	  All the member variables will be filled in this function.
	  */
	void setLastInteraction(InteractionMonitor::IntrType t = InteractionMonitor::NOINTR, qint64 time = 0);

	inline qint64 timeLastInteracted() const {return _timeLast;}
	inline IntrType lastInteractionType() const {return _typeLast;}

	inline void setLastInteractionType(InteractionMonitor::IntrType t) {_typeLast = t;}

	qreal ipm() const;

protected:
	/*!
	  When is the last time(msec since epoch) it is interacted
	  */
	qint64 _timeLast;

	/*!
	  This is set when the object is instantiated
	  */
	const qint64 _timeFirst;

	/*!
	  This is continuously updated by SN_BaseWidget to keep track of the most recent interaction type
	  */
	IntrType _typeLast;

	/*!
	  A counter for interaction. This is equal to _interactionHistory.size()
	  */
	quint64 _counter;

	/*!
	  how many interactions per minute
	  */
//	qreal _ipm;

	/*!
	  A list of (msec since epoch, the type of interaction) tuple
	  */
	QMap<qint64, InteractionMonitor::IntrType> _history;
	
signals:
	
public slots:

};

#endif // INTERACTIONMONITOR_H
