#ifndef MOUSEDRAGEXAMPLE_H
#define MOUSEDRAGEXAMPLE_H

#include <QtGui>

#include <applications/base/basewidget.h>
#include <applications/base/SN_plugininterface.h>

/**
  A mouse draggable item on the MouseDragExample widget
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

    /*!
      TrackerItem doesn't need to be deleted explicitly because child items will be deleted automatically by Qt
      */
    ~MouseDragExample() {}


	/**
	  Implementing the interface of the SN_PluginInterface
	  */
	SN_BaseWidget * createInstance();


    /*!
      Reimplementing the SN_BaseWidget::handlePointerPress()
      */
	void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

    /*!
      Reimplementing the SN_BaseWidget::handlePointerRelease()
      */
	void handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

	/**
	  Reimplementing the SN_BaseWidget::handlePointerDrag()
	  */
	void handlePointerDrag(SN_PolygonArrowPointer * pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);

	/**
	  I have nothing to draw on the widget's surface.
	  But let's just draw background color in the content area
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

    /*!
      The tracker items that are currently being dragged by users are stored in here
      */
	QMap<SN_PolygonArrowPointer *, TrackerItem *> _dragTrackerMap;

	/**
	  Returns the tracker item under the point.
      point is in this widget's local coordinate.
	  */
	TrackerItem * getTrackerItemUnderPoint(const QPointF &point);
};

#endif // MOUSEDRAGEXAMPLE_H
