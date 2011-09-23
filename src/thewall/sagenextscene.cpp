#include "sagenextscene.h"
#include <QGraphicsItem>

#include "common/commonitem.h"
#include "applications/base/basewidget.h"
#include "system/resourcemonitor.h"
#include "uiserver/uiserver.h"

SAGENextScene::SAGENextScene(const QRectF &sceneRect, QObject *parent)
    : QGraphicsScene(sceneRect, parent)
    , _uiserver(0)
    , _rmonitor(0)
    , _schedcontrol(0)
    , _rootLayoutWidget(0)
{
	/*
	  Attach close button on the scene. if clicked, scene->deleteLater will be called
      */
	QPixmap closeIcon(":/resources/close_over.png");
//	QPixmap closeIcon(":/resources/powerbutton_black_64x64.png");
	PixmapCloseButtonOnScene *closeButton = new PixmapCloseButtonOnScene(closeIcon.scaledToWidth(sceneRect.width() * 0.02));
//	QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect;
//	opacity->setOpacity(0.2);
//	closeButton->setGraphicsEffect(opacity);
	closeButton->setOpacity(0.2);
	closeButton->setX(sceneRect.width() - closeButton->boundingRect().width() - 1);
	addItem(closeButton);



	_rootLayoutWidget = new SAGENextLayoutWidget(sceneRect);
	addItem(_rootLayoutWidget);
}

void SAGENextScene::prepareClosing() {
	// close UiServer

	if (_uiserver) {
		qDebug() << "Scene is deleting UiServer";
		delete _uiserver;
	}


	if (_rmonitor) {
		qDebug() << "Scene is deleting ResourceMonitor";
		delete _rmonitor;
	}

//	::sleep(1);
}

SAGENextScene::~SAGENextScene() {
	if (_rootLayoutWidget) {
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

	qDebug() << "\nScene is closing all views";

	/*
	  close all views
	  */
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










SAGENextLayoutWidget::SAGENextLayoutWidget(const QRectF &r, SAGENextLayoutWidget *parentWidget, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _parentWidget(parentWidget)
    , _left_top_w(0)
    , _right_bottom_w(0)
    , _bar(0)
    , _tileButton(0)
    , _hButton(0)
    , _vButton(0)
    , _xButton(0)
    , _buttonGrp(0)
    , _isTileOn(false)
{
//	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
//	setFlag(QGraphicsItem::ItemHasNoContents, true);// don't paint anything
	setAcceptedMouseButtons(0);

	_hButton = new PixmapButton(":/resources/minimize_shape.gif", this);
	_vButton = new PixmapButton( ":/resources/maximize_shape.gif", this);

	// horizontal button will divide the widget vertically
	connect(_hButton, SIGNAL(clicked()), this, SLOT(createVChildPartitions()));
	// vertical button will divide the widget horizontally
	connect(_vButton, SIGNAL(clicked()), this, SLOT(createHChildPartitions()));

//	_buttonGrp = new QGraphicsItemGroup(this);
//	_buttonGrp->addToGroup(_vButton);
//	_buttonGrp->addToGroup(_hButton);

	if (parent) {
		_xButton = new PixmapButton(":/resources/close_shape.gif", this);
		connect(_xButton, SIGNAL(clicked()), _parentWidget, SLOT(deleteChildPartitions()));
//		_buttonGrp->addToGroup(_xButton);
	}
	else {

	}
//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	setLayout(0);

	resize(r.size());
	setPos(r.topLeft()); // partitionRect.topLeft is in it's parent's coordinate
}

QSizeF SAGENextLayoutWidget::sizeHint(Qt::SizeHint which, const QSizeF &) const {
	return size();
}

SAGENextLayoutWidget::~SAGENextLayoutWidget() {
}

void SAGENextLayoutWidget::setButtonPos() {
//	qDebug() << "attachButton" << boundingRect() << geometry();
	_vButton->setPos(size().width()-_vButton->size().width() -10,  size().height()/2);
	_hButton->setPos(_vButton->geometry().x(),  _vButton->geometry().bottom());
	if (_xButton) {
		_xButton->setPos(_hButton->geometry().x(),  _hButton->geometry().bottom());
	}
}

void SAGENextLayoutWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	QPen pen;
	pen.setColor(QColor(Qt::white));
	painter->setPen(pen);
	painter->drawRect(boundingRect());
}

void SAGENextLayoutWidget::resizeEvent(QGraphicsSceneResizeEvent *) {
	setButtonPos();
	emit resized();
}

void SAGENextLayoutWidget::createChildPartitions(Qt::Orientation orientation) {
	QGraphicsLinearLayout *linear = new QGraphicsLinearLayout(orientation);
	linear->setContentsMargins(0, 0, 0, 0);
//	linear->setPreferredSize(size());

	QRectF left_top;
	QRectF right_bottom;
//	QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
//	linear->setSizePolicy(sp);

	// create PartitionBar child item
	_bar = new PartitionBar(orientation, this, this);

	QRectF br = boundingRect();
	if (orientation == Qt::Horizontal) {
		//
		// Left and Right
		//
//		linear->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

		left_top = QRectF(0, 0, br.width()/2, br.height());
		right_bottom = QRectF(br.width()/2, 0, br.width()/2, br.height());
//		_bar->setLine(br.width()/2, 0, br.width()/2, br.height());
	}
	else {
		//
		// Top and Bottom
		//
//		linear->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

		left_top = QRectF(0, 0, br.width(), br.height()/2);
		right_bottom = QRectF(0, br.height()/2, br.width(), br.height()/2);
//		_bar->setLine(0, boundingRect().height()/2, boundingRect().width(), boundingRect().height()/2);

	}

//	qDebug() << "createChildPartitions()" << left_top << right_bottom;
	_left_top_w = new SAGENextLayoutWidget(left_top, this, this);
	_right_bottom_w = new SAGENextLayoutWidget(right_bottom, this, this);

	connect(_left_top_w, SIGNAL(resized()), this, SLOT(adjustBar()));

	linear->insertItem(0, _left_top_w);
	linear->insertItem(1, _right_bottom_w);

	// QGraphicsWidget takes ownership of the linear
	setLayout(linear);

	// hides my buttons. This widget will just hold child widget
//	_buttonGrp->hide();
	_vButton->hide();
	_hButton->hide();
	if (_xButton) _xButton->hide();
}

void SAGENextLayoutWidget::adjustBar() {
	Q_ASSERT(_bar);
	if ( _bar->orientation() == Qt::Horizontal ) {
		// left and right
		QPointF topRight = _left_top_w->geometry().topRight();
		QPointF bottomRight = _left_top_w->geometry().bottomRight();
		_bar->setLine(topRight.x(), topRight.y(), bottomRight.x(), bottomRight.y());
	}
	else {
		QPointF bottomLeft = _left_top_w->geometry().bottomLeft();
		QPointF bottomRight = _left_top_w->geometry().bottomRight();
		_bar->setLine(bottomLeft.x(), bottomLeft.y(), bottomRight.x(), bottomRight.y());
	}
}

void SAGENextLayoutWidget::deleteChildPartitions() {
//	layout()->removeAt(0);
//	layout()->removeAt(1); // widget won't be deleted
	delete _left_top_w;
	delete _right_bottom_w;

	setLayout(0);

	Q_ASSERT(_bar);
	delete _bar;

	if (_isTileOn) {
		doTile();
	}
//	_buttonGrp->show();
	_vButton->show();
	_hButton->show();
	if (_xButton) _xButton->show();
}

void SAGENextLayoutWidget::doTile() {
	// I must be an actual partition
	Q_ASSERT(layout()==0);

	QGraphicsGridLayout *grid = new QGraphicsGridLayout;

	// add all basewidget into this grid

	setLayout(grid);
}







PartitionBar::PartitionBar(Qt::Orientation ori, SAGENextLayoutWidget *owner, QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
    , _ownerNode(owner)
    , _orientation(ori)
{
//	setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
//	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, true);

	setAcceptedMouseButtons(0);

	QPen pen;
	pen.setWidth(6);
	pen.setStyle(Qt::DashDotLine);
	pen.setColor(QColor(Qt::white));
	setPen(pen);
}

QVariant PartitionBar::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemPositionHasChanged) {

		QPointF newpos = value.toPointF();
		qDebug() << "PartitionBar::itemChange() new position" << newpos;

/*
		// adjust colliding line item's size
		QList<QGraphicsItem *> citems = collidingItems(Qt::IntersectsItemBoundingRect);
		foreach(QGraphicsItem *item, citems) {
			PartitionBar *bar = dynamic_cast<PartitionBar *>(item);
			if (bar && bar->orientation() != _orientation) {
				// change size of those bars
			}
		}
		*/

		Q_ASSERT(_ownerNode);
//		_ownerNode->lineHasMoved(newpos);

	}
	return value;
}


















#include "common/commonitem.h"



PartitionTreeNode::PartitionTreeNode(QGraphicsScene *s, PartitionTreeNode *p, const QRectF &r, QObject *parent)
    : QObject(parent)
    , _scene(s)
    , _parentNode(p)

    , _leftOrTop(0)
    , _rightOrBottom(0)
    , _orientation(0)
    , _lineItem(0)

    , _tileButton(0)
    , _hButton(0)
    , _vButton(0)
    , _closeButton(0)
{
	_rectf = r;

	qDebug() << "PartitionTreeNode() " << _rectf;

	_hButton = new PixmapButton(":/resources/minimize_shape.gif");
	connect(_hButton, SIGNAL(clicked()), this, SLOT(createNewHPartition()));
	// add this button to the scene based on _rectf
	_scene->addItem(_hButton);
	_hButton->setPos(_rectf.width() - _hButton->rect().width(), _rectf.y() +  _rectf.height() / 2);


	if ( _parentNode ) {
		_closeButton = new PixmapButton(":/resources/close_over.png");
		connect(_closeButton, SIGNAL(clicked()), _parentNode, SLOT(deleteChildPartitions()));
		_scene->addItem(_closeButton);
		_closeButton->setPos(_rectf.width() - _closeButton->rect().width(),  _rectf.y() + _rectf.height()/2);
	}
}
PartitionTreeNode::~PartitionTreeNode() {
	if (_lineItem) delete _lineItem;

	if (_hButton) delete _hButton;
	if (_vButton) delete _vButton;
	if (_closeButton) delete _closeButton;
	if (_tileButton) delete _tileButton;

	if (_leftOrTop) delete _leftOrTop;
	if (_rightOrBottom) delete _rightOrBottom;
}

void PartitionTreeNode::lineHasMoved(const QPointF &newpos) {
	Q_ASSERT(_leftOrTop);
	Q_ASSERT(_rightOrBottom);

	QRectF leftrect;
	QRectF rightrect;

	if (_orientation == 1) {
		// horizontal
		// line only moves up and down

		// top
		leftrect = QRectF( _rectf.x(), _rectf.y(), _rectf.width(), newpos.y() - _rectf.y() );

		// bottom
		rightrect = QRectF(_rectf.x(), newpos.y(), _rectf.width(), _rectf.bottomRight().y() - newpos.y());
	}
	else if (_orientation ==2) {

	}

	_leftOrTop->setNewRectf(leftrect);
	_rightOrBottom->setNewRectf(rightrect);
}

void PartitionTreeNode::createNewPartition(int o) {
	_orientation = o;

	QRectF leftrect;
	QRectF rightrect;
	Qt::Orientation ori;

	if (o == 1) {
		// horizontal
		ori = Qt::Horizontal;
		qreal newHeight = _rectf.height() / 2.0;
		leftrect = QRectF(_rectf.x(), _rectf.y(), _rectf.width(), newHeight);
		rightrect = QRectF(_rectf.x(), _rectf.y() + newHeight, _rectf.width(), newHeight);
	}
	else if ( o == 2) {
		ori = Qt::Vertical;
	}
	_leftOrTop = new PartitionTreeNode(_scene, this, leftrect);
	_rightOrBottom = new PartitionTreeNode(_scene, this, rightrect);

	// hide my buttons
	if (_hButton) _hButton->hide();
	if (_closeButton) _closeButton->hide();


	// create line item and I'm the owner
//	_lineItem = new PartitionBar(ori, this);
	_lineItem->setPos(rightrect.topLeft());
	_scene->addItem(_lineItem);

}

void PartitionTreeNode::deleteChildPartitions() {
	Q_ASSERT(_lineItem);
	Q_ASSERT(_leftOrTop);
	Q_ASSERT(_rightOrBottom);

	delete _lineItem;
	delete _leftOrTop;
	delete _rightOrBottom;

	// show my buttons
	_hButton->show();
	if (_parentNode && _closeButton) _closeButton->show();
}

void PartitionTreeNode::setNewRectf(const QRectF &r) {
	qDebug() << "PartitionTreeNode::setNewRectf()" << r;

	// because I'm an actual partition
	Q_ASSERT( !_lineItem );
	Q_ASSERT( !_leftOrTop );
	Q_ASSERT( !_rightOrBottom );

	_rectf = r;

	// adjust belonging widget's pos
}
