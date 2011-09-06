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

{
        QPolygonF p;
        p << QPointF(0,0) << QPointF(60, 20) << QPointF(46, 34) << QPointF(71, 59) << QPointF(60, 70) << QPointF(35, 45) << QPointF(20, 60) << QPointF(0,0);

        setBrush(QBrush(c));
        setPolygon(p);

        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setZValue(9999999); // the top most
}

void PolygonArrow::pointerMove(const QPointF &_scenePos, Qt::MouseButtons btnFlags) {
    qreal deltax = _scenePos.x() - scenePos().x();
    qreal deltay = _scenePos.y() - scenePos().y();

    // move pointer itself
    // Sets the position of the item to pos, which is in parent coordinates. For items with no parent, pos is in scene coordinates.
    setPos(_scenePos);

    // if dragging
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
}

void PolygonArrow::pointerPress(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {
    if (!setAppUnderPointer(scenePos)) {
        qDebug() << "pointerPress() : setAppUnderPointer failed";
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



void PolygonArrow::pointerClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {
        // should be sent to interactive item like gwebview

    QGraphicsView *view = 0;
    foreach(QGraphicsView *v, scene()->views()) {

        // geometry of widget relative to its parent
//        v->geometry();

        // internal geometry of widget
//        v->rect();

        if ( v->rect().contains( v->mapFromScene(scenePos) ) ) {
            // mouse click position is within this view's bounding rectangle
            view = v;
            break;
        }

    }
    QPointF clickedViewPos = view->mapFromScene( scenePos );

    QMouseEvent mpe(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, btnFlags, Qt::NoModifier);
    QMouseEvent mre(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, btnFlags, Qt::NoModifier);

    /**
      Pointer can't know (in here) which widget is going to receive this event
      because it depends on how child widgets are attached to the parent widget.
      FOr example, gwebview of WebWidget will receive mouse event and gwebview isn't pointed by app.
      **/
    //	app->grabMouse();

    if ( ! QApplication::sendEvent(view->viewport(), & mpe) ) {
        qDebug("PolygonArrow::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
    }
    if ( ! QApplication::sendEvent(view->viewport(), &mre) ) {
        qDebug("PolygonArrow::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
    }
//    app->ungrabMouse();




    /**
      Instead of generating mouse event,
      I can let each widget implement BaseWidget::mouseClick()
      **/
    /*
        Q_UNUSED(btnFlags);
        if (app) {
                // Reimplement this for your app's specific needs
                app->mouseClick(scenePos, btn);
        }
        */
}



void PolygonArrow::pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::MouseButtons btnFlags) {
    if (!app) {
        qDebug("PolygonArrow::%s() : There's no app widget under this pointer", __FUNCTION__);
        return;
    }

    // Or directly control the widget
//    app->maximize();


    // Generate mouse event directly from here
    QGraphicsView *gview = 0;
    Q_ASSERT(scene());
    foreach(QGraphicsView *v, scene()->views()) {
        if ( v->rect().contains(v->mapFromScene(scenePos)) ) {
            gview = v;
            break;
        }
    }

    if (gview) {
        QMouseEvent dblClickEvent(QEvent::MouseButtonDblClick, gview->mapFromScene(scenePos), btn, btnFlags, Qt::NoModifier);

        // sendEvent doesn't delete event object, so event should be created in stack space
        if ( ! QApplication::sendEvent(gview->viewport(), &dblClickEvent) ) {
            qDebug("PolygonArrow::%s() : sendEvent MouseMuttonDblClick on %d,%d failed", __FUNCTION__, scenePos.x(), scenePos.y());
        }
    }
    else {
        qDebug("PolygonArrow::%s() : there is no viewport on %d, %d", __FUNCTION__, scenePos.x(), scenePos.y());
    }
}



void PolygonArrow::pointerWheel(const QPointF &scenePos, int delta) {
    // Generate mouse event directly from here
    QGraphicsView *gview = 0;
    Q_ASSERT(scene());
    foreach(QGraphicsView *v, scene()->views()) {
        if ( v->rect().contains( v->mapFromScene(scenePos)) ) {
            gview = v;
            break;
        }
    }
    if (gview) {
        if ( ! QApplication::sendEvent(gview->viewport(), & QWheelEvent(gview->mapFromScene(scenePos), gview->mapToGlobal(scenePos.toPoint()), delta, Qt::NoButton, Qt::NoModifier)) ) {
            qDebug("PolygonArrow::%s() : send wheelEvent failed", __FUNCTION__);
        }
        else {
            //qDebug() << "PolygonArrow wheel" << gview->mapFromScene(scenePos);
        }
    }
}

PolygonArrow::~PolygonArrow() {
    if(scene())
        scene()->removeItem(this);
}

void PolygonArrow::setPointerName(const QString &text) {
        textItem = new QGraphicsSimpleTextItem(text, this);
        textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
        textItem->setFlag(QGraphicsItem::ItemIsMovable, false);
        textItem->setBrush(Qt::white);
        textItem->moveBy(0, boundingRect().height());
}

bool PolygonArrow::setAppUnderPointer(const QPointF scenePos) {
    Q_ASSERT(scene());
    QList<QGraphicsItem *> list = scene()->items(scenePos, Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
    //app = static_cast<BaseGraphicsWidget *>(scene()->itemAt(scenePosOfPointer, QTransform()));

    //qDebug() << "\nPolygonArrow::setAppUnderPointer() :" << list.size() << " items";
    foreach (QGraphicsItem *item, list) {
        if ( item == this ) continue;
        //qDebug() << item;

        if ( item->type() == UserType + 2) {
            app = static_cast<BaseWidget *>(item);
            //app->setTopmost();
            //qDebug("PolygonArrow::%s() : uiclientid %llu, appid %llu", __FUNCTION__, uiclientid, app->globalAppId());
            return true;
        }
    }
    app = 0; // reset

    //qDebug("PolygonArrow::%s() : uiclientid %llu, There's BaseGraphicsWidget type under pointer", __FUNCTION__, uiclientid);
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
//		SAGENextScene *s = qobject_cast<SAGENextScene *>(scene());
                QGraphicsScene *s = scene();
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



