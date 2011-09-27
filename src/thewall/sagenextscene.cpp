#include "sagenextscene.h"
#include <QGraphicsItem>

#include "common/commonitem.h"
#include "applications/base/basewidget.h"
#include "system/resourcemonitor.h"
#include "uiserver/uiserver.h"

SAGENextScene::SAGENextScene(const QRectF &sceneRect, const QSettings *s, QObject *parent)
    : QGraphicsScene(sceneRect, parent)
    , _settings(s)
    , _uiserver(0)
//    , _rmonitor(0)
    , _schedcontrol(0)
    , _rootLayoutWidget(0)
    , _closeFlag(false)
    , _closeButton(0)
    , _appRemoveButton(0)
{
	/*
	  Attach close button on the scene. if clicked twice, scene->deleteLater will be called
      */
	QPixmap closeIcon(":/resources/x_circle_gray.png");
//	QPixmap closeIcon(":/resources/powerbutton_black_64x64.png");
//	PixmapCloseButtonOnScene *closeButton = new PixmapCloseButtonOnScene(closeIcon.scaledToWidth(sceneRect.width() * 0.02));
	_closeButton = new PixmapButton(closeIcon, _settings->value("gui/iconwidth").toDouble());
	connect(_closeButton, SIGNAL(clicked()), this, SLOT(prepareClosing()));
//	QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect;
//	opacity->setOpacity(0.2);
//	closeButton->setGraphicsEffect(opacity);
	_closeButton->setOpacity(0.1);
	_closeButton->setPos(sceneRect.width() - _closeButton->boundingRect().width() - 10, 10);
//	_closeButton->setScale(0.5);
	addItem(_closeButton);



	_appRemoveButton = new PixmapButton(":/resources/default_button_up.png", 2 * _settings->value("gui/iconwidth").toDouble(), "Remove");
	_appRemoveButton->setTransformOriginPoint(_appRemoveButton->size().width()/2, 0);
	_appRemoveButton->setPos(sceneRect.width()/2, 5);
	_appRemoveButton->setZValue(999999998); // 1 less than polygon arrow
	addItem(_appRemoveButton);



	/**
	  Create base widget for wall partitioning.
	  */
	_rootLayoutWidget = new SAGENextLayoutWidget("ROOT", sceneRect, 0, _settings);

	/**
	  Child widgets of _rootLayoutWidget, which are sagenext applications, will be added to the scene automatically.
      refer QGraphicsItem::setParentItem()
	  So, do NOT explicitly additem(sagenext application) !!
	  */
	addItem(_rootLayoutWidget);
}

bool SAGENextScene::isOnAppRemoveButton(const QPointF &scenepos) {
	if (!_appRemoveButton) return false;

	return _appRemoveButton->geometry().contains(scenepos);
}

void SAGENextScene::prepareClosing() {
	if (_closeFlag) {
		// close UiServer so that all the shared pointers can be deleted first
		if (_uiserver) {
			qDebug() << "Scene is deleting UiServer";
			delete _uiserver;
		}

//		if (_rmonitor) {
//			qDebug() << "Scene is deleting ResourceMonitor";
//			delete _rmonitor;
//		}

		deleteLater();
	}
	else {
		// set the flag
		_closeFlag = true;
		_closeButton->setOpacity(1.0);
	}
}

SAGENextScene::~SAGENextScene() {
	if (_closeButton) {
		removeItem(_closeButton);
		delete _closeButton;
	}

	if (_appRemoveButton) {
		removeItem(_appRemoveButton);
		delete _appRemoveButton;
	}

	if (_rootLayoutWidget) {
		removeItem(_rootLayoutWidget);
		delete _rootLayoutWidget;
	}

	foreach (QGraphicsItem *item, items()) {
		if (!item) continue;
		if (item->type() >= QGraphicsItem::UserType + 12) {
			// this is user application
			BaseWidget *bw = static_cast<BaseWidget *>(item);
//			bw->fadeOutClose();
			bw->hide();
			bw->close();
		}
		else {
			// this probably is common GUI
			delete item;
		}
	}
	/*
	  close all views
	  */
	qDebug() << "\nScene is closing all views";
	foreach (QGraphicsView *view, views()) {
		view->close(); // WA_DeleteOnClose is set
	}
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}


void SAGENextScene::closeAllUserApp() {
	foreach (QGraphicsItem *item, items()) {
		if (!item ) continue;
		if (item->type() >= QGraphicsItem::UserType + 12) {
			// this is user application
			BaseWidget *bw = static_cast<BaseWidget *>(item);
			Q_ASSERT(bw);
			bw->fadeOutClose();
		}
	}
}










SAGENextLayoutWidget::SAGENextLayoutWidget(const QString &pos, const QRectF &r, SAGENextLayoutWidget *parentWidget, const QSettings *s, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _settings(s)
    , _parentWidget(parentWidget)
    , _leftWidget(0)
    , _rightWidget(0)
    , _topWidget(0)
    , _bottomWidget(0)
    , _bar(0)
    , _tileButton(0)
    , _hButton(0)
    , _vButton(0)
    , _xButton(0)
    , _buttonGrp(0)
    , _isTileOn(false)
    , _position(pos)
{
//	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemHasNoContents, true);// don't paint anything

	// pointer->setAppUnderPointer() will pass this item
	setAcceptedMouseButtons(0);

	// these png files are 499x499
	_hButton = new PixmapButton(":/resources/horizontal_divider_btn_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);
	_vButton = new PixmapButton( ":/resources/vertical_divider_btn_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);

	// horizontal button will divide the widget vertically
	connect(_hButton, SIGNAL(clicked()), this, SLOT(createHBar()));
	// vertical button will divide the widget horizontally
	connect(_vButton, SIGNAL(clicked()), this, SLOT(createVBar()));

//	_buttonGrp = new QGraphicsItemGroup(this);
//	_buttonGrp->addToGroup(_vButton);
//	_buttonGrp->addToGroup(_hButton);

	if (parent) {
		_xButton = new PixmapButton(":/resources/close_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);
		connect(_xButton, SIGNAL(clicked()), _parentWidget, SLOT(deleteChildPartitions()));
//		_buttonGrp->addToGroup(_xButton);
	}
	else {

	}
//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
//	setMinimumSize(r.size());

	setLayout(0);

	resize(r.size());
	setPos(r.topLeft()); // partitionRect.topLeft is in it's parent's coordinate
}

SAGENextLayoutWidget::~SAGENextLayoutWidget() {
}


void SAGENextLayoutWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	QPen pen;
	pen.setColor(QColor(Qt::white));
	painter->setPen(pen);
	painter->drawRect(boundingRect());
}

void SAGENextLayoutWidget::resizeEvent(QGraphicsSceneResizeEvent *e) {
	// adjust button position
	setButtonPos();

	//
	// this signal will make parentWidget to call adjustBar()
	//
	emit resized();

	// upon resizing, resize my childs as well
	if (_bar) {
		QSizeF deltaSize = e->newSize() - e->oldSize();

		if (_bar->orientation() == Qt::Horizontal) {
			//
			// bar is horizontal so my children is at TOP and BOTTOM
			//
			_topWidget->setPos(0, 0);

			if (deltaSize.height() == 0) {
				//
				// I'm resized horizontally.  -> only width changes for my childs
				//
				_topWidget->resize(   e->newSize().width() ,    _topWidget->size().height());
				_bottomWidget->resize(e->newSize().width() , _bottomWidget->size().height());

				_bottomWidget->setPos(0, _bottomWidget->geometry().y());
			}
			else if (deltaSize.width() == 0) {
				//
				// resized vertically, top and bottom child widgets will share height delta
				//
				_topWidget->resize(      _topWidget->size().width() ,    _topWidget->size().height() + deltaSize.height() / 2.0);
				_bottomWidget->resize(_bottomWidget->size().width() , _bottomWidget->size().height() + deltaSize.height() / 2.0);

				_bottomWidget->setPos(0, _bottomWidget->geometry().y() + deltaSize.height()/2.0);
			}
		}
		else {
			//
			// the bar is vertical so my children is at LEFT and RIGHT
			//
			_leftWidget->setPos(0,0);

			if (deltaSize.height() == 0) {
				// I'm resized horizontally. -> no height changes in my children
				_leftWidget->resize(_leftWidget->size().width() + deltaSize.width()/2.0,  _leftWidget->size().height());
				_rightWidget->resize(_rightWidget->size().width() + deltaSize.width()/2.0, _rightWidget->size().height());

				_rightWidget->setPos(_rightWidget->geometry().x() + deltaSize.width()/2.0 , 0);
			}
			else if (deltaSize.width() == 0) {
				// I'm resized vertically -> no width changes in my children
				_leftWidget->resize( _leftWidget->size().width(),  e->newSize().height());
				_rightWidget->resize(_rightWidget->size().width(), e->newSize().height());

				_rightWidget->setPos(_rightWidget->geometry().x(), 0);
			}

		}
	}

	// If growing, do nothing
	// If shrinking, move baseWidgets accordingly

	// First, check all child basewidgets,
	// and un-child if the basewidget has moved (by user) from this widget
	// or set as a child if new basewidget has come.
	// So, un-child everything and find colliding items and make them as childs
}

void SAGENextLayoutWidget::createChildPartitions(Qt::Orientation dividerOrientation) {
//	QGraphicsLinearLayout *linear = new QGraphicsLinearLayout(orientation);
//	linear->setContentsMargins(0, 0, 0, 0);

	QRectF first;
	QRectF second;
//	QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
//	linear->setSizePolicy(sp);

	// create PartitionBar child item
	_bar = new PartitionBar(dividerOrientation, this, this);

	QRectF br = boundingRect();
	if (dividerOrientation == Qt::Horizontal) {
		//
		// bar is horizontal, partition is Top and Bottom (same X pos)
		//
		first = QRectF(    0, 0,             br.width(), br.height()/2);
		second = QRectF(0, br.height()/2, br.width(), br.height()/2);

		_topWidget = new SAGENextLayoutWidget("top", first, this, _settings, this);
		_bottomWidget = new SAGENextLayoutWidget("bottom", second, this, _settings, this);

		//
		// if child widget is resized, then adjust my bar pos and length
		//
		connect(_topWidget, SIGNAL(resized()), this, SLOT(adjustBar()));
	}
	else {
		//
		// bar is vertical, partition is Left and Right (same Y pos)
		//
		first = QRectF(    0,            0, br.width()/2, br.height());
		second = QRectF(br.width()/2, 0, br.width()/2, br.height());
		_leftWidget = new SAGENextLayoutWidget("left", first, this, _settings, this);
		_rightWidget = new SAGENextLayoutWidget("right", second, this, _settings, this);

		connect(_leftWidget, SIGNAL(resized()), this, SLOT(adjustBar()));
	}

	//
	// sets bar's pos and length
	//
	adjustBar();

//	linear->insertItem(0, _left_top_w);
//	linear->insertItem(1, _right_bottom_w);

	// QGraphicsWidget takes ownership of the linear
//	setLayout(linear);

	// hides my buttons. This widget will just hold child widget
//	_buttonGrp->hide();
	_vButton->hide();
	_hButton->hide();
	if (_xButton) _xButton->hide();
}

void SAGENextLayoutWidget::adjustBar() {
	Q_ASSERT(_bar);
	if ( _bar->orientation() == Qt::Horizontal ) {
		// my children is top and bottom widgets
		QPointF bottomLeft = _topWidget->geometry().bottomLeft();
		QPointF bottomRight = _topWidget->geometry().bottomRight();

		_bar->setLine(bottomLeft.x(), bottomLeft.y(), bottomRight.x(), bottomRight.y());
	}
	else {
		// my children is left and right widgets
		QPointF topRight = _leftWidget->geometry().topRight();
		QPointF bottomRight = _leftWidget->geometry().bottomRight();

		_bar->setLine(topRight.x(), topRight.y(), bottomRight.x(), bottomRight.y());
	}
}

void SAGENextLayoutWidget::deleteChildPartitions() {
//	layout()->removeAt(0);
//	layout()->removeAt(1); // widget won't be deleted
	Q_ASSERT(_bar);
	if ( _bar->orientation() == Qt::Horizontal) {
		delete _topWidget;
		delete _bottomWidget;
	}
	else {
		delete _leftWidget;
		delete _rightWidget;
	}
	delete _bar;

	if (_isTileOn) {
		doTile();
	}
	else {
		setLayout(0);
	}

//	_buttonGrp->show();
	_vButton->show();
	_hButton->show();
	if (_xButton) _xButton->show();
}

void SAGENextLayoutWidget::doTile() {
	// I must be an actual partition

	QGraphicsGridLayout *grid = new QGraphicsGridLayout;

	// add all basewidget into this grid

	setLayout(grid);
}

void SAGENextLayoutWidget::setButtonPos() {
//	qDebug() << "attachButton" << boundingRect() << geometry();
	_vButton->setPos(size().width()-_vButton->size().width() - 10,  size().height()/2);
	_hButton->setPos(_vButton->geometry().x(),  _vButton->geometry().bottom());
	if (_xButton) {
		_xButton->setPos(_hButton->geometry().x(),  _hButton->geometry().bottom());
	}
}






PartitionBar::PartitionBar(Qt::Orientation ori, SAGENextLayoutWidget *owner, QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
    , _ownerNode(owner)
    , _orientation(ori)
{
//	setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, true);

//	setAcceptedMouseButtons(0);

	QPen pen;
	pen.setWidth(12);
	pen.setStyle(Qt::DashDotLine);
	pen.setColor(QColor(Qt::white));
	setPen(pen);
}






















//PartitionTreeNode::PartitionTreeNode(QGraphicsScene *s, PartitionTreeNode *p, const QRectF &r, QObject *parent)
//    : QObject(parent)
//    , _scene(s)
//    , _parentNode(p)

//    , _leftOrTop(0)
//    , _rightOrBottom(0)
//    , _orientation(0)
//    , _lineItem(0)

//    , _tileButton(0)
//    , _hButton(0)
//    , _vButton(0)
//    , _closeButton(0)
//{
//	_rectf = r;

//	qDebug() << "PartitionTreeNode() " << _rectf;

//	_hButton = new PixmapButton(":/resources/minimize_shape.gif");
//	connect(_hButton, SIGNAL(clicked()), this, SLOT(createNewHPartition()));
//	// add this button to the scene based on _rectf
//	_scene->addItem(_hButton);
//	_hButton->setPos(_rectf.width() - _hButton->rect().width(), _rectf.y() +  _rectf.height() / 2);


//	if ( _parentNode ) {
//		_closeButton = new PixmapButton(":/resources/close_over.png");
//		connect(_closeButton, SIGNAL(clicked()), _parentNode, SLOT(deleteChildPartitions()));
//		_scene->addItem(_closeButton);
//		_closeButton->setPos(_rectf.width() - _closeButton->rect().width(),  _rectf.y() + _rectf.height()/2);
//	}
//}
//PartitionTreeNode::~PartitionTreeNode() {
//	if (_lineItem) delete _lineItem;

//	if (_hButton) delete _hButton;
//	if (_vButton) delete _vButton;
//	if (_closeButton) delete _closeButton;
//	if (_tileButton) delete _tileButton;

//	if (_leftOrTop) delete _leftOrTop;
//	if (_rightOrBottom) delete _rightOrBottom;
//}

//void PartitionTreeNode::lineHasMoved(const QPointF &newpos) {
//	Q_ASSERT(_leftOrTop);
//	Q_ASSERT(_rightOrBottom);

//	QRectF leftrect;
//	QRectF rightrect;

//	if (_orientation == 1) {
//		// horizontal
//		// line only moves up and down

//		// top
//		leftrect = QRectF( _rectf.x(), _rectf.y(), _rectf.width(), newpos.y() - _rectf.y() );

//		// bottom
//		rightrect = QRectF(_rectf.x(), newpos.y(), _rectf.width(), _rectf.bottomRight().y() - newpos.y());
//	}
//	else if (_orientation ==2) {

//	}

//	_leftOrTop->setNewRectf(leftrect);
//	_rightOrBottom->setNewRectf(rightrect);
//}

//void PartitionTreeNode::createNewPartition(int o) {
//	_orientation = o;

//	QRectF leftrect;
//	QRectF rightrect;
//	Qt::Orientation ori;

//	if (o == 1) {
//		// horizontal
//		ori = Qt::Horizontal;
//		qreal newHeight = _rectf.height() / 2.0;
//		leftrect = QRectF(_rectf.x(), _rectf.y(), _rectf.width(), newHeight);
//		rightrect = QRectF(_rectf.x(), _rectf.y() + newHeight, _rectf.width(), newHeight);
//	}
//	else if ( o == 2) {
//		ori = Qt::Vertical;
//	}
//	_leftOrTop = new PartitionTreeNode(_scene, this, leftrect);
//	_rightOrBottom = new PartitionTreeNode(_scene, this, rightrect);

//	// hide my buttons
//	if (_hButton) _hButton->hide();
//	if (_closeButton) _closeButton->hide();


//	// create line item and I'm the owner
////	_lineItem = new PartitionBar(ori, this);
//	_lineItem->setPos(rightrect.topLeft());
//	_scene->addItem(_lineItem);

//}

//void PartitionTreeNode::deleteChildPartitions() {
//	Q_ASSERT(_lineItem);
//	Q_ASSERT(_leftOrTop);
//	Q_ASSERT(_rightOrBottom);

//	delete _lineItem;
//	delete _leftOrTop;
//	delete _rightOrBottom;

//	// show my buttons
//	_hButton->show();
//	if (_parentNode && _closeButton) _closeButton->show();
//}

//void PartitionTreeNode::setNewRectf(const QRectF &r) {
//	qDebug() << "PartitionTreeNode::setNewRectf()" << r;

//	// because I'm an actual partition
//	Q_ASSERT( !_lineItem );
//	Q_ASSERT( !_leftOrTop );
//	Q_ASSERT( !_rightOrBottom );

//	_rectf = r;

//	// adjust belonging widget's pos
//}
