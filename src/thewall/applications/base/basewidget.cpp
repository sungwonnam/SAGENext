#include "basewidget.h"
#include "appinfo.h"
#include "../../common/commonitem.h"
#include "perfmonitor.h"

//#include "../../system/resourcemonitor.h"
#include "affinityinfo.h"
//#include "../../system/affinitycontroldialog.h"

#include <QtGui>

BaseWidget::BaseWidget()
	: QGraphicsWidget()
	, _globalAppId(-1)
	, settings(0)
	, _windowState(BaseWidget::W_NORMAL)
	, _appInfo(new AppInfo())
	, showInfo(false)
	, _perfMon(new PerfMonitor(this))
	, _affInfo(0)
	, infoTextItem(0)
	, _lastTouch(0)
	, _rMonitor(0)
	, _quality(1.0)
	, _contextMenu(0)
	, _priority(0.5)
{
	init();
}

BaseWidget::BaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent /*0*/, Qt::WindowFlags wflags /*0*/)
	: QGraphicsWidget(parent, wflags)
	, _globalAppId(globalappid)
	, settings(s)
	, _windowState(BaseWidget::W_NORMAL)
	, _appInfo(new AppInfo())
	, showInfo(false)
	, _perfMon(new PerfMonitor(this))
	, _affInfo(0)
	, infoTextItem(0)
	, _lastTouch(0)
	, _rMonitor(0)
	, _quality(1.0)
	, _contextMenu(0)
	, _priority(0.5)
{
	init();
}

BaseWidget::~BaseWidget()
{
    if ( scene() ) {
        scene()->removeItem(this);
    }

    //qDebug("BaseGraphicsWidget::%s() : deleting _contextMenu", __FUNCTION__);
    //	qDeleteAll(actions());
    if(_contextMenu)
        delete _contextMenu;

    if ( _appInfo ) delete _appInfo;
    if ( _perfMon ) delete _perfMon;
    //	if ( infoTextItem ) delete infoTextItem;

    qDebug("BaseWidget::%s() : Removed widget %llu from scene.", __FUNCTION__, _globalAppId);
}


void BaseWidget::setWidgetType(Widget_Type wt) {
    _widgetType = wt;
    if (_perfMon) {
        _perfMon->setWidgetType(wt);
    }
}

void BaseWidget::setProxyWidget(QGraphicsProxyWidget *proxyWidget)
{
    if(proxyWidget) {

        QGraphicsLinearLayout *layout = new QGraphicsLinearLayout();

        proxyWidget->setParentItem(this);
        proxyWidget->setWindowFlags(Qt::FramelessWindowHint);
        proxyWidget->setFlag(QGraphicsItem::ItemIsMovable, false);
        proxyWidget->setFlag(QGraphicsItem::ItemIsSelectable, false);

        layout->addItem( proxyWidget );
        setLayout(layout);
    }
}

void BaseWidget::init()
{
        setAcceptHoverEvents(true);


        // Indicates that the widget paints all its pixels when it receives a paint event
        // Thus, it is not required for operations like updating, resizing, scrolling and focus changes
        // to erase the widget before generating paint events.
        setAttribute(Qt::WA_OpaquePaintEvent, true);

		// Destructor will be called by close()
        setAttribute(Qt::WA_DeleteOnClose, true);

        setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);

        /* When enabled, the item's paint() function will be called only once for each call to update(); for any subsequent repaint requests, the Graphics View framework will redraw from the cache. */
        /* Turn cache off for streaming application */
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);

        if (settings) {
            // This will affect boundingRect() of the widget
                setWindowFrameMargins(settings->value("general/framemarginleft", 3).toDouble(),
                                                  settings->value("general/framemargintop", 3).toDouble(),
                                                  settings->value("general/framemarginright",3).toDouble(),
                                                  settings->value("general/framemarginbottom",3).toDouble());
        }
//	getWindowFrameMargins(&frameMarginLeft, &frameMarginTop, &frameMarginRight, &frameMarginBottom);

        /*!
          Define QActions
          */
        createActions();

        /*!
          System provided context menu (mouse right click)
          */
        _contextMenu = new QMenu(tr("Menu"));
        _contextMenu->addActions( actions() );


        //! drawInfo() will show this item
        infoTextItem = new SwSimpleTextItem(0, this);
        infoTextItem->setFlag(QGraphicsItem::ItemIsMovable, false);
        infoTextItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
//	infoTextItem = new SwSimpleTextItem(settings->value("general/fontpointsize").toInt(), this);
        infoTextItem->hide();


        /*!
         Define animation
          */
        pAnim_pos = new QPropertyAnimation(this, "pos", this);
        pAnim_pos->setDuration(300); // 300 msec : ~16 paint events
        pAnim_pos->setEasingCurve(QEasingCurve::OutCubic);

        pAnim_scale = new QPropertyAnimation(this, "scale", this);
        pAnim_scale->setDuration(300);
        pAnim_scale->setEasingCurve(QEasingCurve::OutCubic);

        aGroup = new QParallelAnimationGroup();
        aGroup->addAnimation(pAnim_pos);
        aGroup->addAnimation(pAnim_scale);

        pAnim_opacity = new QPropertyAnimation(this, "opacity", this);
        pAnim_opacity->setEasingCurve(QEasingCurve::OutCubic);

        connect(pAnim_opacity, SIGNAL(finished()), this, SLOT(close()));


        /*!
          graphics effect
          */
//	shadow = new QGraphicsDropShadowEffect(this);
//	shadow->setBlurRadius(35);
//	shadow->setColor(Qt::gray);;
//	shadow->setEnabled(false);
//	setGraphicsEffect(shadow); // you can set only one effect
}


void BaseWidget::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (isSelected()) {

    }
    else {

    }

	// for now, let's use default implementation
	QGraphicsWidget::paintWindowFrame(painter, option, widget);
}

qreal BaseWidget::ratioToTheWall() const {
        if(!scene()) return 0.0;

        QSizeF sceneSize = scene()->sceneRect().size();
        QSizeF currentSize = size() * scale();

        qreal sceneArea = sceneSize.width() * sceneSize.height();
        qreal currentArea = currentSize.width() * currentSize.height();

        if ( currentArea > sceneArea ) return 1;

        return currentArea / sceneArea;
}

QRegion BaseWidget::effectiveVisibleRegion() const {
        QRegion effectiveRegion;
        if (!scene()) return effectiveRegion; // return empty region

        effectiveRegion = boundingRegion(sceneTransform()); // returns a region in scene coordinates

        QList<QGraphicsItem *> collidingItems = scene()->collidingItems(this);
        foreach (QGraphicsItem *i, collidingItems) {
                if ( i->zValue() <= zValue() ) continue;

                QRegion overlapped = i->boundingRegion(i->sceneTransform());

                effectiveRegion = effectiveRegion.subtracted( overlapped );
        }

//	_effectiveVisibleRegion = effectiveRegion;
        return effectiveRegion;
}


qreal BaseWidget::priority(qint64 ctepoch /* 0 */) {
        if (!scene()) return 0;

        /*******************
          size of Effective Visible Region
          *****************/
        qreal weight1 = 1.0;
        QRegion effectiveRegion = effectiveVisibleRegion();

        int effectiveVisibleSize  = 0;
        for (int i=0; i<effectiveRegion.rectCount(); ++i ) {
                QRect rect = effectiveRegion.rects().at(i);
                effectiveVisibleSize += (rect.size().width() * rect.size().height());
        }

        if (isObscured()) {
//		_priority = 0;
        }
        else {
//		_priority = ratioToTheWall()  * (1 + zValue()); // ratio is more important

                _priority = weight1  *  effectiveVisibleSize / (scene()->width() * scene()->height());
        }


        /****
          Time passed since last touch
          ****/
        qreal weight2 = 0.4;
        if ( ctepoch > 0 &&  _lastTouch > 0) {

                // _lastTouch is updated in mousePressEvent
                qreal T_lt = (qreal)(ctepoch - _lastTouch); // msec passed since last touch (current time in epoch - last touched time in epoch)
                T_lt /= 1000.0; // to second

//		qDebug() << "[[ Widget" << _globalAppId << weight2 << "/" << T_lt << "=" << weight2/T_lt;
                _priority += (weight2 / T_lt);
        }

//	qDebug() << "[[ Widget" << _globalAppId << "priority" << _priority << "]]";

        return _priority;
}



void BaseWidget::drawInfo()
{
        if (!showInfo) {
                showInfo = true;
                _showInfoAction->setDisabled(true);
                _hideInfoAction->setEnabled(true);
//		update();

                /* starts timer */
                timerID = startTimer(1000); // timerEvent every 1000 msec
        }
}

void BaseWidget::hideInfo()
{
        if (showInfo) {
                killTimer(timerID);
                showInfo = false;
                _hideInfoAction->setDisabled(true);
                _showInfoAction->setEnabled(true);
                update();
        }
}

void BaseWidget::minimize()
{
        if ( !_appInfo  ||  _windowState == W_MINIMIZED ) return;
        // icon size must be defined somewhere based on scene rectangle
        // QSetting pref dialog would be good point to start

        // based on this, I could use QGraphicsLayoutItem::setMinimumWidth/Height

        // before minimizing, widget's bounding rectangle must be saved.
        _appInfo->setRecentBoundingRect(mapRectToScene(boundingRect()));
        _appInfo->setRecentScale( scale() );
//	QPointF pos = mapToScene(boundingRect().topLeft());
//	qDebug("BaseGraphicsWidget::%s() : Pos (%.1f, %.1f) on the scene", __FUNCTION__, pos.x(), pos.y());

        /* minimize. No aspect ratio conserverd */
//	prepareGeometryChange();
//	resize(64, 64);

//	setScale( 128 / boundingRect().width() );

        _windowState = W_MINIMIZED;

        /* action status */
        _restoreAction->setEnabled(true);
        _minimizeAction->setDisabled(true);


        /* animation */
        if (pAnim_pos && pAnim_scale && aGroup ) {
                pAnim_pos->setStartValue(pos());
                pAnim_pos->setEndValue(QPointF(20,20));

                pAnim_scale->setStartValue(scale());
                pAnim_scale->setEndValue(0.2);

                aGroup->start();
        }
}

void BaseWidget::maximize()
{
	//	qDebug() << "BaseWidget::maximize()";
	if ( _windowState == W_MAXIMIZED ) {
		restore();
		return;
	}

	_maximizeAction->setEnabled(false);
	_restoreAction->setEnabled(true);

	// record current position and scale
	Q_ASSERT(_appInfo);
	_appInfo->setRecentBoundingRect(mapRectToScene(boundingRect()));
	_appInfo->setRecentScale( scale() );

	QSizeF s = size(); // current size of the widget. scaling won't change the size of the widget
	qreal scaleFactorW = scene()->width() / s.width();
	qreal scaleFactorH = scene()->height() / s.height();
	qreal scaleFactor = 1.0;
	(scaleFactorW < scaleFactorH) ? scaleFactor = scaleFactorW : scaleFactor = scaleFactorH;

	_windowState = W_MAXIMIZED;

	if (pAnim_pos && pAnim_scale && aGroup ) {
		pAnim_scale->setStartValue(scale());
		pAnim_scale->setEndValue(scaleFactor);

		pAnim_pos->setStartValue(pos());

		// this is based on top left as the transformation origin
//		pAnim_pos->setEndValue( QPointF( scene()->width() / 2  -  (s.width() * scaleFactor) / 2 , 0) );

		// this is based on center as the transformation origin
		pAnim_pos->setEndValue( QPointF(scene()->width() / 2 , scene()->height() / 2) );

		aGroup->start();
	}
}

void BaseWidget::restore()
{
	//        qDebug() << "BaseWidget::restore()";
	if ( _windowState == W_NORMAL ) return;

	Q_ASSERT(_appInfo);
	QRectF rect = _appInfo->getRecentBoundingRect();
	//	setPos(rect.topLeft());
	//	setScale(appInfo->getRecentScale());

	//	prepareGeometryChange();
	//	setPos(rect.x(), rect.y());
	//	setCurrentSize(rect.size());
	//setGeometry(rect);

	_windowState = W_NORMAL;

	/* action status */
	_restoreAction->setDisabled(true);
	_minimizeAction->setEnabled(true);
	_maximizeAction->setEnabled(true);

	if (pAnim_pos && pAnim_scale && aGroup ) {
		pAnim_pos->setStartValue(pos());
//		pAnim_pos->setEndValue(rect.topLeft());
		pAnim_pos->setEndValue(rect.center());

		pAnim_scale->setStartValue(scale());
		pAnim_scale->setEndValue(_appInfo->getRecentScale());

		aGroup->start();
	}
}

void BaseWidget::fadeOutClose()
{
//    qDebug() << "BaseWidget::fadeOutClose()";
    //	if (perfMon) {
    //		delete perfMon;
    //		perfMon = 0;
    //	}

    //	delete _contextMenu;
    //	_contextMenu = 0;

    // disable any QGraphicsEffect on this item to speed up paint()
    if (graphicsEffect()) graphicsEffect()->setEnabled(false);

    if (pAnim_opacity) {
        pAnim_opacity->setDuration(400);
        pAnim_opacity->setStartValue(0.9);
        pAnim_opacity->setEndValue(0.0);
        pAnim_opacity->start(); // when finishes, close() will be called
    }
    else {
        close();
    }
}

void BaseWidget::setTopmost()
{
        if(!scene()) return;

        qreal tempZ = -10000.0;

        QList<QGraphicsItem *> list = scene()->items(); // all items in the scene
        QGraphicsItem *item = 0;

        for ( int i=0; i<list.size(); i++ ) {
                item = list.at(i);
//		qDebug() << typeid(*item).name() << ". And this is " << typeid(*this).name();

                // only consider real app -> excluding buttons pointers, and so on
                if ( !item ||  item->type() != UserType + 2 ) continue;

                if ( item == this ) continue;

                if ( tempZ <= item->zValue() ) {
                        tempZ = item->zValue();
                }
        }

        // default z value is 0

        if (tempZ >= zValue())
                setZValue(tempZ + 0.0001);


        /**
        setZValue(1);
        if (!scene()) return;

//	QList<QGraphicsItem *> list = scene()->collidingItems(this);
        QList<QGraphicsItem *> list = scene()->items(); // all items in the scene
        QGraphicsItem *item = 0;
        for ( int i=0; i<list.size(); i++ ) {
                item = list.at(i);
//		qDebug() << typeid(*item).name() << ". And this is " << typeid(*this).name();

                // only consider real app -> excluding buttons pointers, and so on
                if ( !item || item == this || item->type() != UserType + 2 ) continue;

                BaseWidget *bw = static_cast<BaseWidget *>(item);
                bw->setZValue( bw->zValue() - 0.1 );
                if ( bw->zValue() <= 0 ) {
                        bw->setZValue(0.09);
                }
        }
        **/
}



void BaseWidget::reScale(int tick, qreal factor)
{
        qreal currentScale = scale();
        currentScale += ((qreal)tick * factor);

        // Note : Item transformations accumulate from parent to child, so if both a parent and child item are rotated 90 degrees, the child's total transformation will be 180 degrees. Similarly, if the item's parent is scaled to 2x its original size, its children will also be twice as large. An item's transformation does not affect its own local geometry; all geometry functions (e.g., contains(), update(), and all the mapping functions) still operate in local coordinates.
        setScale(currentScale);

        //! optional
//	appInfo->setRecentScale(currentScale);


        // This function will not change widget's size !!
//        qDebug() << "size: " << size() << "boundingRect" << boundingRect() << "geometry" << geometry();
}

QRectF BaseWidget::resizeHandleSceneRect()
{
        QSizeF size(100, 100);
        QPointF pos( boundingRect().width() - size.width(), boundingRect().height() - size.height() );
        return mapRectToScene(QRectF(pos, size));
}



void BaseWidget::updateInfoTextItem()
{
        if (!infoTextItem) return;

        QByteArray infotext(256, '\0');
        QByteArray perftext(512, '\0');
        QByteArray afftext(256, '\0');
        QString text("");

        //! let's show scene geometry of this widget
        QRectF rect = mapRectToScene(boundingRect());

        if (_appInfo) {
                sprintf(infotext.data(), "Global App ID %llu\nNative WxHxbpp: %ux%ux%u (%u KB)\nCurr Geometry: (%.0f,%.0f)(%.0fx%.0f)\nRatio to wall %.3f",
                                /*qPrintable(appInfo->getFilename()),*/
                                /* SizeRatio: %u %%\n */
                                _globalAppId,
                                _appInfo->nativeSize().width() ,_appInfo->nativeSize().height(), _appInfo->getBitPerPixel(), _appInfo->getFrameBytecount() / 1024, // to KiloByte
                                rect.topLeft().x(), rect.topLeft().y(),
                                rect.width() * scale(), rect.height() * scale(),
//				appInfo->ratioToTheScene( scene()->width() * scene()->height() ),
                                /*_appInfo->getDrawingThreadCpu()*/
                                ratioToTheWall()
                                );
                text.append( infotext );
        }

        if ( _affInfo ) {
                sprintf(afftext.data(), "\nrecv Thread\nRunning on CPU %d\nNUMA Node affinity %s\nMemory bind %s\nCPU affinity %s\n",
                                _affInfo->cpuOfMine(), _affInfo->getNodeMask(), _affInfo->getMemMask(), _affInfo->getCpuMask()
                                );
                text.append(afftext);
        }

        if ( _perfMon ) {
                sprintf(perftext.data(), "\nCurr/Avg Recv Lat : %.2f / %.2f ms\nCurr/Avg Conv Lat : %.2f / %.2f ms\nCurr/Avg Draw Lat : %.2f / %.2f ms\n\nCurr/Avg Recv FPS : %.2f / %.2f\nCurr/Avg recvFps AbsDevi. %.2f / %.2f\nRecv FPS variance %.4f\nCurr/Avg Disp FPS : %.2f / %.2f\n\nRecv/Draw Count %llu/%llu\nCurr Bandwidth : %.3f Mbps\nCPU usage : %.6f\nVol/InVol Ctx Sw : %ld / %ld" /*maxrss %ld, pageReclaims %ld"*/,
                                _perfMon->getCurrRecvLatency() * 1000.0 , _perfMon->getAvgRecvLatency() * 1000.0,
                                _perfMon->getCurrConvDelay() * 1000.0 , _perfMon->getAvgConvDelay() * 1000.0,
                                /*_perfMon->getCurrEqDelay() * 1000.0, _perfMon->getAvgEqDelay() * 1000.0,*/
                                _perfMon->getCurrDrawLatency() * 1000.0 , _perfMon->getAvgDrawLatency() * 1000.0,

                                _perfMon->getCurrRecvFps(), _perfMon->getAvgRecvFps(),
                                _perfMon->getCurrAbsDeviation(), _perfMon->getAvgAbsDeviation(),
                                _perfMon->getRecvFpsVariance(),
                                _perfMon->getCurrDispFps(), _perfMon->getAvgDispFps(),

                                _perfMon->getRecvCount(), _perfMon->getDrawCount(),
                                _perfMon->getCurrBandwidthMbps(),

                                _perfMon->getCpuUsage() * 100.0, _perfMon->getEndNvcsw(), _perfMon->getEndNivcsw()
                                /*perfMon->getEndMaxrss(), perfMon->getEndMinflt()*/
                                );
                text.append(perftext);


//		qDebug() << "BaseWidget::" << __FUNCTION__ <<  _globalAppId << _perfMon->ts_nextframe() *1000.0 << _perfMon->deadline_miseed() * 1000.0 << "ms";
        }

        if (infoTextItem) {
                infoTextItem->setText(QString(text));
                infoTextItem->update();
        }
}

void BaseWidget::createActions()
{
        _showInfoAction = new QAction("Show Info", this);
        _hideInfoAction = new QAction("Hide Info", this);

        if ( showInfo ) {
                _showInfoAction->setDisabled(true);
        }
        else {
                _hideInfoAction->setDisabled(true);
        }

        /* this is enabled only when affInfo is not null */
//	_affinityControlAction = new QAction("Affinity Control", this);
//	_affinityControlAction->setDisabled(true);

        _minimizeAction = new QAction("Minimize", this);
        _maximizeAction = new QAction("Maximize", this);
        _restoreAction = new QAction("Restore", this);
        _restoreAction->setDisabled(true);
        _closeAction = new QAction("Close", this);
//	_closeAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_W) );

        addAction(_showInfoAction);
        addAction(_hideInfoAction);
//	addAction(_affinityControlAction);
        addAction(_minimizeAction);
        addAction(_maximizeAction);
        addAction(_restoreAction);
        addAction(_closeAction);

        connect(_showInfoAction, SIGNAL(triggered()), this, SLOT(drawInfo()));
        connect(_hideInfoAction, SIGNAL(triggered()), this, SLOT(hideInfo()));
//	connect(_affinityControlAction, SIGNAL(triggered()), this, SLOT(showAffinityControlDialog()));
        connect(_minimizeAction, SIGNAL(triggered()), this, SLOT(minimize()));
        connect(_maximizeAction, SIGNAL(triggered()), this, SLOT(maximize()));
        connect(_restoreAction, SIGNAL(triggered()), this, SLOT(restore()));
        connect(_closeAction, SIGNAL(triggered()), this, SLOT(fadeOutClose()));
//        connect(_closeAction, SIGNAL(triggered()), this, SLOT(close()));
}






void BaseWidget::mouseClick(const QPointF &clickedScenePos, Qt::MouseButton btn) {

    QPointF itempos = mapFromScene(clickedScenePos);

    QGraphicsView *gview = 0;
    foreach (gview, scene()->views()) {
		// geometry of widget relative to its parent
		//        v->geometry();

		// internal geometry of widget
		//        v->rect();
        if ( gview->rect().contains(gview->mapFromScene(clickedScenePos)) )
            break;
    }

    if (gview) {

        // be aware that scaling doesn't change size(), boundingRect(), and geometry() at all
        if ( boundingRect().contains(itempos) ) {

            QMouseEvent *press = new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);
            QMouseEvent *release = new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);

            //grabMouse(); // don't do this before mousePress
            // sendEvent doesn't delete event object, so event can be created in stack (local to this function)
            if ( ! QApplication::sendEvent(gview->viewport(), press) ) {
                qDebug("BaseWidget::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
            }

            // this widget is now mouseGrabber because of the pressEvent

            if ( ! QApplication::sendEvent(gview->viewport(), release) ) {
                qDebug("BaseWidget::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
            }
            ungrabMouse();

            if (press) delete press;
            if (release) delete release;
        }
    }
    else {
        qDebug() << "BaseWidget::mouseClick() : couldn't find viewport contains scene position" << clickedScenePos;
    }
}

void BaseWidget::mouseDrag(const QPointF &, Qt::MouseButton) {

}






/*! reimplementing events */

void BaseWidget::timerEvent(QTimerEvent *) {
//	qDebug("BaseWidget::%s()", __FUNCTION__);
        updateInfoTextItem();

        /*
        if ( affControl && affControl->isVisible() ) {
                affControl->updateInfo();
        }
        */
}

void BaseWidget::resizeEvent(QGraphicsSceneResizeEvent *) {
	if(_appInfo) {
//		appInfo->setRecentBoundingRect( boundingRect() );
	}
}

void BaseWidget::setLastTouch() {
        _lastTouch = QDateTime::currentMSecsSinceEpoch();
}


void BaseWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {

}

void BaseWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {

}

/*!
  I'm reimplementing this so event will by default be accepted and this is the mousegrabber
  */
void BaseWidget::mousePressEvent(QGraphicsSceneMouseEvent *event) {
//	qDebug() << "BaseWidget::mousePressEvent() : pos:" << event->pos() << " ,scenePos:" << event->scenePos() << " ,screenPos:" << event->screenPos();

        if ( event->buttons() & Qt::LeftButton) {

                // refresh lastTouch
                _lastTouch = QDateTime::currentMSecsSinceEpoch();

                // change zvalue
                setTopmost();
        }
        // keep the base implementation
        // The event is QEvent::ignore() for items that are neither movable nor selectable.
        QGraphicsWidget::mousePressEvent(event);
}

//void BaseWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
////	qDebug() << "BGW::mouseMoveEvent" << e->pos() << e->scenePos() << e->screenPos() << e->button() << e->buttons();
//        QGraphicsWidget::mouseMoveEvent(e);
//}

void BaseWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
//	Q_UNUSED(event);

//        qDebug() << "doubleClickEvent" << event->lastPos() << event->pos() << ", " << event->lastScenePos() << event->scenePos() << ", " << event->lastScreenPos() << event->screenPos();

        if ( mapRectToScene(boundingRect()).contains(event->scenePos()) )
                maximize();
}

//void BaseWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
//	if ( event->buttons() & Qt::LeftButton ) {
//		mapToScene(event->pos());
//	}
//}

void BaseWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	//	if ( affInfo && _affinityControlAction ) {
	//		_affinityControlAction->setEnabled(true);
	//	}
	scene()->clearSelection();
	setSelected(true);
	_contextMenu->exec(event->screenPos());
	//_contextMenu->popup(event->scenePos());
}

void BaseWidget::wheelEvent(QGraphicsSceneWheelEvent *event) {
        // positive delta : rotated forward away from user
        int numDegrees = event->delta() / 8;
        int numTicks = numDegrees / 15;
//	qreal scaleFactor = 0.03; // 3 %
//	qDebug("BaseWidget::%s() : delta %d numDegrees %d numTicks %d", __FUNCTION__, event->delta(), numDegrees, numTicks);
//	qDebug() << "BGW wheel event" << event->pos() << event->scenePos() << event->screenPos() << event->buttons() << event->modifiers();

        reScale(numTicks, 0.03);
}



