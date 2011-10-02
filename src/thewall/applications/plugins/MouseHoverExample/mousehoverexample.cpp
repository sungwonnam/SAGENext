#include "mousehoverexample.h"
#include "../../../common/commonitem.h"

#include <QSettings>


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
    : BaseWidget(Qt::Window)
	, _textItem(0)
	, _hoverFlag(false)
	, _marginleft(8)
	, _marginright(8)
	, _margintop(28)
	, _marginbottom(8)
{
//	_textItem = new QGraphicsSimpleTextItem("Pointer hovering\n", this);
//	QBrush brush(Qt::white);
//	_textItem->setBrush(brush);

	//
	// register myself to the hoveraccepting app list of the scene
	//
	setRegisterForMouseHover(true);

	resize(800, 600);

//	_textItem->hide();

//	setWindowFrameMargins(0, 0, 0, 0);
	setContentsMargins(_marginleft, _margintop, _marginright, _marginbottom);
//	qDebug() << boundingRect();
}

MouseHoverExample::~MouseHoverExample()
{
}

QString MouseHoverExample::name() const {
	return "BaseWidget";
}

QGraphicsProxyWidget * MouseHoverExample::rootWidget() {
	return 0;
}

void MouseHoverExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	painter->fillRect(_marginleft, _margintop, size().width()-_marginleft-_marginright, size().height()-_margintop-_marginbottom, Qt::lightGray);
}

void MouseHoverExample::resizeEvent(QGraphicsSceneResizeEvent *e) {
//	_textItem->setPos(e->newSize().width()/2 - _textItem->boundingRect().size().width()/2
//					  ,e->newSize().height()/2 - _textItem->boundingRect().size().height()/2);
}

void MouseHoverExample::toggleHover(SAGENextPolygonArrow *pointer, const QPointF &pointerPosOnMe, bool isHovering) {

	//
	// update the map first
	//
	_pointerMap.insert(pointer, QPair<QPointF, bool>(pointerPosOnMe, isHovering));

	QMap<SAGENextPolygonArrow *, QPair<QPointF, bool> >::const_iterator it = _pointerMap.constBegin();

	if ( isHovering ) {
		// handle mouse hovering. For example,..
		_hoverFlag = true;
//		_textItem->show();
//		update(); // to schedule paint(). If you don't need to repaint then don't call update()

		//
		// find out where the pointers are hovering to handle each pointer's hovering separately
		//
		for (; it!=_pointerMap.constEnd(); it++) {
			if (it.value().second == true) {
//				qDebug() << "pointer id" << it.key()->id() << "is hovering on" << it.value().first << "in my local coordinate";

				TrackerItem *tracker = 0;
				if ( ! (tracker = _hoverTrackerItemList.value(pointer->id(), 0)) ) {
					 tracker = new TrackerItem(0,0,64,64, this);
//					 tracker->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
					_hoverTrackerItemList.insert(pointer->id(), tracker);
				}
				tracker->setPos( pointerPosOnMe.x() - 32 , pointerPosOnMe.y() - 32 );
			}
		}
	}
	else {
		//
		// find any pointer who is still hovering on me
		//
		for (; it!=_pointerMap.constEnd(); it++) {
			if ( it.value().second == true ) {
				// there still is a pointer hovering on me, so do nothing and just return
				return;
			}
		}

		// if control reaches here then nothing is hovering on me
		// handle hover leave
		_hoverFlag = false;
//		_textItem->hide();
//		update(); // to schedule paint(). If you don't need to repaint then don't call update()
	}
}



Q_EXPORT_PLUGIN2(MouseHoverExamplePlugin, MouseHoverExample)
