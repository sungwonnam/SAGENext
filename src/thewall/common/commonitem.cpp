#include "commonitem.h"
#include "../applications/base/basewidget.h"
#include "../sagenextscene.h"


WidgetRemoveButton::WidgetRemoveButton(QGraphicsItem *parent) :
        QGraphicsPixmapItem(parent)
{
        setAcceptDrops(true);
}

void WidgetRemoveButton::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
        Q_UNUSED(event);

        //just highlight the button
}

void WidgetRemoveButton::dropEvent(QGraphicsSceneDragDropEvent *event) {
        Q_UNUSED(event);

        // find out the widget dragged to here and close it
}











PolygonArrow::PolygonArrow(const quint64 uicid, const QSettings *s, const QColor c, QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent)
    , uiclientid(uicid)
    , settings(s)
    , textItem(0)
    , app(0)
    , _item(0)
{
	QPolygonF p;
	p << QPointF(0,0) << QPointF(60, 20) << QPointF(46, 34) << QPointF(71, 59) << QPointF(60, 70) << QPointF(35, 45) << QPointF(20, 60) << QPointF(0,0);

	QPen pen;
	pen.setWidth(2); // 3 pixel
	pen.setColor(Qt::white);
	setPen(pen);

	setBrush(QBrush(c));
	setPolygon(p);

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setZValue(999999999); // the top most
}

PolygonArrow::~PolygonArrow() {
}

void PolygonArrow::setPointerName(const QString &text) {
	textItem = new QGraphicsSimpleTextItem(text, this);
	textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
	textItem->setFlag(QGraphicsItem::ItemIsMovable, false);

	QFont f;
	f.setPointSize(settings->value("gui/pointerfontsize", 20).toInt());
	f.setBold(true);
	textItem->setFont(f);

	textItem->setBrush(Qt::white);
	textItem->moveBy(0, boundingRect().height());
}

void PolygonArrow::pointerMove(const QPointF &_scenePos, Qt::MouseButtons btnFlags, Qt::KeyboardModifier modifier) {
    qreal deltax = _scenePos.x() - scenePos().x();
    qreal deltay = _scenePos.y() - scenePos().y();

	//
    // move pointer itself
    // Sets the position of the item to pos, which is in parent coordinates. For items with no parent, pos is in scene coordinates.
	//
    setPos(_scenePos);

	//
    // LEFT button mouse dragging
	//
    if ( btnFlags & Qt::LeftButton ) {

        /* Because of pointerPress, appUnderPointer has already set at this point */
        if (app) {
            //qDebug() << app->resizeHandleSceneRect() << _scenePos << deltax << deltay;
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
            }
            else {
                // move app widget under this pointer
                app->moveBy(deltax, deltay);
				app->mouseDrag( _scenePos, Qt::LeftButton, modifier);
            }
        }


		//
		// The PartitionBar item for instance
		//
		else if (_item) {
//			QGraphicsLineItem *l = qgraphicsitem_cast<QGraphicsLineItem *>(_item);
			PartitionBar *bar = dynamic_cast<PartitionBar *>(_item);
			if (bar) {
//				qDebug() << "pointerMove bar" << deltax << deltay;
				if ( bar->orientation() == Qt::Horizontal) {
					//
					// bar moves only up or down direction (y axis)

					SAGENextLayoutWidget *top = bar->ownerNode()->topWidget();
					SAGENextLayoutWidget *bottom = bar->ownerNode()->bottomWidget();
					top->resize(      top->size().width(),    top->size().height() + deltay);
					bottom->resize(bottom->size().width(), bottom->size().height() - deltay);
					bottom->moveBy(0, deltay);
				}
				else {
					// bar moves only left or right (x axis)

					SAGENextLayoutWidget *left = bar->ownerNode()->leftWidget();
					SAGENextLayoutWidget *right = bar->ownerNode()->rightWidget();
					left->resize(  left->size().width() + deltax,  left->size().height());
					right->resize(right->size().width() - deltax, right->size().height());
					right->moveBy(deltax, 0);
				}
			}
		}


        // Why not just send mouse event ??
        // Because Having multiple users simultaneously do mouse draggin will confuse the system
        /*
        QGraphicsView *view = 0;
        foreach (QGraphicsView *v, scene()->views()) {
            if ( v->rect().contains(v->mapFromScene(_scenePos)) ) {
                view=v;
                break;
            }
        }
        QMouseEvent mouseDragging(QMouseEvent::MouseMove, view->mapFromScene(_scenePos), Qt::LeftButton, btnFlags, Qt::NoModifier);
        if ( ! QApplication::sendEvent(view->viewport(), &mouseDragging) ) {
            qDebug() << "failed to send mouseMove event with left button";
        }
        */
    }
	else if (btnFlags & Qt::RightButton) {

	}
}

void PolygonArrow::pointerPress(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags, Qt::KeyboardModifier modifier) {
	Q_UNUSED(btnFlags);
	// note that this doesn't consider window frame
    if (!setAppUnderPointer(scenePos)) {
        qDebug() << "PolygonArrow::pointerPress() : setAppUnderPointer failed";
    }
	else {
//		qDebug() << "PolygonArrow::pointerPress() : got the app";
		if (btn == Qt::LeftButton) {
			if (app) app->setTopmost();
		}
	}
}


void PolygonArrow::pointerClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags, Qt::KeyboardModifier modifier) {
    /**
      Instead of generating mouse event,
      I can let each widget implement BaseWidget::mouseClick()
      **/
//	if (app) {
//		if (btn == Qt::RightButton) {
//			if ( app->isSelected() ) {
//				app->setSelected(false);
//				// to be effective, turn off WA_OpaquePaintEvent or set setAutoFillBackground(true)
//				app->palette().setColor(QPalette::Window, QColor(100, 100, 100, 128));
//			}
//			else {
//				app->setSelected(true);
//				app->palette().setColor(QPalette::Window, QColor(170, 170, 5, 164));
//			}
//		}

//		// Reimplement this for your app's specific needs

//		/**
//		Pointer can't know (in here) which widget is going to receive this event
//		because it depends on how child widgets are attached to the parent widget.

//		For example, gwebview of WebWidget will receive mouse event and gwebview isn't pointed by app. It's gwebview's parent (which is WebWidget) that is pointed by the app.
//	 So, app->grabMouse() is not a good idea especially when WebWidget installs eventFilter for its children
//		**/

//		app->mouseClick(scenePos, btn);
//	}


	/*
	  mouse event for graphics item
	  */
	/*
	else if (_item) {
		PartitionBar *snw = dynamic_cast<PartitionBar *>(_item);
		if (! snw) {
			qDebug() << "pointerClick() : it's not partitionBar";

			QGraphicsView *view = eventReceivingViewport(scenePos);
			if ( !view ) {
				qDebug() << "pointerClick: no view is available";
				return;
			}
			QPointF clickedViewPos = view->mapFromScene( scenePos );

			QMouseEvent mpe(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, btnFlags, Qt::NoModifier);
			QMouseEvent mre(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, btnFlags, Qt::NoModifier);

//			if (_item) _item->grabMouse();
			if ( ! QApplication::sendEvent(view->viewport(), &mpe) ) {
				// Upon receiving mousePressEvent, the item will become mouseGrabber if the item reimplement mousePressEvent
				qDebug("PolygonArrow::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
			}
			if ( ! QApplication::sendEvent(view->viewport(), &mre) ) {
				qDebug("PolygonArrow::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
			}
//			if (_item) _item->ungrabMouse();
		}
	}
	*/

	QGraphicsView *view = eventReceivingViewport(scenePos);
	if ( !view ) {
		qDebug() << "pointerClick: no view is available";
		return;
	}
	QPointF clickedViewPos = view->mapFromScene( scenePos );

	QMouseEvent mpe(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, btnFlags, modifier);
	QMouseEvent mre(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, btnFlags, modifier);

	if ( ! QApplication::sendEvent(view->viewport(), &mpe) ) {
		// Upon receiving mousePressEvent, the item will become mouseGrabber if the item reimplement mousePressEvent
		qDebug("PolygonArrow::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
	}
	else {
//		qDebug() << "press delievered";
		if ( ! QApplication::sendEvent(view->viewport(), &mre) ) {
			qDebug("PolygonArrow::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
		}
		else {
//			qDebug() << "release delievered";
			// who should ungrabmouse() ??
			QGraphicsItem *mouseGrabberItem = view->scene()->mouseGrabberItem();
			if (mouseGrabberItem) mouseGrabberItem->ungrabMouse();
		}
	}
}



void PolygonArrow::pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags, Qt::KeyboardModifier modifier) {
    if (!app) {
        qDebug("PolygonArrow::%s() : There's no app widget under this pointer", __FUNCTION__);
        return;
    }

    // Generate mouse event directly from here
    QGraphicsView *gview = eventReceivingViewport(scenePos);

    if (gview) {
        QMouseEvent dblClickEvent(QEvent::MouseButtonDblClick, gview->mapFromScene(scenePos), btn, btnFlags, modifier);
		QMouseEvent release(QEvent::MouseButtonRelease, gview->mapFromScene(scenePos), btn, btnFlags, modifier);

        // sendEvent doesn't delete event object, so event should be created in stack space
        if ( ! QApplication::sendEvent(gview->viewport(), &dblClickEvent) ) {
            qDebug("PolygonArrow::%s() : sendEvent MouseButtonDblClick on (%.1f,%.1f) failed", __FUNCTION__, scenePos.x(), scenePos.y());
        }
		else {
//			qDebug() << "dblClick delievered";
			if ( ! QApplication::sendEvent(gview->viewport(), &release)) {
				qDebug("PolygonArrow::%s() : sendEvent releaseEvent failed", __FUNCTION__);
			}
			else {
//				qDebug() << "release delivered";

				//
				// If the item is still mouse grabber, further pressEvent won't be delievered to any item including this one
				//
				QGraphicsItem *mouseGrabberItem = gview->scene()->mouseGrabberItem();
				if (mouseGrabberItem) mouseGrabberItem->ungrabMouse();
//				qDebug() << "mouse grabber" << gview->scene()->mouseGrabberItem();

			}
		}
    }
    else {
        qDebug("PolygonArrow::%s() : there is no viewport on %.1f, %.1f", __FUNCTION__, scenePos.x(), scenePos.y());
    }
}



void PolygonArrow::pointerWheel(const QPointF &scenePos, int delta, Qt::KeyboardModifier modifier) {
    // Generate mouse event directly from here
    QGraphicsView *gview = eventReceivingViewport(scenePos);

    if (gview) {
//		qDebug() << "pointerWheel" << delta;
		int _delta = 0;
		if ( delta > 0 )  _delta = 120;
		else if (delta <0) _delta = -120;
        if ( ! QApplication::sendEvent(gview->viewport(), & QWheelEvent(gview->mapFromScene(scenePos), /*gview->mapToGlobal(scenePos.toPoint()),*/ _delta, Qt::NoButton, Qt::NoModifier)) ) {
            qDebug("PolygonArrow::%s() : send wheelEvent failed", __FUNCTION__);
        }
        else {
            //qDebug() << "PolygonArrow wheel" << gview->mapFromScene(scenePos);
        }
    }
}

bool PolygonArrow::setAppUnderPointer(const QPointF scenePos) {
    Q_ASSERT(scene());
    QList<QGraphicsItem *> list = scene()->items(scenePos, Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
    //app = static_cast<BaseGraphicsWidget *>(scene()->itemAt(scenePosOfPointer, QTransform()));

    //qDebug() << "\nPolygonArrow::setAppUnderPointer() :" << list.size() << " items";
    foreach (QGraphicsItem *item, list) {
        if ( item == this ) continue;
        //qDebug() << item;

		if ( item->acceptedMouseButtons() == 0 ) continue;

        if ( item->type() >= QGraphicsItem::UserType + 12) {
			//
			// User application (BaseWidget)
			//
            app = static_cast<BaseWidget *>(item);
            //qDebug("PolygonArrow::%s() : uiclientid %llu, appid %llu", __FUNCTION__, uiclientid, app->globalAppId());
			_item = 0;
            return true;
        }
		else if (item->type() > QGraphicsItem::UserType) {
			//
			// custom type. So this custom type should be less than UserType + 12
			//
		}
		else {
			//
			// regualar graphics item (PixmapButton, PixmapCloseButtonOn
			//
			_item = item;
//			qDebug() << _item;
			app = 0;
			return true;
		}
    }
    app = 0; // reset
	_item = 0;

    //qDebug("PolygonArrow::%s() : uiclientid %llu, There's BaseGraphicsWidget type under pointer", __FUNCTION__, uiclientid);
    return false;
}

QGraphicsView * PolygonArrow::eventReceivingViewport(const QPointF scenePos) {
	Q_ASSERT(scene());
    foreach(QGraphicsView *v, scene()->views()) {
		if (!v) continue;

        // geometry of widget relative to its parent
//        v->geometry();

        // internal geometry of widget
//        v->rect();

        if ( v->rect().contains( v->mapFromScene(scenePos) ) ) {
            // mouse click position is within this view's bounding rectangle
			return v;
        }
    }
	return 0;
}







SwSimpleTextItem::SwSimpleTextItem(int ps, QGraphicsItem *parent)
	: QGraphicsSimpleTextItem(parent)
{
	if ( ps > 0 ) {
		QFont f;
		f.setPointSize(ps);
		setFont(f);
	}
//	setBrush(QColor(100, 100, 100, 128)); // will change brush for text
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

	painter->fillRect(boundingRect(), QBrush(QColor(128, 128, 128, 164)));
	QGraphicsSimpleTextItem::paint(painter, option, widget);
}










PixmapButton::PixmapButton(const QString &res, qreal desiredWidth, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
	QPixmap orgPixmap(res);
	if (desiredWidth) {
		orgPixmap = orgPixmap.scaledToWidth(desiredWidth);
	}
	QGraphicsPixmapItem *p = new QGraphicsPixmapItem(orgPixmap, this);

	// This widget (PixmapButton) has to receive mouse event
	p->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

	resize(p->pixmap().size());
	setOpacity(0.5);
}
PixmapButton::PixmapButton(const QPixmap &pixmap, qreal desiredWidth, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
	QGraphicsPixmapItem *p = 0;
	if ( desiredWidth ) {
		p = new QGraphicsPixmapItem(pixmap.scaledToWidth(desiredWidth), this);
	}
	else {
		p = new QGraphicsPixmapItem(pixmap, this);
	}

	// This widget (PixmapButton) has to receive mouse event
	p->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
	resize(p->pixmap().size());
	setOpacity(0.5);
}
PixmapButton::~PixmapButton() {

}

void PixmapButton::mousePressEvent(QGraphicsSceneMouseEvent *) {
	setOpacity(1);
}
void PixmapButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
	setOpacity(0.5);
//	qDebug() << "pixmapbutton emitting signal";
	emit clicked();
}






PixmapCloseButtonOnScene::PixmapCloseButtonOnScene(const QString res, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , flag(false)
{
	setPixmap(QPixmap(res));
	setAcceptedMouseButtons(Qt::LeftButton);
}

void PixmapCloseButtonOnScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(event);
	flag = true;
}

void PixmapCloseButtonOnScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (flag && boundingRect().contains(event->pos())) {
		SAGENextScene *s = static_cast<SAGENextScene *>(scene());
//		QGraphicsScene *s = scene();
		s->prepareClosing();
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



