#include "sn_sharedpointer.h"

#include "sn_layoutwidget.h"
#include "../applications/base/basewidget.h"
#include "../sagenextscene.h"

#include "sn_drawingwidget.h"

SelectionRectangle::SelectionRectangle(QGraphicsItem *parent)
	: QGraphicsWidget(parent)
{
	
}


SN_PolygonArrowPointer::SN_PolygonArrowPointer(const quint32 uicid, const QSettings *s, SN_TheScene *scene, const QString &name, const QColor &c, QFile *scenarioFile, QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent)
    , _scene(scene)
    , _uiclientid(uicid)
    , _settings(s)
    , _textItem(0)
	, _color(c)
    , _basewidget(0)
    , _item(0)
	, _selectionRect(0)
	, _selectedApps(0)
	, _scenarioFile(scenarioFile)
	, _isDrawing(false)
	, _isErasing(false)
{
	///
	/// will this help?
	///
	//setCacheMode(QGraphicsItem::ItemCoordinateCache);

	QPolygonF p;
//	p << QPointF(0,0) << QPointF(60, 20) << QPointF(46, 34) << QPointF(71, 59) << QPointF(60, 70) << QPointF(35, 45) << QPointF(20, 60) << QPointF(0,0);
	p << QPointF(0,0) << QPointF(113,75) << QPointF(47, 67) << QPointF(35,125) << QPointF(0,0);

	QPen pen;
	pen.setWidth(4); // 4 pixel
	pen.setColor(Qt::white);
	setPen(pen);

	setBrush(QBrush(c));
	setPolygon(p);

	setAcceptedMouseButtons(0);

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setZValue(999999999); // the top most

	if (!name.isNull() && !name.isEmpty())
		setPointerName(name);
}

SN_PolygonArrowPointer::~SN_PolygonArrowPointer() {
	//
	// BaseWidget who registered for hoveraccept needs to know if a pointer is nolonger valid
	//
	foreach(SN_BaseWidget *bw, _scene->hoverAcceptingApps) {
		bw->removePointerFromPointerMap(this);
	}

	//
	// for Recording, it needs to know when the pointer had been unshared
	//
	if (_scenarioFile) {
		char buffer[64];
		sprintf(buffer, "%lld %d %u", QDateTime::currentMSecsSinceEpoch(), 11, _uiclientid);
		_scenarioFile->write(buffer);
		_scenarioFile->flush();
	}
}

void SN_PolygonArrowPointer::setErasing(bool b) {
	_isErasing = b;
	if (b) {
		if (!_isDrawing) {
			_isDrawing = true;
		}
	}
}

void SN_PolygonArrowPointer::setPointerName(const QString &text) {
//	_nameBg = new QGraphicsRectItem(this);

	_textItem = new QGraphicsSimpleTextItem(text, this);
	_textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
	_textItem->setFlag(QGraphicsItem::ItemIsMovable, false);

	QFont f;
	f.setStyleStrategy(QFont::OpenGLCompatible);
//	f.setStyleStrategy(QFont::ForceOutline);
	f.setPointSize(_settings->value("gui/pointerfontsize", 20).toInt());
	f.setBold(true);
	_textItem->setFont(f);

	_textItem->setBrush(Qt::white);
	_textItem->moveBy(60, boundingRect().height() - 30);
//	_nameBg->moveBy(60, boundingRect().height() - 15);

	_textItem->setPen(QColor(Qt::black));
}

void SN_PolygonArrowPointer::pointerMove(const QPointF &_scenePos, Qt::MouseButtons btnFlags, Qt::KeyboardModifier modifier) {
    qreal deltax = _scenePos.x() - scenePos().x();
    qreal deltay = _scenePos.y() - scenePos().y();


	//
    // move pointer itself
    // Sets the position of the item to pos, which is in parent coordinates. For items with no parent, pos is in scene coordinates.
	//
    setPos(_scenePos);


	////////////////////////////
	//
	// Record pointer move (2)
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		int button = 0;
		if (btnFlags & Qt::LeftButton) button = 1;
		else if (btnFlags & Qt::RightButton) button = 2;
		sprintf(record, "%lld %d %u %d %d %d\n", QDateTime::currentMSecsSinceEpoch(), 2, _uiclientid, _scenePos.toPoint().x(), _scenePos.toPoint().y(), button);
		_scenarioFile->write(record);
	}
	//////////////////////////////


	

	if (btnFlags & Qt::NoButton) {
		//
		// iterate over items in the container maintained by the scene
		// if pointer is on one of them
		// set hover flag for the item
		// else
		// unset hover flag
		//
		QList<QGraphicsItem *> collidingapps = _scene->collidingItems(this);
		SN_BaseWidget *firstUnderPointer = 0;
		for (int i=collidingapps.size()-1; i>=0; i--) {
			QGraphicsItem *item = collidingapps.at(i);
			if (item->type() < QGraphicsItem::UserType + 12) continue;
			firstUnderPointer = static_cast<SN_BaseWidget *>(item);
			break;
		}
		foreach(SN_BaseWidget *bw, _scene->hoverAcceptingApps) {
			if (!bw) {
				_scene->hoverAcceptingApps.removeOne(bw);
				continue;
			}
	
			if (bw == firstUnderPointer && bw->contains(bw->mapFromScene(_scenePos)) ) {
				bw->handlePointerHover(this, bw->mapFromScene(_scenePos), true);
			}
			else {
				bw->handlePointerHover(this, QPointF(), false);
			}
		}
	}

	//
    // LEFT button mouse dragging
	//
    else if ( btnFlags & Qt::LeftButton ) {
		
		SN_DrawingWidget *canvas = _scene->drawingCanvas();
		if (_isDrawing && canvas) {
			const QRectF &r = QRectF(canvas->mapFromScene(_scenePos), QSizeF(32,32));
			if (_isErasing) {
				canvas->drawEllipse(r, QColor(0,0,0,0), true);
			}
			else {
				canvas->drawEllipse(r, _color);
			}
			return;
		}

		//
        // Because of pointerPress, appUnderPointer has already been set at this point
        //
        if (_basewidget) {
			_basewidget->handlePointerDrag(this, _scenePos, deltax, deltay, Qt::LeftButton, modifier);
        }

		//
		// The PartitionBar item for instance
		//
		else if (_item) {
//			QGraphicsLineItem *l = qgraphicsitem_cast<QGraphicsLineItem *>(_item);
			SN_WallPartitionBar *bar = dynamic_cast<SN_WallPartitionBar *>(_item);
			if (bar) {
//				qDebug() << "pointerMove bar" << deltax << deltay;
				if ( bar->orientation() == Qt::Horizontal) {
					//
					// bar moves only up or down direction (y axis)

					SN_LayoutWidget *top = bar->ownerNode()->firstChildLayout();
					SN_LayoutWidget *bottom = bar->ownerNode()->secondChildLayout();
					top->resize(      top->size().width(),    top->size().height() + deltay);
					bottom->resize(bottom->size().width(), bottom->size().height() - deltay);

					// below is when top-left is 0,0
					bottom->moveBy(0, deltay);

					/*********
					  ** when the SN_LayoutWidget's 0,0 is the center */
//					top->moveBy(0, deltay/2);
//					bottom->moveBy(0, deltay/2);
				}
				else {
					// bar moves only left or right (x axis)

					SN_LayoutWidget *left = bar->ownerNode()->firstChildLayout();
					SN_LayoutWidget *right = bar->ownerNode()->secondChildLayout();
					left->resize(  left->size().width() + deltax,  left->size().height());
					right->resize(right->size().width() - deltax, right->size().height());

					// below is wehn top-left is 0,0
					right->moveBy(deltax, 0);

					/*********
					  ** when the SN_LayoutWidget's 0,0 is the center
					  */
//					left->moveBy(deltax/2, 0);
//					right->moveBy(deltax/2, 0);
				}
			}
		}
		else {
			// do nothing
		}

        // Why not just send mouse event ??
        // Because Having multiple users simultaneously do mouse draggin will confuse the system.
		/*
           I shouldn't simulate mouse move event like this..
           Because event keeps recording last known mouse event position
           If there are multiple virtual mouses, event->lastPos keeps garbled.
         */
        /*
        QMouseEvent mouseDragging(QMouseEvent::MouseMove, view->mapFromScene(_scenePos), Qt::LeftButton, btnFlags, modifier);
        if ( ! QApplication::sendEvent(view->viewport(), &mouseDragging) ) {
            qDebug() << "failed to send mouseMove event with left button";
        }
        */
    }

	//
	// RIGHT button mouse dragging
	//
	else if (btnFlags & Qt::RightButton) {
		// to be able to provide selection rectangle
		// mouse RIGHT PRESS must be sent from uiclient

		// resize selection rectangle
		// selection rectangle's bottom right must be on the scenePos
	}
}

/**
  Mouse right press won't be sent from uiclient
  */
void SN_PolygonArrowPointer::pointerPress(const QPointF &scenePos, Qt::MouseButton btn, Qt::KeyboardModifier modifier) {
	Q_UNUSED(modifier);

	//
	// Record
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		sprintf(record, "%lld %d %u %d %d %d\n",QDateTime::currentMSecsSinceEpoch(), 3, _uiclientid, scenePos.toPoint().x(), scenePos.toPoint().y(), btn);
		_scenarioFile->write(record);
	}


	// note that this doesn't consider window frame
    if (!setAppUnderPointer(scenePos)) {
        qDebug() << "PolygonArrow::pointerPress() : setAppUnderPointer failed";
    }
	else {
		Q_ASSERT(_basewidget || _item);
		if (_basewidget) _basewidget->handlePointerPress(this, _basewidget->mapFromScene(scenePos), btn);

		/**
//		qDebug() << "PolygonArrow::pointerPress() : got the app";
		if (btn == Qt::LeftButton) {
			if (_basewidget)
				_basewidget->setTopmost();
		}
		else if (btn == Qt::RightButton) {
			// this won't be delivered
			// do nothing

			// to be able to provide selection rectangle
			// mouse RIGHT PRESS must be sent from uiclient

			// Assume each pointer has the selection rectangle widget
			// selRect->setPos(scenePos);
		}
		**/
	}
}


void SN_PolygonArrowPointer::pointerRelease(const QPointF &scenePos, Qt::MouseButton button, Qt::KeyboardModifier modifier) {
	Q_UNUSED(modifier);

	//
	// Record
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		sprintf(record, "%lld %d %u %d %d %d\n", QDateTime::currentMSecsSinceEpoch(), 4, _uiclientid, scenePos.toPoint().x(), scenePos.toPoint().y(), button);
		_scenarioFile->write(record);
	}


	//
	// left mouse draggin with the app has finished
	//
	if (button == Qt::LeftButton) {
		if (_basewidget) {

			// I can move this app to SN_LayoutWidget that contains released scenePos
			// Always pass scene position
			_scene->addItemOnTheLayout(_basewidget, _basewidget->scenePos());


			// I can minimize this app if it's on the minimize rectangle


			// I can close this app if removeButton on the scene contains released scenePos
			if (_scene->isOnAppRemoveButton(scenePos)) {
				_scene->hoverAcceptingApps.removeAll(_basewidget);
				_basewidget->close();

//				if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
//					char record[64];
//					sprintf(record, "%lld %d \n", QDateTime::currentMSecsSinceEpoch(), 8, _uiclientid, scenePos.x(), scenePos.y(), button);
//					_scenarioFile->write(record);
//				}
			}
		}
	}

	//
	// right mouse dragging finished
	//
	else if (button == Qt::RightButton) {

		// selects all the basewidget intersect with selection rectangle
		// hide selection rectangle

	}
}


void SN_PolygonArrowPointer::pointerClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::KeyboardModifier modifier) {
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



	//
	// Record
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		sprintf(record, "%lld %d %u %d %d %d\n", QDateTime::currentMSecsSinceEpoch(), 5, _uiclientid, scenePos.toPoint().x(), scenePos.toPoint().y(), btn);
		_scenarioFile->write(record);
	}




	//
	// if there's app under pointer, toggle selected state with mouse Right click
	// and DON'T send mouse right click event
	//
	if ( btn == Qt::RightButton) {
		if (_basewidget) {
			if ( _basewidget->isSelected() ) {
				//
				// unselect widget
				//
				_basewidget->setSelected(false);
				// to be effective, turn off WA_OpaquePaintEvent or set setAutoFillBackground(true)
//				_basewidget->palette().setColor(QPalette::Window, QColor(100, 100, 100, 128));
			}
			else {
				//
				// select widget
				//
				_basewidget->setSelected(true);
//				_basewidget->palette().setColor(QPalette::Window, QColor(170, 170, 5, 164));
			}
		}
	}

	//
	// mouse left click event will be generated and sent to the viewport no matter what
	//
	else if (btn == Qt::LeftButton) {
		QGraphicsView *view = eventReceivingViewport(scenePos);
		if ( !view ) {
			qDebug() << "pointerClick: no view is available";
			return;
		}
		QPointF clickedViewPos = view->mapFromScene( scenePos );

//		qDebug() << "pointerClick() : LeftButton" << scenePos;

		QMouseEvent mpe(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, btn | Qt::NoButton, modifier);
		QMouseEvent mre(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, btn | Qt::NoButton, modifier);

		if ( ! QApplication::sendEvent(view->viewport(), &mpe) ) {
			// Upon receiving mousePressEvent, the item will become mouseGrabber if the item reimplement mousePressEvent
			qDebug("PolygonArrow::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
		}
		else {
			//qDebug() << "press delievered";
			if ( ! QApplication::sendEvent(view->viewport(), &mre) ) {
				qDebug("PolygonArrow::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
			}
			else {
				//qDebug() << "release delievered";
				// who should ungrabmouse() ??
				QGraphicsItem *mouseGrabberItem = view->scene()->mouseGrabberItem();
				if (mouseGrabberItem) mouseGrabberItem->ungrabMouse();
			}
		}
	}
}



void SN_PolygonArrowPointer::pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton btn, Qt::KeyboardModifier modifier) {

	//
	// Record
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		sprintf(record, "%lld %d %u %d %d %d\n", QDateTime::currentMSecsSinceEpoch(), 6, _uiclientid, scenePos.toPoint().x(), scenePos.toPoint().y(), btn);
		_scenarioFile->write(record);
	}


    if (!_basewidget) {
        qDebug("PolygonArrow::%s() : There's no app widget under this pointer", __FUNCTION__);
        return;
    }

    // Generate mouse event directly from here
    QGraphicsView *gview = eventReceivingViewport(scenePos);

    if (gview) {
        QMouseEvent dblClickEvent(QEvent::MouseButtonDblClick, gview->mapFromScene(scenePos), btn, btn | Qt::NoButton, modifier);
		QMouseEvent release(QEvent::MouseButtonRelease, gview->mapFromScene(scenePos), btn, btn | Qt::NoButton, modifier);

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



void SN_PolygonArrowPointer::pointerWheel(const QPointF &scenePos, int delta, Qt::KeyboardModifier) {

	//
	// Record
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		sprintf(record, "%lld %d %u %d %d %d\n", QDateTime::currentMSecsSinceEpoch(), 7, _uiclientid, scenePos.toPoint().x(), scenePos.toPoint().y(), delta);
		_scenarioFile->write(record);
	}


    // Generate mouse event directly from here
    QGraphicsView *gview = eventReceivingViewport(scenePos);

    if (gview) {
//		qDebug() << "pointerWheel" << delta;
		int _delta = 0;
		if ( delta > 0 )  _delta = 120;
		else if (delta <0) _delta = -120;

		QWheelEvent we(gview->mapFromScene(scenePos), /*gview->mapToGlobal(scenePos.toPoint()),*/ _delta, Qt::NoButton, Qt::NoModifier);

        if ( ! QApplication::sendEvent(gview->viewport(), &we) ) {
            qDebug("PolygonArrow::%s() : send wheelEvent failed", __FUNCTION__);
        }
        else {
            //qDebug() << "PolygonArrow wheel" << gview->mapFromScene(scenePos);
        }
    }
}

bool SN_PolygonArrowPointer::setAppUnderPointer(const QPointF scenePos) {
    Q_ASSERT(scene());
    QList<QGraphicsItem *> list = scene()->items(scenePos, Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
    //app = static_cast<BaseGraphicsWidget *>(scene()->itemAt(scenePosOfPointer, QTransform()));

    //qDebug() << "\nPolygonArrow::setAppUnderPointer() :" << list.size() << " items";
    foreach (QGraphicsItem *item, list) {
        if ( item == this ) continue;

		if ( item->acceptedMouseButtons() == 0 ) continue;

        if ( item->type() >= QGraphicsItem::UserType + 12) {
			//
			// User application (SN_BaseWidget)
			//
            _basewidget = static_cast<SN_BaseWidget *>(item);
            //qDebug("PolygonArrow::%s() : uiclientid %u, appid %llu", __FUNCTION__, uiclientid, app->globalAppId());
			_item = 0;
            return true;
        }
		else if (item->type() > QGraphicsItem::UserType) {
			//
			// custom item that inherits SN_BaseWidget. (SN_MediaBrowser is UserType + 11)
			// So this custom type should be less than UserType + 12
			//
		}
		else {
			//
			// regualar graphics items, All the SN_PixmapButton, SN_WallPartitionBar
			//
			_item = item;
//			qDebug() << _item;
			_basewidget = 0;
			return true;
		}
    }
    _basewidget = 0; // reset
	_item = 0;

    //qDebug("PolygonArrow::%s() : uiclientid %u, There's BaseGraphicsWidget type under pointer", __FUNCTION__, uiclientid);
    return false;
}

QGraphicsView * SN_PolygonArrowPointer::eventReceivingViewport(const QPointF scenePos) {
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

void SN_PolygonArrowPointer::pointerOperation(int opcode, const QPointF &scenepos, Qt::MouseButton btn, int delta, Qt::MouseButtons btnflags) {
	switch(opcode) {

	// MOVE
	case 0: {
		pointerMove(scenepos, btnflags);
		break;
	}
		// PRESS
	case 1: {
		pointerPress(scenepos, btn);
		break;
	}
		// RELEASE
	case 2: {
		pointerRelease(scenepos, btn);
		break;
	}

		// CLICK
	case 3: {
		pointerClick(scenepos, btn);
		break;
	}
		// DOUBLE CLICK
	case 4: {
		pointerDoubleClick(scenepos, btn);
		break;
	}

		// WHEEL
	case 5: {
		pointerWheel(scenepos, delta);
		break;
	}
	}
}

