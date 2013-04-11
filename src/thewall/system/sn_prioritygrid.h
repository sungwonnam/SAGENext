#ifndef PRIORITYGRID_H
#define PRIORITYGRID_H

#include <QtGui>

class QGraphicsScene;

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



class SN_BaseWidget;

class SN_PriorityGrid : public QObject
{
	Q_OBJECT
public:
//	explicit SN_PriorityGrid(QObject *parent=0) : QObject(parent), _thescene(0), _isEnabled(false) {}

	/*!
	  rectSize defines the size of each cell in the grid
	  */
	explicit SN_PriorityGrid(const QSize &rectSize, QGraphicsScene *scene, QObject *parent = 0);

    explicit SN_PriorityGrid(int numrow, int numcol, QGraphicsScene *scene, QObject *parent = 0);

    ~SN_PriorityGrid();

	inline int dimx() const {return _dimx;}
	inline int dimy() const {return _dimy;}

	inline void setScene(QGraphicsScene *s) {_thescene = s;}

	inline void setRectSize(const QSize &s) {_rectSize = s;}
	inline void setRectSize(int w, int h) {_rectSize = QSize(w, h);}

	inline bool isEnabled() const {return _isEnabled;}

    inline qint64 gridStartTimeSinceEpoch() const {return _gridStartTimeSinceEpoch;}

    /*!
      An application can call this function to add its priority value
      to the cells under its window
      */
    void addPriorityOfApp(const QRect &windowRect_sceneCoord, qreal priority);


	/*!
	  An application adds its priority to the rects intersects with its window
	  */
//	int addPriority(const QRect &windowSceneRect, qreal priorityvalue);

	/*!
	  Returns the aggregated priority value covered by the scenerect
	  */
	qreal getPriorityOffset(const QRect &scenerect);

	/*!
	  returns the priority value of the grid item at row, col
	  */
	qreal getPriorityOffset(int row, int col);

    qreal getPriorityOffset(const SN_BaseWidget *widget);
	
private:
    /*!
      num cells in a row
      */
	int _dimx;

    /*!
      num cells in a column
      */
	int _dimy;

	/*!
	  The region covers the entire scene
	  */
	QRegion _theSceneRegion;

	/*!
	  This array will hold aggregated priority values.
	  So the values in this vector will keep increasing!
	  */
	QVector<qreal> _priorityRawVec;

	/*!
	  This array holds normalized values.
	  */
	QVector<qreal> _priorityVec;

	QGraphicsScene *_thescene;

	QSize _rectSize;

	/*!
	  false until buildRectangle() returns
	  */
	bool _isEnabled;

    /*!
      The sum of the all the priority value of cells
      */
    qreal _theTotalPriority;

    /*!
      msec
      */
    qint64 _gridStartTimeSinceEpoch;

	void buildRectangles();

    qreal m_getTotalPriority();

signals:
	
public slots:

	/*!
      All the cell value needs to be updated in a bundle.

	  finds out app rects above each cell and updates values accordingly
	  */
	void updatePriorities();

    void printTheGrid();
	
};

#endif // PRIORITYGRID_H
