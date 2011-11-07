#ifndef MOUSEDRAGEXAMPLE_H
#define MOUSEDRAGEXAMPLE_H

#include <QtGui>

/**
  If INCLUDEPATH is set correctly in the .pro file, I can include header files like this
  */
#include <applications/base/basewidget.h>
#include <applications/base/SN_plugininterface.h>




/**
  A draggable item
  */
class TrackerItem : public QGraphicsEllipseItem
{
public:
	TrackerItem(qreal x, qreal y, qreal w, qreal h, const QBrush &brush, QGraphicsItem *parent = 0);
protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
private:
	QBrush _brush;
};






class MouseDragExample : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
	Q_INTERFACES(SN_PluginInterface)

public:
    MouseDragExample();
    ~MouseDragExample();


	/**
	  Implementing pure virtual defined in SN_PluginInterface
	  */
	SN_BaseWidget * createInstance();


	void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

	/**
	  Reimplementing SN_BaseWidget::handlePointerDrag()
	  */
	void handlePointerDrag(SN_PolygonArrowPointer * pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);

protected:
	/**
	  I have nothing to draw.
	  But let's just draw bg color in the content area
	  */
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


private:

	/**
	  The number of draggable items
	  */
	int _numItems;


	/**
	  The contents margins to show window frame color defined in SN_BaseWidget::paintWindowFrame()
	  By default, in SN_BaseWidget, content margin for item type Qt::Window is set to 4, 24, 4, 4 (left, top, right, bottom)
	  */
	int _marginleft;
	int _marginright;
	int _margintop;
	int _marginbottom;

	QMap<SN_PolygonArrowPointer *, TrackerItem *> _dragTrackerMap;

	/**
	  point is in this example's local coordinate
	  */
	TrackerItem * getTrackerItemUnderPoint(const QPointF &point);
};

#endif // MOUSEDRAGEXAMPLE_H
