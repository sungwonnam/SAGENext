#include "sagenextscene.h"
#include <QGraphicsItem>

#include "sagenextlauncher.h"
#include "common/commonitem.h"
#include "applications/base/basewidget.h"
#include "applications/base/appinfo.h"
#include "system/resourcemonitor.h"
#include "uiserver/uiserver.h"

SN_TheScene::SN_TheScene(const QRectF &sceneRect, const QSettings *s, QObject *parent)
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
	_closeButton = new SN_PixmapButton(closeIcon, _settings->value("gui/iconwidth").toDouble());
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
	_appRemoveButton = new SN_PixmapButton(":/resources/default_button_up.png", QSize(512, 32), "Remove");
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
	_rootLayoutWidget = new SN_LayoutWidget("ROOT", 0, _settings);
	_rootLayoutWidget->setRectangle(sceneRect);

	/**
	  Child widgets of _rootLayoutWidget, which are sagenext applications, will be added to the scene automatically.
      refer QGraphicsItem::setParentItem()
	  So, do NOT explicitly additem(sagenext application) !!
	  */
	addItem(_rootLayoutWidget);
}

bool SN_TheScene::isOnAppRemoveButton(const QPointF &scenepos) {
	if (!_appRemoveButton) return false;

	return _appRemoveButton->geometry().contains(scenepos);
}

void SN_TheScene::addItemOnTheLayout(SN_BaseWidget *bw, const QPointF &scenepos) {
	if(_rootLayoutWidget) {
		_rootLayoutWidget->addItem(bw, scenepos);
	}
	else {
		addItem(bw);
		bw->setPos(scenepos);
	}
}

void SN_TheScene::prepareClosing() {
	if (_closeFlag) {
		// close UiServer so that all the shared pointers can be deleted first
		if (_uiserver) {
			qDebug() << "\n[ Scene is deleting UiServer ]\n";
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

SN_TheScene::~SN_TheScene() {
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

			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			qDebug() << "Scene is deleting an application id" << bw->globalAppId();
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
	qDebug() << "\n[ Scene is closing all viewports ]\n";
	foreach (QGraphicsView *view, views()) {
		view->close(); // WA_DeleteOnClose is set
	}
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void SN_TheScene::closeAllUserApp() {
	foreach (QGraphicsItem *item, items()) {
		if (!item ) continue;
		if (item->type() >= QGraphicsItem::UserType + 12) {
			// this is user application
			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			Q_ASSERT(bw);
//			bw->fadeOutClose();
			bw->close();
		}
	}
}

void SN_TheScene::saveSession() {
	QString sessionFilename = QDir::homePath() + "/.sagenext/sessions/";

	sessionFilename.append(QDateTime::currentDateTime().toString("hh.mm.ss_MM.dd.yyyy_"));

	QString screenshotFilename = sessionFilename;
	screenshotFilename.append(".jpg");

	sessionFilename.append(".session");

	QFile sessionfile(sessionFilename);
	if(!sessionfile.open(QIODevice::ReadWrite)) {
		qCritical() << "SN_TheScene::saveSession() : couldn't open the file" << sessionFilename;
		return;
	}
	qWarning() << "\nSN_TheScene::saveSession() : save current layout to" << sessionFilename;


	////
	// create snapshot
	////
	QPixmap screenshot(sceneRect().size().toSize());
	QPainter painter(&screenshot);
	render(&painter);
	screenshot.save(screenshotFilename, "jpg", 100);


	QDataStream out(&sessionfile);

	if (_rootLayoutWidget) {
		_rootLayoutWidget->saveSession(out);
	}
	else {
		//
		// just save widget states
		//
		foreach(QGraphicsItem *item, items()) {

			// only consider user application
			if (!item || item->type() < QGraphicsItem::UserType + 12 ) continue;

			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			if (!bw) continue;

			AppInfo *ai = bw->appInfo();

			// common
			out << QString("ITEM") << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale();

			// video, image, pdf, plugin, web have filename
			if (ai->fileInfo().exists()) {
				out << ai->mediaFilename();
				qDebug() << "SN_TheScene::saveSession() : " << QString("ITEM") << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->mediaFilename();
			}
			else if (!ai->webUrl().isEmpty()) {
				out << ai->webUrl().toString();
				qDebug() << "SN_TheScene::saveSession() : " << QString("ITEM") << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->webUrl().toString();
			}
			// vnc doesn't have filename
			else {
				out << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();
				qDebug() << "SN_TheScene::saveSession() : " << QString("ITEM") << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();
			}

			///
			/// and application specific state can be followed
			///
		}
	}
	sessionfile.flush();
	sessionfile.close();
}

void SN_TheScene::loadSession(QDataStream &in, SN_Launcher *launcher) {
	if (_rootLayoutWidget) {
		_rootLayoutWidget->loadSession(in, launcher);
	}
	else {
		QString header; /// ITEM
		int mtype;
		QPointF scenepos;
		QSizeF size;
		qreal scale;
		while (!in.atEnd()) {
			in >> header;
			if (header != "ITEM") continue;

			in >> mtype >> scenepos >> size >> scale;
	//		qDebug() << "\tentry : " << mtype << scenepos << size << scale;

			QString file;
			QString user;
			QString pass;
			QString srcaddr;

			SN_BaseWidget *bw = 0;

			if (mtype == SAGENext::MEDIA_TYPE_VNC) {
				in >> srcaddr >> user >> pass;
				bw = launcher->launch(user, pass, 0, srcaddr, 10, scenepos);
			}
			else {
				in >> file;
				bw = launcher->launch(mtype, file, scenepos);
				if (mtype == SAGENext::MEDIA_TYPE_LOCAL_VIDEO || mtype == SAGENext::MEDIA_TYPE_VIDEO) {
					::usleep(300 * 1000);
				}
			}
			if (!bw) {
				qDebug() << "SN_TheScene::loadSession() : Error : can't launch this entry from the session file" << mtype << file << srcaddr << user << pass << scenepos << size << scale;
				continue;
			}

			bw->resize(size);
			bw->setScale(scale);
		}
	}
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







SN_LayoutWidget::SN_LayoutWidget(const QString &pos, SN_LayoutWidget *parentWidget, const QSettings *s, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , _settings(s)
    , _parentLayoutWidget(parentWidget)
//    , _leftWidget(0)
//    , _rightWidget(0)
//    , _topWidget(0)
//    , _bottomWidget(0)
	, _firstChildLayout(0)
	, _secondChildLayout(0)
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
	_tileButton = new SN_PixmapButton(":/resources/tile_btn_over.jpg", _settings->value("gui/iconwidth").toDouble(), "", this);
	_hButton = new SN_PixmapButton(":/resources/horizontal_divider_btn_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);
	_vButton = new SN_PixmapButton( ":/resources/vertical_divider_btn_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);

	connect(_tileButton, SIGNAL(clicked()), this, SLOT(toggleTile()));
	// horizontal button will divide the widget vertically
	connect(_hButton, SIGNAL(clicked()), this, SLOT(createHBar()));
	// vertical button will divide the widget horizontally
	connect(_vButton, SIGNAL(clicked()), this, SLOT(createVBar()));

//	_buttonGrp = new QGraphicsItemGroup(this);
//	_buttonGrp->addToGroup(_vButton);
//	_buttonGrp->addToGroup(_hButton);

	if (parentWidget) {
		_xButton = new SN_PixmapButton(":/resources/close_over.png", _settings->value("gui/iconwidth").toDouble(), "", this);
		connect(_xButton, SIGNAL(clicked()), _parentLayoutWidget, SLOT(deleteChildPartitions()));
//		_buttonGrp->addToGroup(_xButton);
	}
	else {

	}
//	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
//	setMinimumSize(r.size());
}

SN_LayoutWidget::~SN_LayoutWidget() {

}

void SN_LayoutWidget::setRectangle(const QRectF &r) {
	resize(r.size());

	// my (0,0) is my boundingRect().center()
//	setPos(r.center());

	// my (0,0) is my boundingRect().topLeft()
	setPos(r.topLeft());
}

void SN_LayoutWidget::addItem(SN_BaseWidget *bw, const QPointF &pos /* = 30,30*/) {
	/**
	  If _bar exist, that means this layoutWidget is just a container for child layoutwidgets.
	  So this layoutWidget can't have any baseWidget as a child.
	  */
	if (_bar) {
		if ( _firstChildLayout->rect().contains( _firstChildLayout->mapFromScene(pos))) {
			_firstChildLayout->addItem(bw, pos);
		}
		else {
			_secondChildLayout->addItem(bw, pos);
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
		bw->setPos(pos);
	}
}

/**
  Caller's child items will be reparented to the newParent
  */
void SN_LayoutWidget::reparentWidgets(SN_LayoutWidget *newParent) {
	if (_bar) {
		_firstChildLayout->reparentWidgets(newParent);
		_secondChildLayout->reparentWidgets(newParent);
	}
	else {
		foreach(QGraphicsItem *item, childItems()) {
			// exclude PartitionBar
			// exclude PixmapButton
			if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item==_firstChildLayout || item==_secondChildLayout) continue;

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


void SN_LayoutWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	QPen pen;
	pen.setColor(QColor(Qt::white));
	painter->setPen(pen);
	painter->drawRect(boundingRect());
}

void SN_LayoutWidget::resizeEvent(QGraphicsSceneResizeEvent *e) {
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

		/*******
		  ****
		 *****  Below is when (0, 0) is center

		QRectF br = boundingRect();
		QRectF first, second;

		if (_bar->orientation() == Qt::Horizontal) {
			//
			// bar is horizontal so my children is at TOP and BOTTOM
			//
			first = QRectF(br.topLeft(), QSizeF(br.width(), _bar->line().y1() - br.top()));
			second = QRectF(br.left(), _bar->line().y1(), br.width(), br.bottom() - _bar->line().y1());
		}
		else {
			first = QRectF(br.topLeft(), QSizeF(_bar->line().x1() - br.left(), br.height()));
			second = QRectF(_bar->line().x1(), br.top(), br.right() - _bar->line().x1(), br.height());
		}
		_firstChildLayout->setRectangle(first);
		_secondChildLayout->setRectangle(second);
		******
		*****/

		if (_bar->orientation() == Qt::Horizontal) {
			//
			// bar is horizontal so my children is at TOP and BOTTOM
			//
			_firstChildLayout->setPos(0, 0);

			if (deltaSize.height() == 0) {
				//
				// I'm resized horizontally.  -> only width changes for my childs
				//
				_firstChildLayout->resize(   e->newSize().width() ,    _firstChildLayout->size().height());
				_secondChildLayout->resize(e->newSize().width() , _secondChildLayout->size().height());

				_secondChildLayout->setPos(0, _secondChildLayout->geometry().y());
			}
			else if (deltaSize.width() == 0) {
				//
				// resized vertically, top and bottom child widgets will share height delta
				//
				_firstChildLayout->resize(      _firstChildLayout->size().width() ,    _firstChildLayout->size().height() + deltaSize.height() / 2.0);
				_secondChildLayout->resize(_secondChildLayout->size().width() , _secondChildLayout->size().height() + deltaSize.height() / 2.0);

				_secondChildLayout->setPos(0, _secondChildLayout->geometry().y() + deltaSize.height()/2.0);
			}
		}
		else {
			//
			// the bar is vertical so my children is at LEFT and RIGHT
			//
			_firstChildLayout->setPos(0,0);

			if (deltaSize.height() == 0) {
				// I'm resized horizontally. -> no height changes in my children
				_firstChildLayout->resize(_firstChildLayout->size().width() + deltaSize.width()/2.0,  _firstChildLayout->size().height());
				_secondChildLayout->resize(_secondChildLayout->size().width() + deltaSize.width()/2.0, _secondChildLayout->size().height());

				_secondChildLayout->setPos(_secondChildLayout->geometry().x() + deltaSize.width()/2.0 , 0);
			}
			else if (deltaSize.width() == 0) {
				// I'm resized vertically -> no width changes in my children
				_firstChildLayout->resize( _firstChildLayout->size().width(),  e->newSize().height());
				_secondChildLayout->resize(_secondChildLayout->size().width(), e->newSize().height());

				_secondChildLayout->setPos(_secondChildLayout->geometry().x(), 0);
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
			if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton ||  item==_firstChildLayout || item==_secondChildLayout) {
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

void SN_LayoutWidget::createChildPartitions(Qt::Orientation dividerOrientation) {
	QRectF first;
	QRectF second;

	QRectF br = boundingRect();
	if (dividerOrientation == Qt::Horizontal) {
		//
		// bar is horizontal, partition is Top and Bottom (same X pos)
		//

		/******* (0,0) is center
		first = QRectF(br.topLeft(), QSizeF(br.width(), br.height()/2));
		second = QRectF(br.left(), 0, br.width(), br.height()/2);
		********/

		first = QRectF( 0, 0,             br.width(), br.height()/2);
		second = QRectF(0, br.height()/2, br.width(), br.height()/2);
	}
	else {
		//
		// bar is vertical, partition is Left and Right (same Y pos)
		//

		/********* (0,0) is center
		first = QRectF(br.topLeft(), QSizeF(br.width()/2, br.height()));
		second = QRectF(0, br.top(), br.width()/2, br.height());
		*********/

		first = QRectF(  0,           0, br.width()/2, br.height());
		second = QRectF(br.width()/2, 0, br.width()/2, br.height());

	}

	createChildPartitions(dividerOrientation, first, second);
}

void SN_LayoutWidget::createChildPartitions(Qt::Orientation dividerOrientation, const QRectF &first, const QRectF &second) {
	// create PartitionBar child item
	_bar = new SN_WallPartitionBar(dividerOrientation, this, this);

	_firstChildLayout = new SN_LayoutWidget("first", this, _settings, this);
	_secondChildLayout = new SN_LayoutWidget("second", this, _settings, this);

	//
	// if child widget is resized, then adjust my bar pos and length
	//
	connect(_firstChildLayout, SIGNAL(resized()), this, SLOT(adjustBar()));


	// will invoke resize()
	_firstChildLayout->setRectangle(first);
	_secondChildLayout->setRectangle(second);
//	qDebug() << "createchild result" << rect() << firstlayoutchild->geometry() << secondlayoutchild->geometry();


	//
	// reparent child items to appropriate widget. I shouldn't have any child baseWidgets at this point
	//
	foreach(QGraphicsItem *item, childItems()) {
		if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton ||  item==_firstChildLayout || item==_secondChildLayout ) {
//			qDebug() << "createChildlayout skipping myself, buttons and bar";
			continue;
		}

		QPointF newPos;
		QRectF intersectedWithFirst = first & mapRectFromItem(item, item->boundingRect());
		if ( intersectedWithFirst.size().width() * intersectedWithFirst.size().height() >= 0.25 * item->boundingRect().size().width() * item->boundingRect().size().height()) {
			newPos = mapToItem(_firstChildLayout, item->pos());
			item->setParentItem(_firstChildLayout);
		}
		else {
			newPos = mapToItem(_secondChildLayout, item->pos());
			item->setParentItem(_secondChildLayout);
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



void SN_LayoutWidget::adjustBar() {
	Q_ASSERT(_bar);

	/*******
	  ***** (0,0) is center
	if ( _bar->orientation() == Qt::Horizontal ) {
		// my children is top and bottom widgets
		qreal newY = boundingRect().top() + _firstChildLayout->size().height();
		QLineF line(boundingRect().left(), newY, boundingRect().right(), newY);
		_bar->setLine(line);
	}
	else {
		// my children is left and right widgets
		qreal newX = boundingRect().left() + _firstChildLayout->size().width();
		QLineF line(newX, boundingRect().top(), newX, boundingRect().bottom());
		_bar->setLine(line);
	}
	********
	*****/

	if ( _bar->orientation() == Qt::Horizontal ) {
		// my children is top and bottom widgets
		QPointF bottomLeft = _firstChildLayout->geometry().bottomLeft();
		QPointF bottomRight = _firstChildLayout->geometry().bottomRight();

		_bar->setLine(bottomLeft.x(), bottomLeft.y(), bottomRight.x(), bottomRight.y());
	}
	else {
		// my children is left and right widgets
		QPointF topRight = _firstChildLayout->geometry().topRight();
		QPointF bottomRight = _firstChildLayout->geometry().bottomRight();

		_bar->setLine(topRight.x(), topRight.y(), bottomRight.x(), bottomRight.y());
	}
}

void SN_LayoutWidget::deleteChildPartitions() {
	Q_ASSERT(_bar);

	//
	// reparent all basewidgets of my child layouts to me
	//
	_firstChildLayout->reparentWidgets(this);
	_secondChildLayout->reparentWidgets(this);
	delete _firstChildLayout;
	delete _secondChildLayout;

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

void SN_LayoutWidget::doTile() {
	if (_bar) return;

	foreach(QGraphicsItem *item, childItems()) {
		// exclude PartitionBar
		// exclude PixmapButton
		if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item==_firstChildLayout || item==_secondChildLayout )
			continue;

	}
}

void SN_LayoutWidget::toggleTile() {
	if (_isTileOn) {
		_isTileOn = false;
		// do nothing
	}
	else {
		_isTileOn = true;
		doTile();
	}
}

void SN_LayoutWidget::setButtonPos() {
//	qDebug() << "attachButton" << boundingRect() << geometry();

	/******* (0,0) center
	_tileButton->setPos(boundingRect().topRight().x() - _tileButton->size().width() - 10, 0);
	*****/

	_tileButton->setPos(size().width() - _tileButton->size().width() - 10, size().height()/2);

	_vButton->setPos(_tileButton->geometry().x(), _tileButton->geometry().bottom() + 5);
	_hButton->setPos(_vButton->geometry().x(),  _vButton->geometry().bottom() + 5);
	if (_xButton) {
		_xButton->setPos(_hButton->geometry().x(),  _hButton->geometry().bottom() + 5);
	}
}

void SN_LayoutWidget::saveSession(QDataStream &out) {

	if (_bar) {
		out << QString("LAYOUT");
		if (_bar->orientation() == Qt::Horizontal)
			out << 0;
		else
			out << 1;

		out << _firstChildLayout->pos() << _firstChildLayout->size() << _secondChildLayout->pos() << _secondChildLayout->size();
		_firstChildLayout->saveSession(out);
		_secondChildLayout->saveSession(out);
	}
	else {
		foreach(QGraphicsItem *item, childItems()) {
			if (item == _bar || item == _tileButton || item == _hButton || item == _vButton || item == _xButton || item == _firstChildLayout || item == _secondChildLayout) {
				continue;
			}

			// only consider user application
			if (item->type() < QGraphicsItem::UserType + 12 ) continue;

			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			if (!bw) continue;

			AppInfo *ai = bw->appInfo();
			out << QString("ITEM");
			//
			// item's pos() is saved. not the scenePos()
			//
			out << (int)ai->mediaType() << bw->pos() << bw->size() << bw->scale();

			// video, image, pdf, plugin, web have filename
			if (ai->fileInfo().exists()) {
				out << ai->mediaFilename();
				qDebug() << "SN_LayoutWidget::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->mediaFilename();
			}
			else if (!ai->webUrl().isEmpty()) {
				out << ai->webUrl().toString();
				qDebug() << "SN_LayoutWidget::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->webUrl().toString();
			}
			// vnc doesn't have filename
			else {
				out << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();
				qDebug() << "SN_LayoutWidget::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();
			}
		}
		out << QString("RETURN");
	}
}

void SN_LayoutWidget::loadSession(QDataStream &in, SN_Launcher *launcher) {
	QString header;
	in >> header;

	if (header == "LAYOUT") {
		int orien;
		in >> orien;

		QPointF pos1, pos2;
		QSizeF size1, size2;
		in >> pos1 >> size1 >> pos2 >> size2;

		if (orien==0)
			createChildPartitions(Qt::Horizontal, QRectF(pos1, size1), QRectF(pos2, size2));
		else
			createChildPartitions(Qt::Vertical, QRectF(pos1, size1), QRectF(pos2, size2));

		_firstChildLayout->loadSession(in, launcher);
		_secondChildLayout->loadSession(in, launcher);
	}
	else if (header == "ITEM") {
		int mtype;
		QPointF pos;
		QSizeF size;
		qreal scale;


		in >> mtype >> pos >> size >> scale;
		//qDebug() << "\tentry : " << mtype << scenepos << size << scale;

		QString file;
		QString user;
		QString pass;
		QString srcaddr;

		SN_BaseWidget *bw = 0;

		if (mtype == SAGENext::MEDIA_TYPE_VNC) {
			in >> srcaddr >> user >> pass;
			bw = launcher->launch(user, pass, 0, srcaddr, 10, pos);
		}
		else {
			in >> file;
			bw = launcher->launch(mtype, file, pos);
			if (mtype == SAGENext::MEDIA_TYPE_LOCAL_VIDEO || mtype == SAGENext::MEDIA_TYPE_VIDEO) {
				::usleep(300 * 1000);
			}
		}
		if (!bw) {
			qDebug() << "Error : can't launch this entry from the session file" << mtype << file << srcaddr << user << pass << pos << size << scale;
		}
		else {
			bw->resize(size);
			bw->setScale(scale);
		}

		loadSession(in, launcher);
	}
	else if (header == "RETURN") {
		qDebug("%s::%s() : RETURN", metaObject()->className(), __FUNCTION__);
	}
	else {
		qDebug("%s::%s() : Unknown entry", metaObject()->className(), __FUNCTION__);
	}
}

//QRectF SN_LayoutWidget::boundingRect() const {
//	return QRectF( -1 * size().width()/2 , -1 * size().height()/2 , size().width(), size().height());
//}



















SN_WallPartitionBar::SN_WallPartitionBar(Qt::Orientation ori, SN_LayoutWidget *owner, QGraphicsItem *parent)
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
