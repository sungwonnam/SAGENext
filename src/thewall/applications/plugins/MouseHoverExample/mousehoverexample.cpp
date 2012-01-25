#include "mousehoverexample.h"

//
// SAGENextPolygonArrow is defined in here
//
#include <common/sn_sharedpointer.h>
#include <common/commondefinitions.h>


TrackerItem::TrackerItem(qreal x, qreal y, qreal w, qreal h, const QColor &c, QGraphicsItem *parent)
	: QGraphicsItem(parent)
    , _color(c)
    , _size(QSizeF(w,h))
{
	setAcceptedMouseButtons(0);
	_startTime = QTime::currentTime();
}

QRectF TrackerItem::boundingRect() const {
	return QRectF( -_size.width()/2 , -_size.height()/2, _size.width() , _size.height());
}

void TrackerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
//	painter->setBrush(QBrush(_color));
//	painter->drawEllipse(boundingRect());

	int dt = _startTime.msecsTo(QTime::currentTime());
	int m_size = _size.width();

    qreal r0 = 0.5 * m_size * (1.0 - exp(-0.001 * ((dt + 3800) % 4000)));
    qreal r1 = 0.5 * m_size * (1.0 - exp(-0.001 * ((dt + 0) % 4000)));
    qreal r2 = 0.5 * m_size * (1.0 - exp(-0.001 * ((dt + 1800) % 4000)));
    qreal r3 = 0.5 * m_size * (1.0 - exp(-0.001 * ((dt + 2000) % 4000)));

    if (r0 > r1)
        r0 = 0.0;
    if (r2 > r3)
        r2 = 0.0;

    QPainterPath path;
    path.moveTo(r1, 0.0);
    path.arcTo(-r1, -r1, 2 * r1, 2 * r1, 0.0, 360.0);
    path.lineTo(r0, 0.0);
    path.arcTo(-r0, -r0, 2 * r0, 2 * r0, 0.0, -360.0);
    path.closeSubpath();
    path.moveTo(r3, 0.0);
    path.arcTo(-r3, -r3, 2 * r3, 2 * r3, 0.0, 360.0);
    path.lineTo(r0, 0.0);
    path.arcTo(-r2, -r2, 2 * r2, 2 * r2, 0.0, -360.0);
    path.closeSubpath();
//    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(QBrush(_color));
    painter->setPen(Qt::NoPen);
    painter->drawPath(path);
//    painter->setBrush(Qt::NoBrush);
//    painter->setPen(Qt::SolidLine);
//    painter->setRenderHint(QPainter::Antialiasing, false);
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
	painter->drawStaticText(100, 100, QStaticText("Hover on me!"));
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
				if ( ! (tracker = _hoverTrackerItemList.value(pointer, 0)) ) {
					 tracker = new TrackerItem(0,0,128,128, pointer->color(), this);
//					 tracker->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
					_hoverTrackerItemList.insert(pointer, tracker);
				}

				//
				// stalking the pointer
				//
				tracker->show();
				tracker->setPos( pointerPosOnMe.x(), pointerPosOnMe.y() );
			}
		}
	}
	else {
		//
		// The pointer informed that it doesn't hover anymore.
		// But we still need to check because there still can be other pointers hovering on me. (Multiuser interaction !!)
		//
		TrackerItem *t = _hoverTrackerItemList.value(pointer, 0);
		if (t) {
			t->hide();
		}

		//
		// So, find any pointer who is still hovering on me
		//
		for (; it!=_pointerMap.constEnd(); it++) {
			if ( it.value().second == true ) {
				// Found one!
				// so there is at least one pointer currently hovering on me. do nothing, return
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
