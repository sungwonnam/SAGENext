#include "mousehoverexample.h"
#include "../../../common/commonitem.h"

MouseHoverExample::MouseHoverExample()
    : BaseWidget(Qt::Window)
	, _textItem(0)
	, _hoverFlag(false)
{
	_textItem = new QGraphicsSimpleTextItem("Pointer hovering\n", this);
	QBrush brush(Qt::white);
	_textItem->setBrush(brush);

	//
	// register myself to the hoveraccepting app list of the scene
	//
	setRegisterForMouseHover(true);

	resize(800, 600);

	_textItem->hide();

	setWindowFrameMargins(0, 0, 0, 0);
	setContentsMargins(4, 4, 24, 4);
	qDebug() << boundingRect();
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
	if (_hoverFlag) {
		painter->fillRect(boundingRect(), Qt::darkCyan);
	}
	else {
		painter->fillRect(boundingRect(), Qt::darkGray);
	}
}

void MouseHoverExample::resizeEvent(QGraphicsSceneResizeEvent *e) {
	_textItem->setPos(e->newSize().width()/2 - _textItem->boundingRect().size().width()/2
					  ,e->newSize().height()/2 - _textItem->boundingRect().size().height()/2);
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
		_textItem->show();
		update(); // to schedule paint(). If you don't need to repaint then don't call update()

		//
		// find out where the pointers are hovering to handle each pointer's hovering separately
		//
		for (; it!=_pointerMap.constEnd(); it++) {
			if (it.value().second == true) {
//				qDebug() << "pointer id" << it.key()->id() << "is hovering on" << it.value().first << "in my local coordinate";
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
		_textItem->hide();
		update(); // to schedule paint(). If you don't need to repaint then don't call update()
	}
}



Q_EXPORT_PLUGIN2(MouseHoverExamplePlugin, MouseHoverExample)
