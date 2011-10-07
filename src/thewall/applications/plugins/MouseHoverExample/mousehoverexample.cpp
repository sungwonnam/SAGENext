#include "mousehoverexample.h"

//
// SAGENextPolygonArrow is defined in here
//
#include "../../../common/commonitem.h"


TrackerItem::TrackerItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem *parent)
	: QGraphicsEllipseItem(x,y,w,h,parent)
{
	setAcceptedMouseButtons(0);
}
void TrackerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->setBrush(QBrush(Qt::red));
	painter->drawEllipse(boundingRect());
}



MouseHoverExample::MouseHoverExample()
    : SN_BaseWidget(Qt::Window)
	, _hoverFlag(false)
	, _marginleft(10)
	, _marginright(10)
	, _margintop(40)
	, _marginbottom(10)
{
	setContentsMargins(_marginleft, _margintop, _marginright, _marginbottom);


	//
	// register myself to the hoveraccepting app list of the scene
	//
	setRegisterForMouseHover(true);


	resize(1024, 768);
}

MouseHoverExample::~MouseHoverExample()
{
}

SN_BaseWidget * MouseHoverExample::createInstance() {
	return new MouseHoverExample;
}


void MouseHoverExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->fillRect(_marginleft, _margintop, size().width()-_marginleft-_marginright, size().height()-_margintop-_marginbottom, Qt::lightGray);
}

void MouseHoverExample::handlePointerHover(SN_PolygonArrowPointer *pointer, const QPointF &pointerPosOnMe, bool isHovering) {

	//
	// update the map first
	//
	_pointerMap.insert(pointer, QPair<QPointF, bool>(pointerPosOnMe, isHovering));

	QMap<SN_PolygonArrowPointer *, QPair<QPointF, bool> >::const_iterator it = _pointerMap.constBegin();

	if ( isHovering ) {
		//
		// Something is hovering on me !
		//


		// handle mouse hovering. For example,..
		_hoverFlag = true;

		//
		// find out where the pointers are hovering to handle each pointer's hovering separately
		//
		for (; it!=_pointerMap.constEnd(); it++) {

			//
			// if a pointer is hovering on me now
			//
			if (it.value().second == true) {
//				qDebug() << "pointer id" << it.key()->id() << "is hovering on" << it.value().first << "in my local coordinate";

				// a tracker item for each pointer
				TrackerItem *tracker = 0;
				if ( ! (tracker = _hoverTrackerItemList.value(pointer->id(), 0)) ) {
					 tracker = new TrackerItem(0,0,64,64, this);
//					 tracker->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
					_hoverTrackerItemList.insert(pointer->id(), tracker);
				}

				//
				// stalking the pointer
				//
				tracker->setPos( pointerPosOnMe.x() - 32 , pointerPosOnMe.y() - 32 );
			}
		}
	}
	else {
		//
		// Something informed that it doesn't hover anymore.
		// But we still need to check because there still can be other pointers hovering on me. (Multiuser interaction !!)
		//

		//
		// So, find any pointer who is still hovering on me
		//
		for (; it!=_pointerMap.constEnd(); it++) {
			if ( it.value().second == true ) {
				// Found one!
				// so do nothing, return
				return;
			}
		}

		//
		// if control reaches here then I'm sure that nothing is hovering on me
		//
		_hoverFlag = false;
	}
}



Q_EXPORT_PLUGIN2(MouseHoverExamplePlugin, MouseHoverExample)
