#ifndef PRIORITYGRID_H
#define PRIORITYGRID_H

#include <QtGui>

/*!
  Rectangle represents region of interest.
  */
class ROIRect : public QRect {
public:
	explicit ROIRect() : QRect() {}
	explicit ROIRect(int id, int x, int y, int dx, int dy) : QRect(x,y,dx,dy), _priority(0.0),_id(id) {}
	~ROIRect() {}

	/*!
	  At every scheduling instance, _priority is updated
	  */
	inline qreal priority() const {return _priority;}

	/*!
	  This function will update rectangle's priority information. This data reflects historical changes.
	  */
//	void update();

	inline int id() const {return _id;}

protected:
	/*!
	  should reflect usage history on this rectangle
	  */
	qreal _priority;

	int _id;
};




class PriorityGrid : public QObject
{
	Q_OBJECT
public:
	explicit PriorityGrid(QObject *parent=0) : QObject(parent), _thescene(0), _isEnabled(false) {}

	/*!
	  rectSize defines the size of each rectangle in the grid
	  */
	explicit PriorityGrid(const QSize &rectSize, QGraphicsScene *scene, QObject *parent = 0);

	inline void setScene(QGraphicsScene *s) {_thescene = s;}

	inline void setRectSize(const QSize &s) {_rectSize = s;}
	inline void setRectSize(int w, int h) {_rectSize = QSize(w, h);}

	inline bool isEnabled() const {return _isEnabled;}

	/*!
	  An application adds its priority to the rects intersects with its window
	  */
//	int addPriority(const QRect &windowSceneRect, qreal priorityvalue);
	
private:
	/*!
	  The region covers the entire scene
	  */
	QRegion _theSceneRegion;

	QVector<qreal> _priorityVec;

	QGraphicsScene *_thescene;

	QSize _rectSize;

	/*!
	  false until buildRectangle() returns
	  */
	bool _isEnabled;

	int buildRectangles();

signals:
	
public slots:

	/*!
	  finds out app rects above each rect in the region
	  and updates values accordingly
	  */
	void updatePriorities();
	
};

#endif // PRIORITYGRID_H
