#include "basewidget.h"

#include "appinfo.h"
#include "perfmonitor.h"
#include "sn_priority.h"
#include "affinityinfo.h"

#include "../../common/commonitem.h"

//#include "../../system/resourcemonitor.h"

#include <QtGui>

#if QT_VERSION < 0x040700
#include <sys/time.h>
#endif

SN_BaseWidget::SN_BaseWidget(Qt::WindowFlags wflags)
	: QGraphicsWidget(0, wflags)
    , _windowState(SN_BaseWidget::W_NORMAL)
    , _isSelectedByPointer(false)
    , _useOpenGL(true)
	, _globalAppId(0)
	, _settings(0)

    , _widgetType(SN_BaseWidget::Widget_Misc)

    , infoTextItem(0)
	, _appInfo(new AppInfo(0))
    , _showInfo(false)
    , _priorityData(0)
	, _perfMon(new PerfMonitor(this))
	, _affInfo(0)
	, _rMonitor(0)
	, _quality(1.0)
	, _contextMenu(0)

    , _isMoving(false)
    , _isResizing(false)
    , _resizeRectangle(new QGraphicsRectItem(this))

    , _showInfoAction(0)
    , _hideInfoAction(0)
    , _minimizeAction(0)
    , _restoreAction(0)
    , _maximizeAction(0)
    , _closeAction(0)

    , pAnim_pos(0)
    , pAnim_scale(0)
    , pAnim_size(0)
    , _parallelAnimGroup(0)
    , pAnim_opacity(0)

//    , _timerID(0)
	, _registerForMouseHover(false)
    , _bordersize(10)

    , _isSchedulable(false)

{
	init();
}

SN_BaseWidget::SN_BaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent /*0*/, Qt::WindowFlags wflags /*0*/)
	: QGraphicsWidget(parent, wflags)
    , _windowState(SN_BaseWidget::W_NORMAL)
    , _isSelectedByPointer(false)
    , _useOpenGL(true)
	, _globalAppId(globalappid)
	, _settings(s)
    , _widgetType(SN_BaseWidget::Widget_Misc)

    , infoTextItem(0)
	, _appInfo(new AppInfo(globalappid))
    , _showInfo(false)
    , _priorityData(0)
	, _perfMon(new PerfMonitor(this))
	, _affInfo(0)
	, _rMonitor(0)
	, _quality(1.0)
	, _contextMenu(0)

    , _isMoving(false)
    , _isResizing(false)
    , _resizeRectangle(new QGraphicsRectItem(this))

    , _showInfoAction(0)
    , _hideInfoAction(0)
    , _minimizeAction(0)
    , _restoreAction(0)
    , _maximizeAction(0)
    , _closeAction(0)

    , pAnim_pos(0)
    , pAnim_scale(0)
    , pAnim_size(0)
    , _parallelAnimGroup(0)
    , pAnim_opacity(0)

//    , _timerID(0)
	, _registerForMouseHover(false)
    , _bordersize(10)

    , _isSchedulable(false)

{
	setSettings(s);

	init();
}

SN_BaseWidget::~SN_BaseWidget()
{
//    if (_isSchedulable) {
//        if (_rMonitor) {
//            _rMonitor->removeSchedulableWidget(this);
//        }
//    }


    if ( scene() ) {
        scene()->removeItem(this);

//		if ( _registerForMouseHover ) {
//			SAGENextScene *sc = static_cast<SAGENextScene *>(scene());
//			sc->hoverAcceptingApps.removeAll(this);
//		}
    }

    //qDebug("BaseGraphicsWidget::%s() : deleting _contextMenu", __FUNCTION__);
    //	qDeleteAll(actions());
    if(_contextMenu)
        delete _contextMenu;

    if ( _appInfo ) delete _appInfo;
	if ( _perfMon ) {
		if (_settings->value("misc/printperfdataattheend",false).toBool()) {
			_perfMon->printData();
		}
		delete _perfMon;
	}
    //	if ( infoTextItem ) delete infoTextItem;

//	if (_priorityData) delete _priorityData;

	qDebug("%s::%s() : widget id %llu has been deleted\n",metaObject()->className(), __FUNCTION__, _globalAppId);
}


void SN_BaseWidget::setGlobalAppId(quint64 gaid) {
    _globalAppId = gaid;
    if (_appInfo) {
        _appInfo->setGID(gaid);
    }
}


void SN_BaseWidget::init()
{
	//
	// Destructor will be called by close()
	//
	setAttribute(Qt::WA_DeleteOnClose, true);

	setAttribute(Qt::WA_NoSystemBackground, true);

	//
	// Indicates that the widget paints all its pixels when it receives a paint event
	// Thus, it is not required for operations like updating, resizing, scrolling and focus changes
	// to erase the widget before generating paint events.
	//
	setAttribute(Qt::WA_OpaquePaintEvent, true);

	//
	// To be effective when true, unset WA_OpaquePaintEvent
	//
	setAutoFillBackground(false);


	setFlag(QGraphicsItem::ItemIsMovable);

	/**
	  When basewidget is child of SN_LayoutWidget,
	  */
	setFlag(QGraphicsItem::ItemIgnoresTransformations, true);


	//qDebug() << "BaseWidget::init() : boundingRect" << boundingRect() << "windowFrameRect" << windowFrameRect();
	// window frame is not interactible by shared pointers
	setWindowFrameMargins(0, 0, 0, 0);

	/*!
	  Define QActions
	  */
	createActions();

	/*!
	  System provided context menu (console mouse right click)
	  This doesn't apply to shared pointers
	  */
	_contextMenu = new QMenu(tr("Menu"));
	_contextMenu->addActions( actions() );


	//! drawInfo() will show this item
	// Note that infoTextItem is child item of BaseWidget
	infoTextItem = new SN_SimpleTextItem(0, QColor(Qt::black), QColor(128, 128, 128, 164), this);
	infoTextItem->setFlag(QGraphicsItem::ItemIsMovable, false);
	infoTextItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
	//infoTextItem = new SwSimpleTextItem(settings->value("general/fontpointsize").toInt(), this);
	infoTextItem->hide();


//    _resizeRectangle->setFlag(QGraphicsItem::ItemStacksBehindParent);
    _resizeRectangle->hide();
    _resizeRectangle->setFlag(QGraphicsItem::ItemIsMovable, false);
    _resizeRectangle->setFlag(QGraphicsItem::ItemIsSelectable, false);
    _resizeRectangle->setAcceptedMouseButtons(0);
    _resizeRectangle->setPen(QPen(QBrush(Qt::white), 10, Qt::DotLine));
//    _resizeRectangle->setBrush(Qt::white);



	//
	// temporary to display info overlay by default
	//
//	_showInfoAction->trigger();



	/*!
	  Define animation
	  */
	/****************
	pAnim_pos = new QPropertyAnimation(this, "pos", this);
	pAnim_pos->setDuration(300); // 300 msec : ~16 paint events
	pAnim_pos->setEasingCurve(QEasingCurve::OutCubic);

	pAnim_scale = new QPropertyAnimation(this, "scale", this);
	pAnim_scale->setDuration(300);
	pAnim_scale->setEasingCurve(QEasingCurve::OutCubic);

	pAnim_size = new QPropertyAnimation(this, "size", this);
	pAnim_size->setDuration(250);
	pAnim_size->setEasingCurve(QEasingCurve::OutCubic);

	_parallelAnimGroup = new QParallelAnimationGroup(this);
	_parallelAnimGroup->addAnimation(pAnim_pos);
	if (isWindow())
		_parallelAnimGroup->addAnimation(pAnim_size);
	else
		_parallelAnimGroup->addAnimation(pAnim_scale);
		************/

	//
	// Opacity effect when closing
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


void SN_BaseWidget::setSettings(const QSettings *s) {
    _settings = s;

    // This will affect boundingRect(), windowFrameRect() of the widget.
	_bordersize = _settings->value("gui/framemargin", 8).toInt();
	if (isWindow()) {
		// Qt::Window might want to define mouse dragging. For that case, give more room to top margin
		setContentsMargins(_bordersize, _bordersize + 40, _bordersize, _bordersize); // by default, this is 0 0 0 0
	}
	else {
		// setting frameMargins won't have any effect.. (Qt::Widget doesn't have frame)
		// setting contentMargins won't have any effect unless this widget has a layout.
	}

	_useOpenGL = _settings->value("graphics/openglviewport").toBool();

	if (_settings->value("system/resourcemonitor").toBool()) {
		_priorityData = new SN_Priority(this);
	}
}

QRegion SN_BaseWidget::effectiveVisibleRegion() const {
	QRegion effectiveRegion;
	if (!scene()) return effectiveRegion; // return empty region

	effectiveRegion = boundingRegion(sceneTransform()); // returns the region in the scene coordinate

	QList<QGraphicsItem *> cItems = collidingItems(Qt::IntersectsItemBoundingRect);
	QList<QGraphicsItem *>::const_iterator it;
	for (it=cItems.constBegin(); it!=cItems.constEnd(); it++) {
		QGraphicsItem *i = (*it);
		Q_ASSERT(i);

		// consider only user application
		if ( i->type() < QGraphicsItem::UserType + BASEWIDGET_USER ) continue;

		SN_BaseWidget *other = static_cast<SN_BaseWidget *>(i);
		Q_ASSERT(other);

		// consider only those apps above this (thereby obstructing a portion of this)
		if ( i->zValue() < zValue() ) continue;

		// items are stacked by insertion order
		// if item B had added after A then B will be on top of A even though their z values are the same
		if (i->zValue() == zValue() ) {
			if (other->globalAppId() < _globalAppId) {
				// then other item were added before this
				// so I'm on top of other
				continue;
			}
		}

		// region is in scene coordinate
		effectiveRegion = effectiveRegion.subtracted( i->sceneBoundingRect().toRect() );
	}

	//	_effectiveVisibleRegion = effectiveRegion;
	return effectiveRegion;
}


/*!
  ctepoch : current time since epoch
  */
qreal SN_BaseWidget::priority(qint64 ctepoch /* 0 */) {
    Q_UNUSED(ctepoch);
//	Q_ASSERT(_priorityData);
	if (!_priorityData) return -1;

    //
    // The widget doesn't need resource.
    // This is useful only for non-periodic widgets (where there's no fixed Rq)
    // Usually, Rq is set to 0 when currBW is 0
    //
    if (_perfMon && _perfMon->getRequiredBW_Mbps() == 0) {
        //
        // no resource will be allocated by the scheduler
        //
        return 0.0;
    }

	return _priorityData->priority();
}

int SN_BaseWidget::priorityQuantized(qint64 currTimeEpoch, int bias /* 1 */) {
	return bias * priority(currTimeEpoch);
}


qreal SN_BaseWidget::observedQuality_Rq() const {
    if (_perfMon) {
        return _perfMon->observedQuality_Rq();
    }

    return -1;
}

qreal SN_BaseWidget::observedQuality_Dq() const {
    if (_perfMon) {
        return _perfMon->observedQuality_Dq();
    }
    return -1;
}



void SN_BaseWidget::drawInfo()
{
	if (!_showInfo) {
		_showInfo = true;
		_showInfoAction->setDisabled(true);
		_hideInfoAction->setEnabled(true);
		//update();
        infoTextItem->show();

		/* starts timer */
//		_timerID = startTimer(1000); // timerEvent every 1000 msec
	}
	else {
		hideInfo();
	}
}

void SN_BaseWidget::hideInfo()
{
	if (_showInfo) {
//		killTimer(_timerID);
		_showInfo = false;
		_hideInfoAction->setDisabled(true);
		_showInfoAction->setEnabled(true);
        infoTextItem->hide();
//		update();
	}
}

void SN_BaseWidget::minimize()
{
	if ( !_appInfo  ||  _windowState == W_MINIMIZED ) return;
	// icon size must be defined somewhere based on scene rectangle
	// QSetting pref dialog would be good point to start

	// based on this, I could use QGraphicsLayoutItem::setMinimumWidth/Height

	// before minimizing, widget's bounding rectangle must be saved.
//	_appInfo->setRecentPos(pos());
//	_appInfo->setRecentSize(size());
//	_appInfo->setRecentScale(scale());
	//	QPointF pos = mapToScene(boundingRect().topLeft());
	//	qDebug("BaseGraphicsWidget::%s() : Pos (%.1f, %.1f) on the scene", __FUNCTION__, pos.x(), pos.y());

	/* minimize. No aspect ratio conserverd */
	//	prepareGeometryChange();
	//	resize(64, 64);

	//	setScale( 128 / boundingRect().width() );


	/* action status */
	_restoreAction->setEnabled(true);
	_minimizeAction->setDisabled(true);


	/* animation */
	if (isWindow()) {
		if (pAnim_pos && pAnim_scale && _parallelAnimGroup ) {
			pAnim_pos->setStartValue(pos());
			pAnim_pos->setEndValue(QPointF(20,20));

			pAnim_size->setStartValue(size());
			pAnim_size->setEndValue(QSizeF(100,100));

			_parallelAnimGroup->start();
		}
		else {
			setPos(QPointF(20, 20)); // bottom of the scene. what if there are other widgets already minimized ???
			resize(100, 100);
		}
	}
	else {
		if (pAnim_pos && pAnim_scale && _parallelAnimGroup ) {
			pAnim_pos->setStartValue(pos());
			pAnim_pos->setEndValue(QPointF(20,20));

			pAnim_scale->setStartValue(scale());
			pAnim_scale->setEndValue(0.2);

			_parallelAnimGroup->start();
		}
		else {
			setPos(QPointF(20, 20)); // bottom of the scene. what if there are other widgets already minimized ???
			setScale(0.2); // must be some uniform size. So scale value should be differ by each widget
		}
	}
	_windowState = W_MINIMIZED;
}

void SN_BaseWidget::maximize()
{
	//	qDebug() << "BaseWidget::maximize()";
	if ( _windowState == W_MAXIMIZED ) {
		restore();
		return;
	}

	_maximizeAction->setEnabled(false);
	_restoreAction->setEnabled(true);


    //
    // widget shouldn't obstruct layout buttons (tile, horizontal, vertical,..) on the left side of the layout
    //
    int layoutButtonWidth = 0;
    if (parentWidget()) {
        layoutButtonWidth = _settings->value("gui/iconwidth").toInt();
    }


	if ( isWindow() ) {
		if (pAnim_pos && pAnim_size && _parallelAnimGroup) {
			/*
			pAnim_size->setStartValue(size());
			pAnim_size->setEndValue(scene()->sceneRect().size()  -  QSizeF(40, 60));

			pAnim_pos->setStartValue(pos()); // == mapToParent(0,0)
			pAnim_pos->setEndValue(scene()->sceneRect().topLeft() + QPointF(20, 40));

			_parallelAnimGroup->start();
			*/
		}
		else {
			if (parentWidget()) {
				resize(parentWidget()->size() - QSizeF(40 + layoutButtonWidth, 60));
				setPos(parentWidget()->boundingRect().topLeft() + QPointF(20,40));
			}
			else {
				resize(scene()->sceneRect().size()  -  QSizeF(40, 60));
				setPos(scene()->sceneRect().topLeft() + QPointF(20, 40));
			}
		}
	}
	else {
		//
		// Determine the maximized window size
		//

		QSizeF s = size(); // current size of the widget. scaling won't change the size of the widget

		qreal scaleFactorW = 0;
		qreal scaleFactorH = 0;

		if (parentWidget()) {
			// SN_LayoutWidget is the parent widget
			scaleFactorW = (parentWidget()->size().width() - layoutButtonWidth) / s.width();
			scaleFactorH = parentWidget()->size().height() / s.height();
		}
		else {
			// if there's no SN_LayoutWidget
			scaleFactorW = scene()->width() / s.width();
			scaleFactorH = scene()->height() / s.height();
		}

        //
        // How much should I scale up (or down) to fit in the SN_LayoutWidget or Scene
        //
		qreal scaleFactor = 1.0;
		(scaleFactorW < scaleFactorH) ? scaleFactor = scaleFactorW : scaleFactor = scaleFactorH;


		if (pAnim_pos && pAnim_scale && _parallelAnimGroup) {
			/*
			pAnim_scale->setStartValue(scale());
			pAnim_scale->setEndValue(scaleFactor);

			pAnim_pos->setStartValue(pos());
			pAnim_pos->setEndValue( QPointF((scene()->width() - s.width())/2 ,(scene()->height() - s.height())/2) );

			_parallelAnimGroup->start();
			*/
		}
		else {
			if (parentWidget()) { // could be the SN_LayoutWidget or 0
                //
                // My aspect ratio can be different from the SN_LayoutWidget's aspect ratio
                // So below determine offsets when placing my 0,0 position on the layout widget
                //
				qreal xoffset = (parentWidget()->size().width() - layoutButtonWidth - s.width() * scaleFactor)/2;
				qreal yoffset = (parentWidget()->size().height() - s.height() * scaleFactor)/2;

				setPos( parentWidget()->boundingRect().left() + xoffset , parentWidget()->boundingRect().top() + yoffset);
			}
			else {
				setPos(QPointF((scene()->width() - s.width())/2 ,(scene()->height() - s.height())/2));
			}
			setScale(scaleFactor);
		}
	}
	_windowState = W_MAXIMIZED;
}

void SN_BaseWidget::restore()
{
	// qDebug() << "BaseWidget::restore()";
	if ( _windowState == W_NORMAL ) return;

	/* action status */
	_restoreAction->setDisabled(true);
	_minimizeAction->setEnabled(true);
	_maximizeAction->setEnabled(true);

	Q_ASSERT(_appInfo);

	if ( isWindow() ) {
		if (pAnim_pos && pAnim_size && _parallelAnimGroup) {
			pAnim_pos->setStartValue(pos());
			pAnim_pos->setEndValue(_appInfo->recentPos());

			pAnim_size->setStartValue(size());
			pAnim_size->setEndValue(_appInfo->recentSize());

			_parallelAnimGroup->start();
		}
		else {
			setPos(_appInfo->recentPos());
			resize(_appInfo->recentSize());
		}
	}
	else {
		if (pAnim_pos && pAnim_scale && _parallelAnimGroup) {
			pAnim_pos->setStartValue(pos());
			pAnim_pos->setEndValue(_appInfo->recentPos());

			pAnim_scale->setStartValue(scale());
			pAnim_scale->setEndValue(_appInfo->recentScale());

			_parallelAnimGroup->start();
		}
		else {
			setPos(_appInfo->recentPos());
			setScale(_appInfo->recentScale());
		}
	}
	_windowState = W_NORMAL;
}

void SN_BaseWidget::fadeOutClose()
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

void SN_BaseWidget::setTopmost()
{
	if(!scene()) return;

	qreal tempZ = -10000.0;

	QList<QGraphicsItem *> list = scene()->items(); // all items in the scene
	QGraphicsItem *item = 0;

	for ( int i=0; i<list.size(); i++ ) {
		item = list.at(i);
		//qDebug() << typeid(*item).name() << ". And this is " << typeid(*this).name();

		// only consider real app -> excluding buttons pointers, and so on
		if ( !item ||  item->type() < UserType + BASEWIDGET_USER ) continue;

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


/*!
  rescaling won't change the boundingRectangle
  */
void SN_BaseWidget::reScale(int tick, qreal factor)
{
	qreal currentScale = scale();

	QSizeF currentVisibleSize = currentScale * size();
	qreal currentArea = currentVisibleSize.width() * currentVisibleSize.height();

	// The application window shouldn't be too small
	if ( tick < 0  &&  (currentScale <= 0.05 || currentArea <= 400)) return;

	qreal delta = (qreal)tick * factor;

	currentScale += delta;

	// Note : Item transformations accumulate from parent to child, so if both a parent and child item are rotated 90 degrees,
	//the child's total transformation will be 180 degrees.
	//Similarly, if the item's parent is scaled to 2x its original size, its children will also be twice as large.
	//An item's transformation does not affect its own local geometry;
	//all geometry functions (e.g., contains(), update(), and all the mapping functions) still operate in local coordinates.
	setScale(currentScale);

	/***********
	  Below is for when the transformOriginPoint() is default (0,0) but we want to make users feel like transform origin is center.
	  I don't use setTransformOriginPoint() because it's very problematic when an item is scaled
	  ****/
	qreal dx = (size().width() * -delta)/2.0;
	qreal dy = (size().height() * -delta)/2.0;
	moveBy(dx,dy);
//	QTransform t(currentScale, 0, 0, currentScale, dx, dy);
//	setTransform(t, true);

	//! optional
//	appInfo->setRecentScale(currentScale);

    //
    // Widget's size and boundingRect won't be changed !!
    // The size of the sceneBoundingRect will reflect the scale !!
    //
//    qDebug() <<"scale:" << scale() << "size" << size() << "boundingRect" << boundingRect() << "geometry" << geometry() << "sceneBoundingRect" << sceneBoundingRect();
}

QRectF SN_BaseWidget::resizeHandleRect() const
{
	QSizeF size(128, 128);

    if (boundingRect().width() <= size.width()) {
        size.rwidth() = boundingRect().width() / 2;
    }
    if (boundingRect().height() <= size.height()) {
        size.rheight() = boundingRect().height() / 2;
    }

    //
    // bottom right corner of the window
    //
	QPointF pos( boundingRect().right() - size.width(), boundingRect().bottom() - size.height());
//	return mapRectToScene(QRectF(pos, size));
	return QRectF(pos, size);
}



void SN_BaseWidget::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
	Q_UNUSED(pointer);
	Q_UNUSED(point);
	Q_UNUSED(btn);

	setTopmost();

    if (btn == Qt::LeftButton) {
		if (resizeHandleRect().contains(point)) {
            _isResizing = true;
            _isMoving = false;

            _resizeRectangle->setRect(boundingRect());
            _resizeRectangle->show();
        }
        else {
            _isResizing = false;
            _isMoving = true;
        }
        
        if (_windowState == SN_BaseWidget::W_NORMAL) {
            _appInfo->setRecentPos(scenePos());
            _appInfo->setRecentSize(size());
            _appInfo->setRecentScale(scale());
        }
    }
}

void SN_BaseWidget::handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
	Q_UNUSED(pointer);
	Q_UNUSED(point);
	Q_UNUSED(btn);

    _isMoving = false;

    if (_isResizing) {

        //
        // now resize
        //
        if (isWindow()) {
            resize( _resizeRectangle->rect().size() );
        }

        //
        // rescale
        //
        else {
            //qDebug() << _resizeRectangle->rect() << boundingRect() << size() << scale();
            qreal se = _resizeRectangle->rect().width() * scale() / boundingRect().width();
            setScale(se);
        }

    }
    _resizeRectangle->hide();
    _isResizing = false;

    // base implementation does nothing
}

void SN_BaseWidget::handlePointerDrag(SN_PolygonArrowPointer * pointer, const QPointF & point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton btn, Qt::KeyboardModifier) {
	Q_UNUSED(pointer);
    Q_UNUSED(point);

    if ( btn == Qt::LeftButton ) {

        //
        // move the window
        //
        if (_isMoving) {
            moveBy(pointerDeltaX, pointerDeltaY);
        }

        //
        // resize/rescale window
        //
        else if (_isResizing) {

            //
            // show and resize the _resizeRectangle
            //
            QRectF rect = _resizeRectangle->rect();
            if (isWindow()) {
                rect.adjust(0, 0, pointerDeltaX, pointerDeltaY);
            }
            else {
                qreal adjustment = 0.0;
                if (size().width() < size().height()) {
                    adjustment = pointerDeltaX / size().width();
                }
                else {
                    adjustment = pointerDeltaY / size().height();
                }
                rect.adjust(0, 0, rect.width() * adjustment, rect.height() * adjustment);
            }
            _resizeRectangle->setRect(rect);
        }


        else {
            // do nothing in the base implementation
        }
    }
}


void SN_BaseWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	/**
	  changing painter state will hurt performance
	  */
//	QPen pen;
//	pen.setWidth(6); // pen width is ignored when rendering
//	if (isSelected()) {
//		pen.setColor(QColor(170, 170, 5, 200));
//	}
//	else {
//		pen.setColor(QColor(100, 100, 100, 128));
//	}
//	painter->setPen(pen);
//	painter->drawRect(windowFrameRect());
	
	if (! isWindow()  &&  selectedByPointer() ) {
/*
		QLinearGradient lg(boundingRect().topLeft(), boundingRect().bottomLeft());
		if (isSelected()) {
			lg.setColorAt(0, QColor(250, 250, 0, 164));
			lg.setColorAt(1, QColor(200, 0, 0, 164));
		}
		else {
			lg.setColorAt(0, QColor(230, 230, 230, 164));
			lg.setColorAt(1, QColor(20, 20, 20, 164));
		}
		QBrush brush(lg);
*/


//		  Below draws rectangle around widget's content (using boundingRect)
//		  Because of this, it has to set Pen (which chagens painter state)
//		  And this can be called anytime because it's not filling the rectangle
		QPen pen;
		pen.setWidth( _bordersize );
        pen.setColor(QColor(170, 170, 5, 200));
//		pen.setBrush(brush);

		painter->setPen(pen);
		painter->drawRect(boundingRect());

//		  Below fills rectangle (works as background) so it has to be called BEFORE any drawing code
//		painter->fillRect(boundingRect(), brush);
	}


	/* info overlay */
	if ( _showInfo  &&  !infoTextItem->isVisible() ) {
#if defined(Q_OS_LINUX)
		_appInfo->setDrawingThreadCpu(sched_getcpu());
#endif
	}
}

void SN_BaseWidget::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

//    qDebug() << "size" << size() << "boundingrect" << boundingRect() << "geometry" << geometry();

    QLinearGradient lg(boundingRect().topLeft(), boundingRect().bottomLeft());
	if (selectedByPointer()) {
		lg.setColorAt(0, QColor(250, 250, 0, 164));
		lg.setColorAt(1, QColor(200, 0, 0, 164));
	}
	else {
		lg.setColorAt(0, QColor(230, 230, 230, 164));
		lg.setColorAt(1, QColor(20, 20, 20, 164));
	}
	painter->setBrush(QBrush(lg));
    painter->drawRect(windowFrameRect());

	// I'm not using base implementation
//    QGraphicsWidget::paintWindowFrame(painter, option, widget);
}



/**
  drawInfo() will start the timer
  */
void SN_BaseWidget::timerEvent(QTimerEvent *) {
//	qDebug("BaseWidget::%s()", __FUNCTION__);
//	if (e->timerId() == _timerID) {
//		updateInfoTextItem();
//	}
}


void SN_BaseWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
//	Q_UNUSED(event);
	//qDebug() << "doubleClickEvent" << event->lastPos() << event->pos() << ", " << event->lastScenePos() << event->scenePos() << ", " << event->lastScreenPos() << event->screenPos();
	if ( mapRectToScene(boundingRect()).contains(event->scenePos()) ) {
		maximize(); // window will be restored if it is in maximized state already
//		_priorityData->setLastInteraction(SN_Priority::RESIZE);
	}
}

void SN_BaseWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
//	_contextMenu->exec(event->screenPos());
    _contextMenu->popup(event->screenPos());
}

void SN_BaseWidget::wheelEvent(QGraphicsSceneWheelEvent *event) {
	// positive delta : rotated forward away from user
	int numDegrees = event->delta() / 8;
	int numTicks = numDegrees / 15;
	//	qreal scaleFactor = 0.03; // 3 %
	//	qDebug("BaseWidget::%s() : delta %d numDegrees %d numTicks %d", __FUNCTION__, event->delta(), numDegrees, numTicks);
	//	qDebug() << "BGW wheel event" << event->pos() << event->scenePos() << event->screenPos() << event->buttons() << event->modifiers();

	if (isWindow()) {
		// do nothing
	}
	else {
		reScale(numTicks, 0.05);
//		_priorityData->setLastInteraction(SN_Priority::RESIZE);
	}

	// reimplementation will accept the event
}



void SN_BaseWidget::updateInfoTextItem()
{
	if (!infoTextItem || !_showInfo) return;

	QByteArray infotext(256, '\0');
	QByteArray perftext(512, '\0');
	QByteArray afftext(256, '\0');
	QString text("");

	//! let's show scene geometry of this widget
	//QRectF rect = mapRectToScene(boundingRect());
	QRectF rect = geometry();

	if (_appInfo) {
		sprintf(infotext.data(), "Global App ID %llu\nNative WxHxbpp: %ux%ux%u (%u KB)\nCurr Geometry: (%.0f,%.0f)(%.0fx%.0f)\nCurr Scale: %.1f"
		        /*qPrintable(appInfo->getFilename()),*/
		        /* SizeRatio: %u %%\n */
		        , _globalAppId
		        , _appInfo->nativeSize().width() ,_appInfo->nativeSize().height(), _appInfo->bitPerPixel(), _appInfo->frameSizeInByte() / 1024 // to KiloByte
		        , rect.topLeft().x(), rect.topLeft().y()
		        , rect.width(), rect.height()
		        , scale()
		        //appInfo->ratioToTheScene( scene()->width() * scene()->height() ),
		        /*_appInfo->getDrawingThreadCpu()*/
		        );
		text.append( infotext );
	}

	if ( _affInfo ) {
		sprintf(afftext.data(), "\nrecv Thread\nRunning on CPU %d\nNUMA Node affinity %s\nMemory bind %s\nCPU affinity %s\n"
		        , _affInfo->cpuOfMine(), _affInfo->getNodeMask(), _affInfo->getMemMask(), _affInfo->getCpuMask()
		        );
		text.append(afftext);
	}

	if ( _perfMon ) {
		sprintf(perftext.data(), "\nCurr/Avg Recv Lat : %.2f / %.2f ms \n Curr/Avg Upd Lat : %.2f / %.2f ms \n Curr/Avg Draw Lat : %.2f / %.2f ms \n Curr/Avg Recv FPS : %.2f / %.2f \n Curr/Avg Disp FPS : %.2f / %.2f \n Recv/Draw Count %llu/%llu \n Curr/ReqBW %.3f / %.3f Mbps \n CPU %.6f" /*\nVol/InVol Ctx Sw : %ld / %ld*/ /*maxrss %ld, pageReclaims %ld"*/
		        ,_perfMon->getCurrRecvLatency() * 1000.0 , _perfMon->getAvgRecvLatency() * 1000.0
		       ,_perfMon->getCurrUpdateDelay() * 1000.0 , _perfMon->getAvgUpdateDelay() * 1000.0
		        /*_perfMon->getCurrEqDelay() * 1000.0, _perfMon->getAvgEqDelay() * 1000.0,*/
		        ,_perfMon->getCurrDrawLatency() * 1000.0 , _perfMon->getAvgDrawLatency() * 1000.0

		        ,_perfMon->getCurrEffectiveFps(), _perfMon->getAvgEffectiveFps()
//		        ,_perfMon->getCurrAbsDeviation(), _perfMon->getAvgAbsDeviation()
//		        ,_perfMon->getRecvFpsVariance()
		        ,_perfMon->getCurrDispFps(), _perfMon->getAvgDispFps()

		        ,_perfMon->getRecvCount(), _perfMon->getDrawCount()
		        ,_perfMon->getCurrBW_Mbps()
                , _perfMon->getRequiredBW_Mbps()

		        ,_perfMon->getCpuUsage() * 100.0
//                , _perfMon->getEndNvcsw(), _perfMon->getEndNivcsw()
		        /*perfMon->getEndMaxrss(), perfMon->getEndMinflt()*/
		        );
		text.append(perftext);
		//qDebug() << "BaseWidget::" << __FUNCTION__ <<  _globalAppId << _perfMon->ts_nextframe() *1000.0 << _perfMon->deadline_miseed() * 1000.0 << "ms";
	}

	if (infoTextItem) {
		infoTextItem->setText(text);
//		infoTextItem->update();
	}
}

void SN_BaseWidget::createActions()
{
	_showInfoAction = new QAction("Show Info", this);
	_hideInfoAction = new QAction("Hide Info", this);

	if ( _showInfo ) {
		_showInfoAction->setDisabled(true);
	}
	else {
		_hideInfoAction->setDisabled(true);
	}

	/* this is enabled only when affInfo is not null */
	//_affinityControlAction = new QAction("Affinity Control", this);
	//_affinityControlAction->setDisabled(true);

	_minimizeAction = new QAction("Minimize", this);
	_maximizeAction = new QAction("Maximize", this);
	_restoreAction = new QAction("Restore", this);
	_restoreAction->setDisabled(true);
	_closeAction = new QAction("Close", this);
	//_closeAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_W) );

	addAction(_showInfoAction);
	addAction(_hideInfoAction);
	//addAction(_affinityControlAction);
	addAction(_minimizeAction);
	addAction(_maximizeAction);
	addAction(_restoreAction);
	addAction(_closeAction);

	connect(_showInfoAction, SIGNAL(triggered()), this, SLOT(drawInfo()));
	connect(_hideInfoAction, SIGNAL(triggered()), this, SLOT(hideInfo()));
	//connect(_affinityControlAction, SIGNAL(triggered()), this, SLOT(showAffinityControlDialog()));
	connect(_minimizeAction, SIGNAL(triggered()), this, SLOT(minimize()));
	connect(_maximizeAction, SIGNAL(triggered()), this, SLOT(maximize()));
	connect(_restoreAction, SIGNAL(triggered()), this, SLOT(restore()));
//	connect(_closeAction, SIGNAL(triggered()), this, SLOT(fadeOutClose()));
	connect(_closeAction, SIGNAL(triggered()), this, SLOT(close()));
}



