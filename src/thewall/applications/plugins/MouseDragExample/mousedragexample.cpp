#include "mousedragexample.h"

#include <common/commonitem.h>


TrackerItem::TrackerItem(qreal x, qreal y, qreal w, qreal h, const QBrush &brush, QGraphicsItem *parent)
	: QGraphicsEllipseItem(x,y,w,h,parent)
    , _brush(brush)
{
	/**
	  If this item is not interested in real mouse event.
	  If only draggin is needed, then set it to 0.
	  If this item has to receive click, rightclick, and so on. then comment it
	  */
	setAcceptedMouseButtons(0);

	/**
	  If true, this item will be stacked behind the MouseDragExample,
	  so any mouse event can't be reached to this item unless MouseDragExample ignore the events.

	  And item won't be visible unless MouseDragExample paint with transparent color
	  */
//	setFlag(QGraphicsItem::ItemStacksBehindParent, true);
}

void TrackerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->setBrush(_brush);
	painter->drawEllipse(boundingRect());
}




MouseDragExample::MouseDragExample()
    : SN_BaseWidget(Qt::Window)
    , _numItems(10)
    , _marginleft(10)
    , _marginright(10)
    , _margintop(40)
    , _marginbottom(10)
{

	/**
	  when you set layout on this widget ( QGraphicsWidget::setLayout() ),
	  The layout will not cover entire window if below is set greater than 0
	  */
	setContentsMargins(_marginleft, _margintop, _marginright, _marginbottom);

	for (int i=0; i<_numItems; i++) {
		QColor color(128, i * 12, 255 / (i+1));

		// MouseDragExample's child item
		TrackerItem *ti = new TrackerItem(0,0,128,128, QBrush(color), this);

		ti->moveBy(_marginleft + i * 64, _margintop);
	}

	resize(1024, 768);
}

MouseDragExample::~MouseDragExample()
{
	// TrackerItem doesn't need to be deleted explicitly because child items will be deleted automatically
}

/**
  Don't forget this
  */
SN_BaseWidget * MouseDragExample::createInstance() {
	return new MouseDragExample;
}

TrackerItem * MouseDragExample::getTrackerItemUnderScenePos(const QPointF &scenepos) {

	foreach(QGraphicsItem *child, childItems()) {
		TrackerItem *tracker = static_cast<TrackerItem *>(child);
		if (!tracker) continue;

		/**
		  if tracker's rectangle contains the scenepos
		  then this tracker is under the pointer
		  */
		if (tracker->rect().contains( tracker->mapFromScene(scenepos) )) {
			return tracker;
		}
	}
	return 0;
}

void MouseDragExample::handlePointerDrag(const QPointF &scenePos, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier) {

	TrackerItem *tracker = getTrackerItemUnderScenePos(scenePos);
	if (tracker) {
		if (button == Qt::LeftButton) {
			tracker->moveBy(pointerDeltaX, pointerDeltaY);

			// Todo:
			// need to prevent tracker item from moving outside of widget's window
		}
		else if (button == Qt::RightButton) {

		}
		else {

		}
	}
	else {
		/**
		  to keep the base implementation -> moves window itself
		  */
//		if ( scenePos is in dragging handle )
		SN_BaseWidget::handlePointerDrag(scenePos, pointerDeltaX, pointerDeltaY, button, modifier);
	}

}

void MouseDragExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->fillRect(_marginleft, _margintop, size().width() - _marginleft - _marginright, size().height() - _margintop - _marginbottom, QBrush(QColor(200,200,200,128)));
}




Q_EXPORT_PLUGIN2(MouseDragExamplePlugin, MouseDragExample)
