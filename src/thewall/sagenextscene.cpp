#include "sagenextscene.h"
#include <QGraphicsItem>

#include "common/commonitem.h"
#include "applications/base/basewidget.h"
#include "applications/base/appinfo.h"
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
	//
	// This approach is ideal for dynamic scenes, where many items are added, moved or removed continuously.
	//
	setItemIndexMethod(QGraphicsScene::NoIndex);


//	QBrush brush(Qt::black, QPixmap(":/resources/evl-logo.png")); // 1920 x 725 pixels
	setBackgroundBrush(QColor(10,10,10));
	/*
	QGraphicsPixmapItem *pi = new QGraphicsPixmapItem(QPixmap(":/resources/evl-logo.png"));
	pi->setFlag(QGraphicsItem::ItemIsMovable, false);
	pi->setFlag(QGraphicsItem::ItemIsSelectable, false);
	pi->setFlag(QGraphicsItem::ItemIsFocusable, false);
	pi->setAcceptedMouseButtons(0); // Qt::NoButton
	pi->setOpacity(0.1);
	pi->setTransformOriginPoint(pi->boundingRect().width()/2, pi->boundingRect().height()/2); // origin to center
	addItem(pi);
	*/
//	pi->setPos(sceneRect.width()/2, sceneRect.height()/2);




	/*
	  Attach close button on the scene. if clicked twice, scene->deleteLater will be called
      */
	QPixmap closeIcon(":/resources/x_circle_gray.png");
//	QPixmap closeIcon(":/resources/powerbutton_black_64x64.png");
//	PixmapCloseButtonOnScene *closeButton = new PixmapCloseButtonOnScene(closeIcon.scaledToWidth(sceneRect.width() * 0.02));
	_closeButton = new SAGENextPixmapButton(closeIcon, _settings->value("gui/iconwidth").toDouble());
	connect(_closeButton, SIGNAL(clicked()), this, SLOT(prepareClosing()));
//	QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect;
//	opacity->setOpacity(0.2);
//	closeButton->setGraphicsEffect(opacity);
	_closeButton->setOpacity(0.1);
	_closeButton->setPos(sceneRect.width() - _closeButton->boundingRect().width() - 5, 10);
//	_closeButton->setScale(0.5);
	addItem(_closeButton);


	//
	// attach app remove button on the top
	//
	_appRemoveButton = new SAGENextPixmapButton(":/resources/default_button_up.png", QSize(512, 32), "Remove");
	_appRemoveButton->setFlag(QGraphicsItem::ItemIsMovable, false);
	_appRemoveButton->setFlag(QGraphicsItem::ItemIsSelectable, false);
	_appRemoveButton->setFlag(QGraphicsItem::ItemIsFocusable, false);
	_appRemoveButton->setAcceptedMouseButtons(0); // Qt::NoButton
	_appRemoveButton->setPos(sceneRect.width()/2 - _appRemoveButton->size().width()/2, 0); // position to the top center
	_appRemoveButton->setZValue(999999998); // 1 less than polygon arrow but always higher than apps
	addItem(_appRemoveButton);




	/**
	  Create base widget for wall partitioning.
	  */
	_rootLayoutWidget = new SAGENextLayoutWidget("ROOT", 0, _settings);
	_rootLayoutWidget->setRectangle(sceneRect);

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

void SAGENextScene::addItemOnTheLayout(BaseWidget *bw, const QPointF &scenepos) {
	Q_ASSERT(_rootLayoutWidget);
	_rootLayoutWidget->addItem(bw, scenepos);
}

void SAGENextScene::prepareClosing() {
	if (_closeFlag) {
		// close UiServer so that all the shared pointers can be deleted first
		if (_uiserver) {
			qDebug() << "Scene is deleting UiServer";
			delete _uiserver;
		}

		// don't do this here
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
//			bw->fadeOutClose();
			bw->close();
		}
	}
}

void SAGENextScene::saveSession() {
	QString sessionFilename = QDir::homePath() + "/.sagenext/";
	sessionFilename.append(QDateTime::currentDateTime().toString("hh:mm:ss_MM.dd.yyyy_")).append(".session");

	QFile sessionfile(sessionFilename);
	if(!sessionfile.open(QIODevice::ReadWrite)) {
		qCritical() << "SAGENextScene::saveSession() : couldn't open the file" << sessionFilename;
		return;
	}
	qWarning() << "\nSAGENextScene::saveSession() : save current layout to" << sessionFilename;

	QDataStream out(&sessionfile);

	//
	// save layout states
	//



	//
	// save widget states
	//
	foreach(QGraphicsItem *item, items()) {
		if (!item || item->type() < QGraphicsItem::UserType + 12 ) continue;
		BaseWidget *bw = static_cast<BaseWidget *>(item);
		if (!bw) continue;

		AppInfo *ai = bw->appInfo();

		// common
		out << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale();

		// video, image, pdf, plugin, web have filename
		if (ai->fileInfo().exists()) {
			qDebug() << "Scene::saveSession() : " << ai->mediaFilename();
			out << ai->mediaFilename();
		}
		else if (!ai->webUrl().isEmpty()) {
			qDebug() << "Scene::saveSession() : " << ai->webUrl().toString();
			out << ai->webUrl().toString();
		}
		// vnc doesn't have filename
		else {
			out << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();
		}

		///
		/// and application specific state can be followed
		///
	}
	sessionfile.flush();
	sessionfile.close();
}

/*
QDataStream out(&sFile);

foreach (QGraphicsItem *gi, scene()->items()) {
	if (!gi) continue;

	// only consider user application
	if (gi->type() < QGraphicsItem::UserType + 12) continue;

	BaseWidget *bw = static_cast<BaseWidget *>(gi);
	out << bw->pos() << bw->scale() << bw->zValue();
}
*/







SAGENextLayoutWidget::SAGENextLayoutWidget(const QString &pos, SAGENextLayoutWidget *parentWidget, const QSettings *s, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _settings(s)
    , _parentLayoutWidget(parentWidget)
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
	_tileButton = new SAGENextPixmapButton(":/resources/tile_btn_over.jpg", _settings->value("gui/iconwidth").toDouble(), "", this);
	_hButton = new SAGENextPixmapButton(":/resources/horizontal_divider_btn_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);
	_vButton = new SAGENextPixmapButton( ":/resources/vertical_divider_btn_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);

	connect(_tileButton, SIGNAL(clicked()), this, SLOT(toggleTile()));
	// horizontal button will divide the widget vertically
	connect(_hButton, SIGNAL(clicked()), this, SLOT(createHBar()));
	// vertical button will divide the widget horizontally
	connect(_vButton, SIGNAL(clicked()), this, SLOT(createVBar()));

//	_buttonGrp = new QGraphicsItemGroup(this);
//	_buttonGrp->addToGroup(_vButton);
//	_buttonGrp->addToGroup(_hButton);

	if (parentWidget) {
		_xButton = new SAGENextPixmapButton(":/resources/close_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);
		connect(_xButton, SIGNAL(clicked()), _parentLayoutWidget, SLOT(deleteChildPartitions()));
//		_buttonGrp->addToGroup(_xButton);
	}
	else {

	}
//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
//	setMinimumSize(r.size());
}

SAGENextLayoutWidget::~SAGENextLayoutWidget() {

}

void SAGENextLayoutWidget::setRectangle(const QRectF &r) {
	resize(r.size());
	setPos(r.topLeft()); // partitionRect.topLeft is in it's parent's coordinate
}

void SAGENextLayoutWidget::addItem(BaseWidget *bw, const QPointF &scenepos /* = 30,30*/) {
	/**
	  If _bar exist, that means this layoutWidget is just a container for child layoutwidgets.
	  So this layoutWidget can't have any baseWidget as a child.
	  */
	if (_bar) {
		if (_bar->orientation() == Qt::Horizontal) {
			if ( _topWidget->rect().contains( _topWidget->mapFromScene(scenepos))) {
				_topWidget->addItem(bw, scenepos);
			}
			else {
				_bottomWidget->addItem(bw, scenepos);
			}
		}
		else {
			if (_leftWidget->rect().contains(_leftWidget->mapFromScene(scenepos))) {
				_leftWidget->addItem(bw, scenepos);
			}
			else {
				_rightWidget->addItem(bw, scenepos);
			}
		}
	}
	/**
	  BaseWidgets can be added to me
	  */
	else {
		/**
		  if the item already has a parent it is first removed from the previous parent
		  This implicitly adds this item to the scene of the parent.

		  QGraphicsObject::parentChanged() will be emitted
		  */
		bw->setParentItem(this);
		bw->setPos( mapFromScene(scenepos) );
	}
}

/**
  Caller's child items will be reparented to the newParent
  */
void SAGENextLayoutWidget::reparentWidgets(SAGENextLayoutWidget *newParent) {
	if (_bar) {
		if (_bar->orientation() == Qt::Horizontal) {
			_topWidget->reparentWidgets(newParent);
			_bottomWidget->reparentWidgets(newParent);
		}
		else {
			_leftWidget->reparentWidgets(newParent);
			_rightWidget->reparentWidgets(newParent);
		}
	}
	else {
		foreach(QGraphicsItem *item, childItems()) {
			// exclude PartitionBar
			// exclude PixmapButton
			if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item == _leftWidget || item == _rightWidget || item == _topWidget || item == _bottomWidget) continue;

			//
			// this item's pos() which is in this layoutWidget's coordinate to newParent's coordinate
			//
			QPointF newPos = mapToItem(newParent, item->pos());

			// reparent
			item->setParentItem(newParent);

			item->setPos(newPos);
		}
	}
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

	QSizeF deltaSize = e->newSize() - e->oldSize();

	//
	// upon resizing, resize my child layout widgets as well
	// Since I have the bar, I dont have any base widget as a child
	//
	if (_bar) {
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

	//
	// no _bar, so this layoutWidget contains child basewidgets
	//
	else {
		// If growing, do nothing
		// If shrinking, move child BaseWidgets accordingly

		foreach(QGraphicsItem *item, childItems()) {
			if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item == _leftWidget || item == _rightWidget || item == _topWidget || item == _bottomWidget) {
	//			qDebug() << "createChildlayout skipping myself, buttons and bar";
				continue;
			}

			QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
			Q_ASSERT(widget);

			if (!rect().contains( widget->geometry() )) {
				// this widget need to be moved

			}
		}
	}
}

void SAGENextLayoutWidget::createChildPartitions(Qt::Orientation dividerOrientation) {
//	QGraphicsLinearLayout *linear = new QGraphicsLinearLayout(orientation);
//	linear->setContentsMargins(0, 0, 0, 0);

	QRectF first;
	QRectF second;
	SAGENextLayoutWidget *firstlayoutchild = 0;
	SAGENextLayoutWidget *secondlayoutchild = 0;
//	QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Fixed);
//	linear->setSizePolicy(sp);

	// create PartitionBar child item
	_bar = new PartitionBar(dividerOrientation, this, this);

	QRectF br = boundingRect();
	if (dividerOrientation == Qt::Horizontal) {
		//
		// bar is horizontal, partition is Top and Bottom (same X pos)
		//
		first = QRectF( 0, 0,             br.width(), br.height()/2);
		second = QRectF(0, br.height()/2, br.width(), br.height()/2);

		_topWidget = new SAGENextLayoutWidget("top",  this, _settings, this);
		_bottomWidget = new SAGENextLayoutWidget("bottom",  this, _settings, this);

		firstlayoutchild = _topWidget;
		secondlayoutchild = _bottomWidget;
	}
	else {
		//
		// bar is vertical, partition is Left and Right (same Y pos)
		//
		first = QRectF(  0,           0, br.width()/2, br.height());
		second = QRectF(br.width()/2, 0, br.width()/2, br.height());

		_leftWidget = new SAGENextLayoutWidget("left",  this, _settings, this);
		_rightWidget = new SAGENextLayoutWidget("right",  this, _settings, this);

		firstlayoutchild = _leftWidget;
		secondlayoutchild = _rightWidget;
	}

	//
	// if child widget is resized, then adjust my bar pos and length
	//
	connect(firstlayoutchild, SIGNAL(resized()), this, SLOT(adjustBar()));


	// will invoke resize()
	firstlayoutchild->setRectangle(first);
	secondlayoutchild->setRectangle(second);
//	qDebug() << "createchild result" << rect() << firstlayoutchild->geometry() << secondlayoutchild->geometry();


	//
	// reparent child items to appropriate widget. I shouldn't have any child baseWidgets at this point
	//
	foreach(QGraphicsItem *item, childItems()) {
		if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item == _leftWidget || item == _rightWidget || item == _topWidget || item == _bottomWidget) {
//			qDebug() << "createChildlayout skipping myself, buttons and bar";
			continue;
		}

		QPointF newPos;
		QRectF intersectedWithFirst = first & mapRectFromItem(item, item->boundingRect());
		if ( intersectedWithFirst.size().width() * intersectedWithFirst.size().height() >= 0.25 * item->boundingRect().size().width() * item->boundingRect().size().height()) {
			newPos = mapToItem(firstlayoutchild, item->pos());
			item->setParentItem(firstlayoutchild);
		}
		else {
			newPos = mapToItem(secondlayoutchild, item->pos());
			item->setParentItem(secondlayoutchild);
		}
		item->setPos(newPos);
	}


	//
	// hides my buttons. This widget will just hold child widget
	//
	_tileButton->hide();
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
	Q_ASSERT(_bar);

	//
	// reparent all basewidgets of my child layouts to me
	//
	if ( _bar->orientation() == Qt::Horizontal) {
		_topWidget->reparentWidgets(this);
		_bottomWidget->reparentWidgets(this);

		delete _topWidget;
		delete _bottomWidget;
	}
	else {
		_leftWidget->reparentWidgets(this);
		_rightWidget->reparentWidgets(this);

		delete _leftWidget;
		delete _rightWidget;
	}
	delete _bar;
	_bar = 0; // do I need this?

	if (_isTileOn) {
		doTile();
	}

	//
	// provide interactivity after everything is done
	//
	_tileButton->show();
	_vButton->show();
	_hButton->show();
	if (_xButton) _xButton->show();
}

void SAGENextLayoutWidget::doTile() {
	if (_bar) return;

	foreach(QGraphicsItem *item, childItems()) {
		// exclude PartitionBar
		// exclude PixmapButton
		if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item == _leftWidget || item == _rightWidget || item == _topWidget || item == _bottomWidget ) continue;

	}
}

void SAGENextLayoutWidget::toggleTile() {
	if (_isTileOn) {
		_isTileOn = false;
		// do nothing
	}
	else {
		_isTileOn = true;
		doTile();
	}
}

void SAGENextLayoutWidget::setButtonPos() {
//	qDebug() << "attachButton" << boundingRect() << geometry();
	_tileButton->setPos(size().width() - _tileButton->size().width() - 10, size().height()/2);
	_vButton->setPos(_tileButton->geometry().x(), _tileButton->geometry().bottom() + 5);
	_hButton->setPos(_vButton->geometry().x(),  _vButton->geometry().bottom() + 5);
	if (_xButton) {
		_xButton->setPos(_hButton->geometry().x(),  _hButton->geometry().bottom() + 5);
	}
}

void SAGENextLayoutWidget::saveCurrentSession() {

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

	setAcceptedMouseButtons(Qt::LeftButton | Qt::NoButton);

	QPen pen;
	pen.setWidth(12);
	pen.setStyle(Qt::DashLine);
	pen.setColor(QColor(Qt::lightGray));
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
