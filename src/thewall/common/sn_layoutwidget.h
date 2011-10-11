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

	inline SN_LayoutWidget * firstChildLayout() {return _firstChildLayout;}
	inline SN_LayoutWidget * secondChildLayout() {return _secondChildLayout;}

	/**
	  Add widget as my child. This will automatically add widget to the scene
	  */
	void addItem(SN_BaseWidget *bw, const QPointF &pos = QPointF(30, 30));

	/**
	  Reparent all the basewidgets to the new layoutWidget
	  */
	void reparentWidgets(SN_LayoutWidget *newParent);

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

	void doTile();

signals:
	void resized();

public slots:
	/**
	  create horizontal bar that partitions this widget into top and bottom
	  */
	inline void createHBar() {createChildPartitions(Qt::Horizontal);}

	/**
	  creates vertical bar that partitions this widget into left and right
	  */
	inline void createVBar() {createChildPartitions(Qt::Vertical);}

	// I'll become actual partition
	void deleteChildPartitions();

	void adjustBar();


	/**
	  move child basewidget so that its window reside within my bounding rectangle
	  */
	void adjustChildPos();


	void toggleTile();

	/**
	  */
	void saveSession(QDataStream &out);

	void loadSession(QDataStream &in, SN_Launcher *launcher);
};



#endif // SN_LAYOUTWIDGET_H
