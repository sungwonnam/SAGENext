#ifndef SN_LAYOUTWIDGET_H
#define SN_LAYOUTWIDGET_H

#include <QtGui>


class SN_LayoutWidget;
class SN_Launcher;
class SN_PixmapButton;
class SN_BaseWidget;

class SN_WallPartitionBar : public QGraphicsLineItem {
public:
	SN_WallPartitionBar(Qt::Orientation o, SN_LayoutWidget *owner, QGraphicsItem *parent=0);

	inline Qt::Orientation orientation() const {return _orientation;}
	inline SN_LayoutWidget * ownerNode() {return _ownerNode;}

protected:

private:
	SN_LayoutWidget *_ownerNode;
	Qt::Orientation _orientation;
};



class SN_GridLayout : public QGraphicsGridLayout
{
public:
	SN_GridLayout(QGraphicsLayoutItem *parent = 0);

	void setGeometry(const QRectF &rect);
};



/**
  will hold BaseWdigets or another SAGENextLayoutWidget
  */
class SN_LayoutWidget : public QGraphicsWidget {
	Q_OBJECT
public:
	SN_LayoutWidget(const QString &posStr, SN_LayoutWidget *parentWidget, const QSettings *s, QGraphicsItem *parent=0);
	~SN_LayoutWidget();

//	inline SN_LayoutWidget * leftWidget() {return _leftWidget;}
//	inline SN_LayoutWidget * rightWidget() {return _rightWidget;}

//	inline SN_LayoutWidget * topWidget() {return _topWidget;}
//	inline SN_LayoutWidget * bottomWidget() {return _bottomWidget;}

	inline QString position() const {return _position;}

	inline void setParentLayoutWidget(SN_LayoutWidget *p) {_parentLayoutWidget = p;}
	inline SN_LayoutWidget * parentLayoutWidget() {return _parentLayoutWidget;}

	inline void setFirstChildLayout(SN_LayoutWidget *f) {_firstChildLayout = f;}
	inline void setSecondChildLayout(SN_LayoutWidget *s) {_secondChildLayout = s;}

	inline SN_LayoutWidget * firstChildLayout() {return _firstChildLayout;}
	inline SN_LayoutWidget * secondChildLayout() {return _secondChildLayout;}

	inline void setSiblingLayout(SN_LayoutWidget *s) {_siblingLayout = s;}
	inline SN_LayoutWidget * siblingLayout() {return _siblingLayout;}

	inline SN_WallPartitionBar * bar() {return _bar;}

	/**
	  Reparent all the basewidgets to the new layoutWidget
	  */
	void reparentMyChildBasewidgets(SN_LayoutWidget *newParent);

	/**
	  This will call resize()
	  */
	void setRectangle(const QRectF &r);

	/**
	  widget's center is (0,0)
	  */
//	QRectF boundingRect() const;

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	const QSettings *_settings;

	SN_LayoutWidget *_parentLayoutWidget;

	/**
	  If this widget has Vertical bar
	  */
//	SN_LayoutWidget *_leftWidget;
//	SN_LayoutWidget *_rightWidget;

	/**
	  If this widget has Horizontal bar
	  */
//	SN_LayoutWidget *_topWidget;
//	SN_LayoutWidget *_bottomWidget;

	SN_LayoutWidget *_firstChildLayout;
	SN_LayoutWidget *_secondChildLayout;

	SN_LayoutWidget *_siblingLayout;

	SN_WallPartitionBar *_bar;
	SN_PixmapButton *_tileButton;
	SN_PixmapButton *_hButton;
	SN_PixmapButton *_vButton;
	SN_PixmapButton *_xButton;

	QGraphicsItemGroup *_buttonGrp;

	bool _isTileOn;

	/**
	  left , right, top, or bottom
	  */
	QString _position;

	void createChildPartitions(Qt::Orientation barOrientation, const QRectF &first, const QRectF &second);

	void createChildPartitions(Qt::Orientation barOrientation);

	void setButtonPos();

signals:
	void resized();

public slots:
	/**
	  Add widget as my child. This will automatically add widget to the scene
	  */
	void addItem(SN_BaseWidget *bw, const QPointF &pos = QPointF(30, 30));

	/**
	  create horizontal bar that partitions this widget into top and bottom
	  */
	inline void createHBar() {createChildPartitions(Qt::Horizontal);}

	/**
	  creates vertical bar that partitions this widget into left and right
	  */
	inline void createVBar() {createChildPartitions(Qt::Vertical);}

	/**
	  My child layouts ( _firstChildLayout, _secondChildLayout) are going to be deleted
	  And their child SN_BaseWidget will become my child
	  */
	void deleteChildPartitions();

	/**
	  I will be deleted (This implies I don't have child layouts) and my child SN_BaseWidget will be moved to my sibling.
	  My sibling's rect() will be same as our parent's rect(). In fact my sibling will replace our parent layout
	  */
	void deleteMyself();

	/**
	  This slot is called when _firstChildLayout is resized.
	  */
	void adjustBar();

	/**
	  move child basewidget so that its window reside within my bounding rectangle.
	  0 left/right
	  1 up/down
	  */
	void adjustChildPos(int direction);

	void toggleTile();

	void doTile();

	/**
	  Recursively save layout and basewidgets.
	  */
	void saveSession(QDataStream &out);

	void loadSession(QDataStream &in, SN_Launcher *launcher);
};



#endif // SN_LAYOUTWIDGET_H
