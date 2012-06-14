#ifndef SN_PRIORITY_H
#define SN_PRIORITY_H

#include <QtCore>
#include "basewidget.h"
#include "../../system/prioritygrid.h"


class SN_Priority : public QObject
{
	Q_OBJECT
	Q_ENUMS(IntrType)
public:
	explicit SN_Priority(SN_BaseWidget *widget, QObject *parent = 0);

	enum IntrType {NOINTR, MOVE, RESIZE, CLICK, CONTENTS};

	inline qint64 timeLastInteracted() const {return _timeLastIntr;}
	inline IntrType lastInteractionType() const {return _typeLastIntr;}
	inline void setLastInteractionType(SN_Priority::IntrType t) {_typeLastIntr = t;}

	inline quint16 evrToWin() const {return _evr_to_win;}
	inline quint16 evrToWall() const {return _evr_to_wall;}

	inline quint64 evrSize() const {return _evrsize;}

    inline qreal priority() const {return _priority;}

	/*!
	  All the member variables will be filled in this function.
	  */
	void setLastInteraction(SN_Priority::IntrType t = SN_Priority::NOINTR, qint64 time = 0);



	/*!
	  Returns interactions per minute on this _widget so far (from creation time to until now)
	  */
	qreal ipm() const;

	qreal computePriority(qint64 currTimeEpoch = 0);


	inline int priorityQuantized() const;

protected:
	/*!
      Always between 0 and 1. 1 means the highest priority
      */
    qreal _priority;


    /*!
      To store _priority * offset (for quantization)
      Getting this value is much faster
      */
    int _priorityQuantized;


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

    quint64 _intrCounterPrev;

	/*!
	  A list of (msec since epoch, the type of interaction) tuple
	  */
	QMap<qint64, SN_Priority::IntrType> _intrHistory;


private:
	SN_BaseWidget const *_widget;

	static SN_PriorityGrid _priorityGrid;

	quint64 _evrsize;

	/*!
	  Ratio of effective visible region to the window size
	  This tells how much the application is revealing its content
	  */
	int _evr_to_win;

	/*!
	  This tells how much the application window covering the wall
	  */
	int _evr_to_wall;

    qreal _ipm;

	/*!
	  _evr_to_win and _evr_to_wall are computed here
	  */
	void computeEvrInfo(void);
	
signals:
	
public slots:

};

#endif // SN_PRIORITY_H
