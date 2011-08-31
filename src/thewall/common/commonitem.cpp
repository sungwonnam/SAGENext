#include "commonitem.h"
#include "../applications/base/basewidget.h"
#include "../sagenextscene.h"

//#include <typeinfo>


PolygonArrow::PolygonArrow(const quint64 uicid, const QSettings *s, const QColor c, QGraphicsItem *parent)
	: QGraphicsPolygonItem(parent), uiclientid(uicid), settings(s)

{
	QPolygonF p;
	p << QPointF(0,0) << QPointF(60, 20) << QPointF(46, 34) << QPointF(71, 59) << QPointF(60, 70) << QPointF(35, 45) << QPointF(20, 60) << QPointF(0,0);

	setBrush(QBrush(c));
	setPolygon(p);

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setZValue(9999999); // the top most

	app = 0;
	textItem = 0;
}

void PolygonArrow::pointerMove(const QPointF &_scenePos, Qt::MouseButtons btnFlags) {
	qreal deltax = _scenePos.x() - scenePos().x();
	qreal deltay = _scenePos.y() - scenePos().y();

	// move pointer itself
	setPos(_scenePos);


	// if dragging
	if ( btnFlags & Qt::LeftButton ) {

		/* because of pointer press, appUnderPointer has already set at this point */

		if (app) {
//			qDebug() << app->resizeHandleSceneRect() << _scenePos << deltax << deltay;
			if ( app->isWindow()  &&  app->resizeHandleSceneRect().contains(_scenePos)) {

				// resize doesn't count window frame
				qreal top, left, right, bottom;
				app->getWindowFrameMargins(&left, &top, &right, &bottom);

				app->resize(app->boundingRect().width()-left-right + deltax, app->boundingRect().height()-top-bottom + deltay); // should have more PIXEL : not scaling

				/*
				 I shouldn't simulate mouse move event like this..
				 Because event keeps recording last known mouse event position
				 If there are multiple virtual mouses, event->lastPos keeps garbled.
				 */
//				foreach(QGraphicsView *gview, scene()->views()) {
//					app->grabMouse();
//					if ( ! QApplication::sendEvent(gview->viewport(), &QMouseEvent(QEvent::MouseMove, gview->mapFromScene(scenePos), gview->mapToGlobal(gview->mapFromScene(scenePos)), Qt::LeftButton, btnFlags, Qt::NoModifier)) ) {
//						qDebug("PolygonArrow::%s() : sendEvent failed", __FUNCTION__);
//					}
//					app->ungrabMouse();
//				}
			}
			else {
				// move app widget under this pointer
				app->moveBy(deltax, deltay);


			}
		}
	}
}

void PolygonArrow::pointerPress(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {
	if (setAppUnderPointer(scenePos)) {
		/* generate mousePressEvent */
		/* This way, I can forward this event to child items which are interactive (such as QGraphicsWebView) */

		/**
		// ASYNCHRONOUS (postEvent return immediately)
		QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonDblClick, gview->mapFromScene(scenePos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
		QApplication::postEvent(gview->viewport(), e); // VIEWPORT() !!! f*ck
//		QApplication::sendPostedEvents(gview->viewport(), e->type()); // Immediately dispatch the event to the receiver
		**/

		/**
		  // SYNCHRONOUS (sendEvent blocks)
		app->grabMouse();
		if ( ! QApplication::sendEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(scenePos), btn, btnFlags, Qt::NoModifier)) ) {
			qDebug("PolygonArrow::%s() : sendEvent failed", __FUNCTION__);
		}
		app->ungrabMouse();
		**/

		/* OR direct control */
//		app->setTopmost();
	}
}

/*
void PolygonArrow::pointerRelease(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {
	app->grabMouse();
	if ( ! QApplication::sendEvent(gview->viewport(), &QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(scenePos), btn, btnFlags, Qt::NoModifier)) ) {
		qDebug("PolygonArrow::%s() : sendEvent failed", __FUNCTION__);
	}
	app->ungrabMouse();
}
*/

void PolygonArrow::pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {

//	setAppUnderPointer(scenePos);

	if (!app) {
		qDebug("PolygonArrow::%s() : There's no app widget under this pointer", __FUNCTION__);
		return;
	}
	/**
	if (!gview) {
		qDebug("PolygonArrow::%s() : gview is null, can't postEvent to the view", __FUNCTION__);
		return;
	}

	// let's just generate mouseDoubleClickEvent !
	// event object is going to be deleted by eventHandler.
	QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonDblClick, gview->mapFromScene(scenePos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

	app->grabMouse(); // make sure app is BaseGraphicsWidget type. not child item
	QApplication::postEvent(gview->viewport(), e); // VIEWPORT() !!! f*ck
	app->ungrabMouse();
//	QApplication::sendPostedEvents(gview->viewport(), e->type());
	**/


	// Or directly control the widget
	app->maximize();
}

void PolygonArrow::pointerWheel(const QPointF &scenePos, int delta /*=120*/) {
	// delta is usually multiple of 120 (==15 * 8)
	foreach (QGraphicsView *gview, scene()->views()) {

		if ( ! QApplication::sendEvent(gview->viewport(), &QWheelEvent(gview->mapFromScene(scenePos), gview->mapToGlobal(scenePos.toPoint()), delta, Qt::NoButton, Qt::NoModifier)) ) {
			qDebug("PolygonArrow::%s() : send wheelEvent failed", __FUNCTION__);
		}
		else {
//			qDebug() << "PolygonArrow wheel" << gview->mapFromScene(scenePos);
		}
	}
}

void PolygonArrow::pointerClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {
	// should be sent to interactive item like gwebview

	/*
	app->grabMouse();
	if ( ! QApplication::sendEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(scenePos), btn, btnFlags, Qt::NoModifier)) ) {
		qDebug("PolygonArrow::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
	}
	if ( ! QApplication::sendEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(scenePos), btn, btnFlags, Qt::NoModifier)) ) {
		qDebug("PolygonArrow::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
	}
	app->ungrabMouse();
	*/

	Q_UNUSED(btnFlags);
//	setAppUnderPointer(scenePos);
	if (app) {
		app->setTopmost();

		/**
		  * PixmapWidget, SageStreamWidget don't reimplement this function
		  * Curretnly, only WebWidget reimplement this function
		  */
		app->mouseClick(scenePos, btn);
	}
}

PolygonArrow::~PolygonArrow() {
	foreach (QGraphicsView *gview, scene()->views()) {
		Q_ASSERT(gview);
		gview->scene()->removeItem(this);
	}
}

void PolygonArrow::setPointerName(const QString &text) {
	textItem = new QGraphicsSimpleTextItem(text, this);
	textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
	textItem->setFlag(QGraphicsItem::ItemIsMovable, false);
	textItem->setBrush(Qt::white);
	textItem->moveBy(0, boundingRect().height());
}

bool PolygonArrow::setAppUnderPointer(const QPointF scenePos) {
	QList<QGraphicsItem *> list = scene()->items(scenePos, Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
//	app = static_cast<BaseGraphicsWidget *>(scene()->itemAt(scenePosOfPointer, QTransform()));

//	qDebug() << "\nPolygonArrow::setAppUnderPointer() :" << list.size() << " items";
	foreach (QGraphicsItem *item, list) {
		if ( item == this ) continue;

//		qDebug() << item;

		if ( item->type() == UserType + 2) {
			app = static_cast<BaseWidget *>(item);
//			qDebug("PolygonArrow::%s() : uiclientid %llu, appid %llu", __FUNCTION__, uiclientid, app->globalAppId());
			return true;
		}

	}
	app = 0; // reset

//	qDebug("PolygonArrow::%s() : uiclientid %llu, There's BaseGraphicsWidget type under pointer", __FUNCTION__, uiclientid);
	return false;
}









SwSimpleTextItem::SwSimpleTextItem(int ps, QGraphicsItem *parent) :
		QGraphicsSimpleTextItem(parent)
{
	if ( ps > 0 ) {
		QFont f;
		f.setPointSize(ps);
//		setBrush(Qt::white);
		//setPen(QPen(Qt::black));
		setFont(f);
	}

	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}

SwSimpleTextItem::~SwSimpleTextItem() {

}

void SwSimpleTextItem::wheelEvent(QGraphicsSceneWheelEvent *event) {
	int numDegrees = event->delta() / 8;
	int numTicks = numDegrees / 15;
//	qDebug("SwSimpleTextItem::%s() : delta %d numDegrees %d numTicks %d", __FUNCTION__, event->delta(), numDegrees, numTicks);

	qreal s = scale();
	if ( numTicks > 0 ) {
		s += 0.1;
	}
	else {
		s -= 0.1;
	}
//	prepareGeometryChange();
	setScale(s);

}

void SwSimpleTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->fillRect(boundingRect(), Qt::lightGray);
	QGraphicsSimpleTextItem::paint(painter, option, widget);
}










PixmapCloseButton::PixmapCloseButton(const QString res, QGraphicsItem *parent)
	: QGraphicsPixmapItem(parent)
{
	setPixmap(QPixmap(res));
	flag = false;
}


void PixmapCloseButton::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(event);
	flag = true;
}

void PixmapCloseButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (flag && boundingRect().contains(event->pos())) {
		QList<QGraphicsView *> vlist = scene()->views();
		QGraphicsView *gview = vlist.front();
		QMetaObject::invokeMethod(gview, "close", Qt::QueuedConnection);
	}
}





PixmapCloseButtonOnScene::PixmapCloseButtonOnScene(const QString res, QGraphicsItem *parent)
	: QGraphicsPixmapItem(parent)
{
	setPixmap(QPixmap(res));
	flag = false;
}

void PixmapCloseButtonOnScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(event);
	flag = true;
}

void PixmapCloseButtonOnScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (flag && boundingRect().contains(event->pos())) {
		SAGENextScene *s = qobject_cast<SAGENextScene *>(scene());
		s->deleteLater();
	}
}





PixmapArrow::PixmapArrow(const quint64 uicid, QGraphicsItem *parent)
	: QGraphicsPixmapItem(parent), uiclientid(uicid)
{
	QPixmap ar(":/resources/arrow.png");
	setPixmap(ar);
	setRotation(120);

	textItem = 0;
	app = 0;

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

void PixmapArrow::setPointerName(const QString &text) {
	textItem = new QGraphicsSimpleTextItem(text, this);
	textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
	textItem->setFlag(QGraphicsItem::ItemIsMovable, false);
	textItem->moveBy(0, boundingRect().height());
}



