#include "basewidget.h"
#include "appinfo.h"
#include "../../common/commonitem.h"
#include "perfmonitor.h"

//#include "../../system/resourcemonitor.h"
#include "affinityinfo.h"
//#include "../../system/affinitycontroldialog.h"

#include "../../sagenextscene.h"

#include <QtGui>

#if QT_VERSION < 0x040700
#include <sys/time.h>
#endif

SN_BaseWidget::SN_BaseWidget(Qt::WindowFlags wflags)
	: QGraphicsWidget(0, wflags)
	, _globalAppId(-1)
	, _settings(0)
	, _windowState(SN_BaseWidget::W_NORMAL)
	, _appInfo(new AppInfo())
	, _showInfo(false)
	, _perfMon(new PerfMonitor(this))
	, _affInfo(0)
	, infoTextItem(0)
	, _lastTouch(0)
	, _rMonitor(0)
	, _quality(1.0)
	, _contextMenu(0)
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
    , timerID(0)
	, _priority(0.5)
	, _registerForMouseHover(false)
    , _priorityQuantized(0)
{
	init();
}

SN_BaseWidget::SN_BaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent /*0*/, Qt::WindowFlags wflags /*0*/)
	: QGraphicsWidget(parent, wflags)
	, _globalAppId(globalappid)
	, _settings(s)
	, _windowState(SN_BaseWidget::W_NORMAL)
	, _appInfo(new AppInfo())
	, _showInfo(false)
	, _perfMon(new PerfMonitor(this))
	, _affInfo(0)
	, infoTextItem(0)
	, _lastTouch(0)
	, _rMonitor(0)
	, _quality(1.0)
	, _contextMenu(0)
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
    , timerID(0)
	, _priority(0.5)
	, _registerForMouseHover(false)
    , _priorityQuantized(0)
{
	// This will affect boundingRect(), windowFrameRect() of the widget.
	qreal fmargin = _settings->value("gui/framemargin", 8).toDouble();
	if (isWindow()) {
		// window frame is not interactible by shared pointers
		setWindowFrameMargins(0, 0, 0, 0);
		
		// Qt::Window might want to define mouse dragging. For that case, give more room to top margin
		setContentsMargins(fmargin, fmargin + 20, fmargin, fmargin); // by default, this is 0 0 0 0
	}
	else {
		// setting frameMargins won't have any effect.. (Qt::Widget doesn't have frame)
		// setting contentMargins won't have any effect unless this widget has a layout.
	}

	init();
}

SN_BaseWidget::~SN_BaseWidget()
{
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

    qDebug("BaseWidget::%s() : Removed widget %llu from scene.", __FUNCTION__, _globalAppId);
}


void SN_BaseWidget::setWidgetType(Widget_Type wt) {
    _widgetType = wt;
    if (_perfMon) {
        _perfMon->setWidgetType(wt);
    }
}


void SN_BaseWidget::init()
{
	//
	// Destructor will be called by close()
	//
	setAttribute(Qt::WA_DeleteOnClose, true);

	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);

	//
	/* When enabled, the item's paint() function will be called only once for each call to update(); for any subsequent repaint requests, the Graphics View framework will redraw from the cache. */
	/* Turn cache off for streaming application */
	//
	//
//	setCacheMode(QGraphicsItem::DeviceCoordinateCache);

	//qDebug() << "BaseWidget::init() : boundingRect" << boundingRect() << "windowFrameRect" << windowFrameRect();
	//getWindowFrameMargins(&frameMarginLeft, &frameMarginTop, &frameMarginRight, &frameMarginBottom);

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



	//
	// Indicates that the widget paints all its pixels when it receives a paint event
	// Thus, it is not required for operations like updating, resizing, scrolling and focus changes
	// to erase the widget before generating paint events.
	setAttribute(Qt::WA_OpaquePaintEvent, true);


	//
	// To be effective, unset WA_OpaquePaintEvent
	setAutoFillBackground(false);


        /*!
          graphics effect
          */
//	shadow = new QGraphicsDropShadowEffect(this);
//	shadow->setBlurRadius(35);
//	shadow->setColor(Qt::gray);;
//	shadow->setEnabled(false);
//	setGraphicsEffect(shadow); // you can set only one effect
}


qreal SN_BaseWidget::ratioToTheWall() const {
	if(!scene()) return 0.0;

	QSizeF sceneSize = scene()->sceneRect().size();
	QSizeF currentSize = size() * scale();

	qreal sceneArea = sceneSize.width() * sceneSize.height();
	qreal currentArea = currentSize.width() * currentSize.height();

	if ( currentArea > sceneArea ) return 1;

	return currentArea / sceneArea;
}

QRegion SN_BaseWidget::effectiveVisibleRegion() const {
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


qreal SN_BaseWidget::priority(qint64 ctepoch /* 0 */) {
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



void SN_BaseWidget::drawInfo()
{
	if (!_showInfo) {
		_showInfo = true;
		_showInfoAction->setDisabled(true);
		_hideInfoAction->setEnabled(true);
		//update();

		/* starts timer */
		timerID = startTimer(1000); // timerEvent every 1000 msec
	}
}

void SN_BaseWidget::hideInfo()
{
	if (_showInfo) {
		killTimer(timerID);
		_showInfo = false;
		_hideInfoAction->setDisabled(true);
		_showInfoAction->setEnabled(true);
		update();
	}
}

void SN_BaseWidget::minimize()
{
	if ( !_appInfo  ||  _windowState == W_MINIMIZED ) return;
	// icon size must be defined somewhere based on scene rectangle
	// QSetting pref dialog would be good point to start

	// based on this, I could use QGraphicsLayoutItem::setMinimumWidth/Height

	// before minimizing, widget's bounding rectangle must be saved.
	_appInfo->setRecentPos(pos());
	_appInfo->setRecentSize(size());
	_appInfo->setRecentScale(scale());
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

	// record current position and scale
	Q_ASSERT(_appInfo);
	_appInfo->setRecentPos(pos());
	_appInfo->setRecentSize(size());
	_appInfo->setRecentScale(scale());

	QSizeF s = size(); // current size of the widget. scaling won't change the size of the widget

	qreal scaleFactorW = 0;
	qreal scaleFactorH = 0;

	if (parentWidget()) {
		scaleFactorW = parentWidget()->size().width() / s.width();
		scaleFactorH = parentWidget()->size().height() / s.height();
	}
	else {
		// if there's no SN_LayoutWidget
		scaleFactorW = scene()->width() / s.width();
		scaleFactorH = scene()->height() / s.height();
	}
	qreal scaleFactor = 1.0;
	(scaleFactorW < scaleFactorH) ? scaleFactor = scaleFactorW : scaleFactor = scaleFactorH;


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
				resize(parentWidget()->size() - QSizeF(40, 60));
				setPos(parentWidget()->boundingRect().topLeft() + QPointF(20,40));
			}
			else {
				resize(scene()->sceneRect().size()  -  QSizeF(40, 60));
				setPos(scene()->sceneRect().topLeft() + QPointF(20, 40));
			}
		}
	}
	else {
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
				qreal xoffset = (parentWidget()->size().width() - s.width())/2;
				qreal yoffset = (parentWidget()->size().height() - s.height())/2;
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
		if ( !item ||  item->type() < UserType + 2 ) continue;

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



void SN_BaseWidget::reScale(int tick, qreal factor)
{
	qreal currentScale = scale();

	QSizeF currentVisibleSize = currentScale * size();
	qreal currentArea = currentVisibleSize.width() * currentVisibleSize.height();

	// shouldn't be too small
	if ( tick < 0  &&  (currentScale <= 0.05 || currentArea <= 400)) return;

	currentScale += ((qreal)tick * factor);

	// Note : Item transformations accumulate from parent to child, so if both a parent and child item are rotated 90 degrees,
	//the child's total transformation will be 180 degrees.
	//Similarly, if the item's parent is scaled to 2x its original size, its children will also be twice as large.
	//An item's transformation does not affect its own local geometry;
	//all geometry functions (e.g., contains(), update(), and all the mapping functions) still operate in local coordinates.
	setScale(currentScale);

	//! optional
//	appInfo->setRecentScale(currentScale);

// This function will not change widget's size !!
//	qDebug() << "size: " << size() << "boundingRect" << boundingRect() << "geometry" << geometry();
}

QRectF SN_BaseWidget::resizeHandleSceneRect()
{
	QSizeF size(100, 100);
	QPointF pos( boundingRect().width() - size.width(), boundingRect().height() - size.height() );
	return mapRectToScene(QRectF(pos, size));
}



void SN_BaseWidget::updateInfoTextItem()
{
	if (!infoTextItem) return;

	QByteArray infotext(256, '\0');
	QByteArray perftext(512, '\0');
	QByteArray afftext(256, '\0');
	QString text("");

	//! let's show scene geometry of this widget
	//QRectF rect = mapRectToScene(boundingRect());
	QRectF rect = geometry();

	if (_appInfo) {
		sprintf(infotext.data(), "Global App ID %llu\nNative WxHxbpp: %ux%ux%u (%u KB)\nCurr Geometry: (%.0f,%.0f)(%.0fx%.0f)\nCurr Scale: %.1f\nRatio to wall %.3f"
		        /*qPrintable(appInfo->getFilename()),*/
		        /* SizeRatio: %u %%\n */
		        , _globalAppId
		        , _appInfo->nativeSize().width() ,_appInfo->nativeSize().height(), _appInfo->bitPerPixel(), _appInfo->frameSizeInByte() / 1024 // to KiloByte
		        , rect.topLeft().x(), rect.topLeft().y()
		        , rect.width(), rect.height()
		        , scale()
		        //appInfo->ratioToTheScene( scene()->width() * scene()->height() ),
		        /*_appInfo->getDrawingThreadCpu()*/
		        , ratioToTheWall()
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
		sprintf(perftext.data(), "\nCurr/Avg Recv Lat : %.2f / %.2f ms\nCurr/Avg Conv Lat : %.2f / %.2f ms\nCurr/Avg Draw Lat : %.2f / %.2f ms\n\nCurr/Avg Recv FPS : %.2f / %.2f\nCurr/Avg recvFps AbsDevi. %.2f / %.2f\nRecv FPS variance %.4f\nCurr/Avg Disp FPS : %.2f / %.2f\n\nRecv/Draw Count %llu/%llu\nCurr Bandwidth : %.3f Mbps\nCPU usage : %.6f\nVol/InVol Ctx Sw : %ld / %ld" /*maxrss %ld, pageReclaims %ld"*/
		        ,_perfMon->getCurrRecvLatency() * 1000.0 , _perfMon->getAvgRecvLatency() * 1000.0
		       ,_perfMon->getCurrConvDelay() * 1000.0 , _perfMon->getAvgConvDelay() * 1000.0
		        /*_perfMon->getCurrEqDelay() * 1000.0, _perfMon->getAvgEqDelay() * 1000.0,*/
		        ,_perfMon->getCurrDrawLatency() * 1000.0 , _perfMon->getAvgDrawLatency() * 1000.0

		        ,_perfMon->getCurrRecvFps(), _perfMon->getAvgRecvFps()
		        ,_perfMon->getCurrAbsDeviation(), _perfMon->getAvgAbsDeviation()
		        ,_perfMon->getRecvFpsVariance()
		        ,_perfMon->getCurrDispFps(), _perfMon->getAvgDispFps()

		        ,_perfMon->getRecvCount(), _perfMon->getDrawCount()
		        ,_perfMon->getCurrBandwidthMbps()

		        ,_perfMon->getCpuUsage() * 100.0, _perfMon->getEndNvcsw(), _perfMon->getEndNivcsw()
		        /*perfMon->getEndMaxrss(), perfMon->getEndMinflt()*/
		        );
		text.append(perftext);
		//qDebug() << "BaseWidget::" << __FUNCTION__ <<  _globalAppId << _perfMon->ts_nextframe() *1000.0 << _perfMon->deadline_miseed() * 1000.0 << "ms";
	}

	if (infoTextItem) {
		infoTextItem->setText(QString(text));
		infoTextItem->update();
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






void SN_BaseWidget::handlePointerDrag(const QPointF & /*pointerScenePos*/, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton btn, Qt::KeyboardModifier) {

	if (btn == Qt::LeftButton) {
		moveBy(pointerDeltaX, pointerDeltaY);
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
	


	if (! isWindow()) {
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


		/**********
		  Below draws rectangle around widget's content (using boundingRect)
		  Because of this, it has to set Pen (which chagens painter state)
		  ********/
		/*
		QPen pen;
		pen.setWidth( _settings->value("gui/framemargin",0).toInt() );
		pen.setBrush(brush);

		painter->setPen(pen);
		painter->drawRect(boundingRect());
		*/


		/***************
		  Below fills rectangle (works as background) so it has to be called BEFORE any drawing code
		  ***********/
		painter->fillRect(boundingRect(), brush);
	}






	/* info overlay */
	if ( _showInfo  &&  !infoTextItem->isVisible() ) {
#if defined(Q_OS_LINUX)
		_appInfo->setDrawingThreadCpu(sched_getcpu());
#endif
		Q_ASSERT(infoTextItem);
		//painter->setBrush(Qt::black);
		//painter->drawRect(infoTextItem->boundingRect());
		infoTextItem->show();
	}
	else if (!_showInfo && infoTextItem->isVisible()){
		Q_ASSERT(infoTextItem);
		infoTextItem->hide();
	}
}


void SN_BaseWidget::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

//    qDebug() << "size" << size() << "boundingrect" << boundingRect() << "geometry" << geometry();

    QLinearGradient lg(boundingRect().topLeft(), boundingRect().bottomLeft());
	if (isSelected()) {
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
	updateInfoTextItem();
        /*
        if ( affControl && affControl->isVisible() ) {
                affControl->updateInfo();
        }
        */
}

void SN_BaseWidget::resizeEvent(QGraphicsSceneResizeEvent *e) {
	setTransformOriginPoint(e->newSize().width() / 2, e->newSize().height() / 2);
//	setTransformOriginPoint(boundingRect().center());
}

void SN_BaseWidget::setLastTouch() {
#if QT_VERSION < 0x040700
    struct timeval tv;
    gettimeofday(&tv, 0);
    _lastTouch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#else
    _lastTouch = QDateTime::currentMSecsSinceEpoch();
#endif
}


/*!
  reimplement this so the item can be the mousegrabber
  */
void SN_BaseWidget::mousePressEvent(QGraphicsSceneMouseEvent *event) {
//	qDebug() << "BaseWidget::mousePressEvent() : buttons"<< event->button() << "pos:" << event->pos() << " ,scenePos:" << event->scenePos() << " ,screenPos:" << event->screenPos();
    if ( event->buttons() & Qt::LeftButton) {
        // refresh lastTouch
#if QT_VERSION < 0x040700
        struct timeval tv;
        gettimeofday(&tv, 0);
        _lastTouch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#else
        _lastTouch = QDateTime::currentMSecsSinceEpoch();
#endif
        // change zvalue
		// This is called by PolygonArrow::pointerPress()
//        setTopmost();
    }

	/*
	  event will by default be accepted, and this item is then the mouse grabber
	  */

	/*
	  The mouse press event decides which item should become the mouse grabber (see QGraphicsScene::mouseGrabberItem()).
      If you do not reimplement this function, the press event will propagate to any topmost item beneath this item,
      and no other mouse events will be delivered to this item.
	  */
//    QGraphicsWidget::mousePressEvent(event);
}

void SN_BaseWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {

	// If you keep the base implementation for console mouse interaction,
	// it makes multiple item selection with shared pointer unavailable
//	QGraphicsItem::mouseReleaseEvent(e);
}


void SN_BaseWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
//	Q_UNUSED(event);
	//qDebug() << "doubleClickEvent" << event->lastPos() << event->pos() << ", " << event->lastScenePos() << event->scenePos() << ", " << event->lastScreenPos() << event->screenPos();
	if ( mapRectToScene(boundingRect()).contains(event->scenePos()) )
		maximize();
}

void SN_BaseWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	_contextMenu->exec(event->screenPos());
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
	}

	// reimplementation will accept the event
}



