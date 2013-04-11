#include "mousedragexample.h"

#include <common/sn_commonitem.h>
#include <common/sn_commondefinitions.h>

TrackerItem::TrackerItem(qreal x, qreal y, qreal w, qreal h, const QBrush &brush, QGraphicsItem *parent)
	: QGraphicsEllipseItem(x,y,w,h,parent)
    , _brush(brush)
{
    //
    // If you want to control how this item reacts then set this so that SN_PolygonArrowPointer ignores this
    //
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
}

/**
  Implementing the interface.
  All the child items will be instantiated in here
  */
SN_BaseWidget * MouseDragExample::createInstance() {

    MouseDragExample *instance = new MouseDragExample;

    for (int i=0; i<_numItems; i++) {
        // A color for each tracker item
		QColor color(128, i * 12, 255 / (i+1));

		// MouseDragExample's child item
		TrackerItem *ti = new TrackerItem(0,0,128,128, QBrush(color), instance);

        // initial position
		ti->moveBy(_marginleft + i * 64, _margintop);
	}

    instance->resize(1280, 1024);

	return instance;
}

TrackerItem * MouseDragExample::getTrackerItemUnderPoint(const QPointF &point) {
	QList<QGraphicsItem *> children = childItems();

	for (int i=children.size()-1; i>=0; i--) {
//		TrackerItem *tracker = static_cast<TrackerItem *>(children.at(i));
		QGraphicsItem *item = children.at(i);
		if(item && item->boundingRect().contains(item->mapFromParent(point)))
			return static_cast<TrackerItem *>(item);
	}

	return 0;
}

void MouseDragExample::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
	qreal maxZ = 0.0;
	TrackerItem *t = getTrackerItemUnderPoint(point);

    //
    // If user's pointer is pressed on a tracker item then
    //
	if (t) {
        // widget is not in the "moving" state
        _isMoving = false;

		QMap<SN_PolygonArrowPointer *, TrackerItem *>::const_iterator it;
		for (it=_dragTrackerMap.constBegin(); it!=_dragTrackerMap.constEnd(); it++) {
			TrackerItem *ti = it.value();
			Q_ASSERT(ti);
			maxZ = qMax(maxZ, ti->zValue());
		}
		t->setZValue(maxZ + 0.1); // set top most child item

		_dragTrackerMap.insert(pointer, t);
	}

    //
    // user's pointer is pressed on a point on which there's no tracker item
    //
	else {
		_dragTrackerMap.erase(_dragTrackerMap.find(pointer));

        //
        // Do whatever the base implementation does
        //
        SN_BaseWidget::handlePointerPress(pointer, point, btn);
	}
}

void MouseDragExample::handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {

    // a user's pointer done interacting (dragging) with a tracker item
	_dragTrackerMap.erase(_dragTrackerMap.find(pointer));

    SN_BaseWidget::handlePointerRelease(pointer, point, btn);
}

void MouseDragExample::handlePointerDrag(SN_PolygonArrowPointer * pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier) {

	TrackerItem *tracker = _dragTrackerMap.value(pointer, 0);

    //
	// If there is a trackeritem under the pointer then
	//
	if (tracker) {
		if (button == Qt::LeftButton) {

			QPointF trackerCenter = tracker->mapToParent(tracker->boundingRect().center());

			// And the tracker resides in this widget
			if (boundingRect().contains(trackerCenter))

				// then move the tracker
				tracker->moveBy(pointerDeltaX, pointerDeltaY);

		}
	}
	else {
		/**
		  keep the base implementation -> moves this example window itself or resize the window
		  */
		SN_BaseWidget::handlePointerDrag(pointer, point, pointerDeltaX, pointerDeltaY, button, modifier);
	}

}

void MouseDragExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->fillRect(_marginleft, _margintop, size().width() - _marginleft - _marginright, size().height() - _margintop - _marginbottom, QBrush(QColor(200,200,200,128)));
}



#ifndef QT5
Q_EXPORT_PLUGIN2(MouseDragExamplePlugin, MouseDragExample)
#endif
