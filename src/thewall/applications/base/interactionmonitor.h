#ifndef INTERACTIONMONITOR_H
#define INTERACTIONMONITOR_H

#include <QtCore>


class SN_Priority : public QObject
{
	Q_OBJECT
	Q_ENUMS(IntrType)
public:
	explicit SN_Priority(QObject *parent = 0);

	enum IntrType {NOINTR, MOVE, RESIZE, CLICK};

	/*!
	  All the member variables will be filled in this function.
	  */
	void setLastInteraction(SN_Priority::IntrType t = SN_Priority::NOINTR, qint64 time = 0);

	inline qint64 timeLastInteracted() const {return _timeLastIntr;}
	inline IntrType lastInteractionType() const {return _typeLastIntr;}

	inline void setLastInteractionType(SN_Priority::IntrType t) {_typeLastIntr = t;}

	qreal ipm() const;

protected:
	/*!
	  When is the last time(msec since epoch) it is interacted
	  */
	qint64 _timeLastIntr;

	/*!
	  This is set when the object is instantiated
	  */
	const qint64 _timeFirstIntr;

	/*!
	  This is continuously updated by SN_BaseWidget to keep track of the most recent interaction type
	  */
	IntrType _typeLastIntr;

	/*!
	  A counter for interaction. This is equal to _interactionHistory.size()
	  */
	quint64 _intrCounter;

	/*!
	  how many interactions per minute
	  */
//	qreal _ipm;

	/*!
	  A list of (msec since epoch, the type of interaction) tuple
	  */
	QMap<qint64, SN_Priority::IntrType> _intrHistory;
	
signals:
	
public slots:

};

#endif // INTERACTIONMONITOR_H
