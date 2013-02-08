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
	explicit SN_Priority(SN_BaseWidget *widget, QObject *parent=0);

	enum IntrType {NOINTR, MOVE, RESIZE, CLICK, CONTENTS};

    inline void setPriorityGrid(SN_PriorityGrid *pg) {_priorityGrid = pg;}

	inline qint64 timeLastInteracted() const {return _timeLastIntr;}
	inline IntrType lastInteractionType() const {return _typeLastIntr;}
	inline void setLastInteractionType(SN_Priority::IntrType t) {_typeLastIntr = t;}

	inline quint16 evrToWin() const {return _evr_to_win;}
	inline quint16 evrToWall() const {return _evr_to_wall;}

	inline quint64 evrSize() const {return _evrsize;}

    inline qreal priority() const {return _priority;}

    inline qreal Pvisual() const {return _Pvisual;}
    inline qreal Pinteract() const {return _Pinteract;}
    inline qreal Ptemp() const {return _Ptemp;}
    inline void setPtemp(qreal v) {_Ptemp = v;}

    inline void setWeights(qreal wv, qreal wi, qreal wt) {
        qreal total = wv + wi + wt;
        _weight_visual = wv/total;
        _weight_interact = wi/total;
        _weight_temp = wt/total;
    }
    inline qreal Wvisual() const {return _weight_visual;}
    inline qreal Winteract() const {return _weight_interact;}
    inline qreal Wtemp() const {return _weight_temp;}

	/*!
	  Increment the _intrCounter
	  */
	void setLastInteraction(SN_Priority::IntrType t = SN_Priority::NOINTR, qint64 time = 0);

	/*!
	  Returns interactions per minute on this _widget so far (from creation time to until now)
	  */
	qreal ipm() const;

	qreal computePriority(quint64 sum_EVS, quint64 sum_intr);


	inline int priorityQuantized() const;

    /*!
      For The Pinteract
      */
    void countIntrIncrements();

    inline quint64 intrIncrements() const {return _intrIncrements;}

protected:
	/*!
      Always between 0 and 1. 1 means the highest priority
      */
    qreal _priority;

    qreal _Pvisual;

    qreal _Pinteract;

    qreal _Ptemp;

    qreal _weight_visual;
    qreal _weight_interact;
    qreal _weight_temp;

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

    quint64 _intrIncrements;

	/*!
	  A list of (msec since epoch, the type of interaction) tuple
	  */
	QMap<qint64, SN_Priority::IntrType> _intrHistory;


private:
	SN_BaseWidget const *_widget;

    SN_PriorityGrid *_priorityGrid;

	quint64 _evrsize;

	/*!
	  Ratio of effective visible region to the window size
	  This tells how much the application is revealing its content
	  */
	qreal _evr_to_win;

	/*!
	  This tells how much the application window covering the wall
	  */
	int _evr_to_wall;

    qreal _ipm;

	/*!
      The Pvisual.
	  _evr_to_win and _evr_to_wall are also computed here
	  */
	void m_computePvisual(quint64 sumevs);


	
signals:
	
public slots:

};

#endif // SN_PRIORITY_H
