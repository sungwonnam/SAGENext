#include "sn_sharedpointer.h"
#include "commonitem.h"
#include "sn_layoutwidget.h"
#include "../applications/base/basewidget.h"
#include "../applications/base/sn_priority.h"
#include "../sagenextscene.h"
#include "../uiserver/uimsgthread.h"

//#include "sn_drawingwidget.h"

SN_SelectionRectangle::SN_SelectionRectangle(QGraphicsItem *parent)
	: QGraphicsWidget(parent)
    , isMoving(false)
{
    QBrush brush(QColor(170,170,170, 128), Qt::Dense7Pattern);
    QPalette p;
    p.setBrush(QPalette::Window, brush);
    setPalette(p);

    setAutoFillBackground(true);

    setAcceptedMouseButtons(0);

    setContentsMargins(0,0,0,0);
    setWindowFrameMargins(0,0,0,0);
}




SN_PolygonArrowPointer::SN_PolygonArrowPointer(const quint32 uicid, UiMsgThread *msgthread, const QSettings *s, SN_TheScene *scene, const QString &name, const QColor &c, QFile *scenarioFile, QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent)
    , _scene(scene)
    , _uimsgthread(msgthread)
    , _uiclientid(uicid)
    , _settings(s)
    , _textItem(0)
	, _color(c)
    , _trapWidget(0)
    , _basewidget(0)
    , _graphicsItem(0)
    , _selectionRect(new SN_SelectionRectangle)
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

    _selectionRect->hide();
    _scene->addItem(_selectionRect);
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

    QColor bgcolor = QColor(Qt::darkGray);
    QColor fgcolor = QColor(Qt::white);

    if (text.compare("sungwon", Qt::CaseInsensitive) == 0) {
        bgcolor = QColor(Qt::lightGray);
        fgcolor = QColor(Qt::black);
    }


    QGraphicsRectItem *bg = new QGraphicsRectItem(this);
    QBrush b(bgcolor, Qt::Dense4Pattern);
    bg->setBrush(b);
    bg->setPen(QPen(Qt::NoPen));

	_textItem = new QGraphicsSimpleTextItem(text, bg);
	_textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
	_textItem->setFlag(QGraphicsItem::ItemIsMovable, false);

	QFont f;
	f.setStyleStrategy(QFont::OpenGLCompatible);
//	f.setStyleStrategy(QFont::ForceOutline);
	f.setPointSize(_settings->value("gui/pointerfontsize", 20).toInt());
	f.setBold(true);
	_textItem->setFont(f);

	_textItem->setBrush(fgcolor);
//	_textItem->moveBy(60, boundingRect().height() - 30);
//	_textItem->setPen(QColor(Qt::black));


    bg->setRect( _textItem->boundingRect().adjusted(-2, -2, 2, 2) );
    bg->moveBy(60, boundingRect().height() - 30);
}


void SN_PolygonArrowPointer::pointerMove(const QPointF &_scenePos, Qt::MouseButtons btnFlags, Qt::KeyboardModifier modifier) {
	const QPointF &oldp = scenePos();

    qreal deltax = _scenePos.x() - oldp.x();
    qreal deltay = _scenePos.y() - oldp.y();


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



    //
    // If _trapWidget then the widget wants to trap the pointer
    // within its window
    //
    // FittsLawTest plugin for example
    //
    if (_trapWidget) {
        QPointF localPoint = _trapWidget->mapFromScene(_scenePos);

        // If the pointer is on the widget's window
        // then move the pointer normally
        // otherwise don't move the pointer
        if (_trapWidget->contains(localPoint)) {
//            setPos(_scenePos);
            setOpacity(0);
            _trapWidget->handlePointerDrag(this, localPoint, deltax, deltay, Qt::NoButton);
        }
        else {
            setOpacity(1.0);
        }

        // return w/o moving pointer and further actions
//        return;
    }



    //
    // move pointer itself
    // Sets the position of the item to pos, which is in parent coordinates. For items with no parent, pos is in scene coordinates.
	//
    setPos(_scenePos);



	//////
	///
	// handle hovering if No button is pressed
	///
	//////
	if (btnFlags == 0 || btnFlags & Qt::NoButton) {
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

			// consider only user applications
			if (item->type() < QGraphicsItem::UserType + BASEWIDGET_USER) continue;

			firstUnderPointer = static_cast<SN_BaseWidget *>(item);
//			qDebug() << "shared pointer collides with basewidget" << firstUnderPointer;
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


	///////////
	//
    // LEFT button mouse dragging
	//
    // Because of the pointerPress action immediately preceding this,
	// the _basewidget or _specialItem might be set at this point.
	// The _basewidget is set if item's type is >= QGraphicsItem::UserType + BASEWIDGET_USER
	// The _specialItem is set if item's type is >= QGraphicsItem::UserType + INTERACTIVE_ITEM
    //
	//
    else if ( btnFlags & Qt::LeftButton ) {
		
		/*
		SN_DrawingWidget *canvas = _scene->drawingCanvas();
		if (_isDrawing && canvas) {
			if (_isErasing) {
				canvas->erase(canvas->mapFromScene(_scenePos));
			}
			else {
				canvas->drawLine(canvas->mapFromScene(oldp), canvas->mapFromScene(_scenePos), _color);
			}
			return;
		}
		*/


		//
		// If user's pointer is now dragging (w/ Left button pressed) on the user widget (SN_BaseWidget)
		// the widget will handle this. Base implementation is either resizing or moving.
		//		
        if (_basewidget) {

            /*
            //
            // if there are widgets selected by this pointer
            //
            if (_selectionRect->_selectedWidgetList.size() > 0) {
                //
                // And _basewidget is not resizing mode
                //
                if (_basewidget->isMoving()) {

                }
            }
            else {
                _basewidget->handlePointerDrag(this, _basewidget->mapFromScene(_scenePos), deltax, deltay, Qt::LeftButton, modifier);
            }
            */


            _basewidget->handlePointerDrag(this, _basewidget->mapFromScene(_scenePos), deltax, deltay, Qt::LeftButton, modifier);
        }

		//
		// If the SN_PartitionBar item (== UserType + INTERACTIVE_ITEM) is under the pointer
		// User is dragging a partitionBar
		//
		else if (_graphicsItem) {
//			QGraphicsLineItem *l = qgraphicsitem_cast<QGraphicsLineItem *>(_item);
			SN_WallPartitionBar *bar = dynamic_cast<SN_WallPartitionBar *>(_graphicsItem);
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
			// then it's either BASEWIDGET_NONUSER or normal QGraphicsItem/Widget
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

	////////////////////////
	//
	// RIGHT button mouse dragging
	//
	else if (btnFlags & Qt::RightButton) {
		// to be able to provide selection rectangle
		// mouse RIGHT PRESS must be sent from uiclient

        //
		// resize selection rectangle if it's begun.
		// selection rectangle's bottom right must be on the scenePos
        //

        if (_selectionRect->isVisible()) {
            QPointF bottomRight = _selectionRect->mapFromScene(_scenePos);

            QRectF rect = _selectionRect->geometry();

            _selectionRect->setGeometry(rect.x(), rect.y(), bottomRight.x(), bottomRight.y());
        }
	}
}

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


    //
    // White pointer color
    //
    setBrush(Qt::lightGray);

    //
    // There's something under the pointer positino
    //
    if (setAppUnderPointer(scenePos)) {
        if (_basewidget) {

            /*
            //
            // if there are widgets selected by this pointer
            //  and pointerPress occurred on one of the selected widgets
            //
            if (_selectionRect->_selectedWidgetList.size() > 0
                    &&
                    _selectionRect->_selectedWidgetList.contains(_basewidget))
            {
                QSetIterator<SN_BaseWidget *> iter(_selectionRect);
                while(iter.hasNext()) {
                    (iter.next())->handlePointerPress(this, _basewidget->mapFromScene(scenePos), btn);
                }
            }
            else {
                _basewidget->handlePointerPress(this, _basewidget->mapFromScene(scenePos), btn);
            }
            */


            _basewidget->handlePointerPress(this, _basewidget->mapFromScene(scenePos), btn);
        }
	}

    //
    // there's nothing under the point where the pointer is pressed
    //
    else {

    }



    if (btn == Qt::RightButton) {
        //
        // then start the selection rectangle (it finish upon release)
        //
        _selectionRect->setPos(scenePos);
        _selectionRect->resize(1,1);
        _selectionRect->show();
    }
}


/**
  The end of mouse dragging
  */
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
    //  item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER
    //
	if (_basewidget) {
		if (button == Qt::LeftButton) {
			//
			// I can close this app if removeButton on the scene contains released scenePos
			//
			if (_scene->isOnAppRemoveButton(scenePos)) {
				_scene->hoverAcceptingApps.removeAll(_basewidget);
				_basewidget->close();
                setBrush(_color);
				return;

//				if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
//					char record[64];
//					sprintf(record, "%lld %d \n", QDateTime::currentMSecsSinceEpoch(), 8, _uiclientid, scenePos.x(), scenePos.y(), button);
//					_scenarioFile->write(record);
//				}
			}


            //
            // minimizeBar
            //
            if (_scene->isOnMinimizeBar(scenePos)) {
                _scene->minimizeWidgetOnTheBar(_basewidget, scenePos);
                setBrush(_color);
                return;
            }

			//
			// I can move this app to SN_LayoutWidget that contains released scenePos
			// Always pass scene position
			//
			_scene->addItemOnTheLayout(_basewidget, _basewidget->scenePos());


			//
			// I can minimize this app if it's on the minimize rectangle.
			// _basewidget won't be a child of SN_LayoutWidget. It's a child of the scene
			//

		}

		//
		// invoke app specific release handler
		//
		_basewidget->handlePointerRelease(this, _basewidget->mapFromScene(scenePos), button);

		//
		// RESET
		//
		_basewidget = 0;
	}

    //
    // item->type() >= QGraphicsItem::UserType + INTERACTIVE_ITEM
    // do nothing for SN_PartitionBar
    //
	else if (_graphicsItem) {
		//
		// RESET
		//
		_graphicsItem = 0;
	}





    if (button == Qt::RightButton) {
        //
        // complete selection rectangle
        //
        QSet<SN_BaseWidget *> selected = _scene->getCollidingUserWidgets(_selectionRect);
        _selectionRect->_selectedWidgetList.unite(selected);

        QSetIterator<SN_BaseWidget *> iter(_selectionRect->_selectedWidgetList);
        while(iter.hasNext()) {
            SN_BaseWidget *bw = iter.next();
            bw->setSelectedByPointer();
        }
        _selectionRect->hide();

        qDebug() << "SN_PolygonArrowPointer::pointerRelease() : selected set is now" << _selectionRect->_selectedWidgetList.size();

    }


    ///
    // restore pointer color
    //
    setBrush(_color);
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
			if ( _basewidget->selectedByPointer()) {
				//
				// unselect widget
				//
				_basewidget->setSelectedByPointer(false);
				// to be effective, turn off WA_OpaquePaintEvent or set setAutoFillBackground(true)
//				_basewidget->palette().setColor(QPalette::Window, QColor(100, 100, 100, 128));

                _selectionRect->_selectedWidgetList.remove(_basewidget);
			}
			else {
				//
				// select widget
				//
				_basewidget->setSelectedByPointer();
//				_basewidget->palette().setColor(QPalette::Window, QColor(170, 170, 5, 164));
                _selectionRect->_selectedWidgetList.insert(_basewidget);
			}
		}
	}

	//
	// mouse left click
	//
	else if (btn == Qt::LeftButton) {
		if (_basewidget) {
			// Note that mousePress on graphicsItem will reset _basewidget to null

			///
			// Below needs to be implemented in the SN_BaseWidget
			// But then I need to reimplement its mouseReleaseEvent() which I don't want
			// or define handlePointerClick() which will be confusing because pointer generates the real mouse event which makes handling pointer click implicit.
			///
			/*
			Q_ASSERT(_basewidget->priorityData());
			_basewidget->priorityData()->setLastInteraction(SN_Priority::CLICK);
			*/

            if (_basewidget->windowState() == SN_BaseWidget::W_MINIMIZED) {
                setBrush(_color);

                _scene->restoreWidgetFromTheBar(_basewidget);

                return;
            }

            //
            // if handlePointerClick() returns true then
            // no system's click event will be generated
            //
            if (_basewidget->handlePointerClick(this, _basewidget->mapFromScene(scenePos), btn)) {

                //
                // no mouse event will be generated
                // if an application returns true in that function
                //
                setBrush(_color);

                //
                // if the function returns true then
                // return without generating system's mouse event
                //
                return;
            }
		}

        //
        // SN_PartitionBar
        //
        else if (_graphicsItem) {
            // SN_PartitionBar doesn't need a real mouse click event
            setBrush(_color);
            return;
        }






        //
        //
        // Generate system's mouse events
        // Press -> Relese
        //
        //
		QGraphicsView *view = eventReceivingViewport(scenePos);
		if ( !view ) {
			qDebug() << "pointerClick: no view is available";
			return;
		}
		QPointF clickedViewPos = view->mapFromScene( scenePos );

		QMouseEvent mpe(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, btn | Qt::NoButton, modifier);
		QMouseEvent mre(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, btn | Qt::NoButton, modifier);

		if ( ! QApplication::sendEvent(view->viewport(), &mpe) ) {
			// Upon receiving mousePressEvent, the item will become mouseGrabber if the item reimplement mousePressEvent
//			qDebug("SN_PolygonArrowPointer::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
//			qDebug() << "\tcurrent mouse grabber:" << _scene->mouseGrabberItem();
		}
		else {
//			qDebug() << "press delievered";
			if ( ! QApplication::sendEvent(view->viewport(), &mre) ) {
//				qDebug("SN_PolygonArrowPointer::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
			}
			else {
//				qDebug() << "release delievered";
				// who should ungrabmouse() ??
				QGraphicsItem *mouseGrabberItem = _scene->mouseGrabberItem();
				if (mouseGrabberItem) mouseGrabberItem->ungrabMouse();

			}
		}
	}


    //
    // Note that sagenextpointer will send PRESS -> CLICK for the pointer click
    // There's no RELEASE !
    //
    setBrush(_color);
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
        QPoint posInViewport = gview->mapFromScene(scenePos);

        QMouseEvent dblClickEvent(QEvent::MouseButtonDblClick, posInViewport, btn, btn | Qt::NoButton, modifier);
		QMouseEvent release(QEvent::MouseButtonRelease, posInViewport, btn, btn | Qt::NoButton, modifier);

        //
        // Below is needed when SN_BaseWidget doesn't reimplement QGraphicsItem::mousePressEvent()
        // Thus it can't become the mouse grabber upon receiving pressEvent
        //
//        _basewidget->grabMouse();

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

        QPoint posInViewport = gview->mapFromScene(scenePos);

		QWheelEvent we(posInViewport, /*gview->mapToGlobal(scenePos.toPoint()),*/ _delta, Qt::NoButton, Qt::NoModifier);

        //
        // system mouse should be on the widget !!
        //
//        QCursor::setPos(gview->mapToGlobal(posInViewport));

        if ( ! QApplication::sendEvent(gview->viewport(), &we) ) {
            qDebug("PolygonArrow::%s() : send wheelEvent failed", __FUNCTION__);
        }
        else {
            //qDebug() << "PolygonArrow wheel event sent" << gview->mapFromScene(scenePos);
        }
    }
}

bool SN_PolygonArrowPointer::setAppUnderPointer(const QPointF &scenePos) {
    Q_ASSERT(scene());

    QList<QGraphicsItem *> list = scene()->items(scenePos, Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
    //app = static_cast<BaseGraphicsWidget *>(scene()->itemAt(scenePosOfPointer, QTransform()));

	_basewidget = 0; // reset
	_graphicsItem = 0;
	_graphicsWidget = 0;

    //qDebug() << "\nPolygonArrow::setAppUnderPointer() :" << list.size() << " items";
    foreach (QGraphicsItem *item, list) {
        if ( item == this ) continue;

		if ( item->acceptedMouseButtons() == 0 ) continue;

		QGraphicsObject *object = item->toGraphicsObject();

        //
        // User application (inherits SN_BaseWidget)
        //
        if ( item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {

            _basewidget = static_cast<SN_BaseWidget *>(item);
            //qDebug("PolygonArrow::%s() : uiclientid %u, appid %llu", __FUNCTION__, uiclientid, app->globalAppId());
			_graphicsItem = 0;
            return true;
        }

        //
        // User application (inherits SN_BaseWidget)
        // But acts differently....
        // thereby the pointer should operate different on this widget.
        //
		else if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_NONUSER) {
			_graphicsItem = 0;
			_basewidget = 0;
			return false;
		}

        //
        // A QGraphicsItem type object that doesn't inherit SN_BaseWidget.
        // But still can be interacted with user shared pointers.
        //
        // SN_PartitionBar
        //
		else if (item->type() >= QGraphicsItem::UserType + INTERACTIVE_ITEM) {

			_graphicsItem = item;
			_basewidget = 0;
			return true;
		}

        //
        // regualar graphics items or items that inherit QGraphicsItem/Widget
        //
		else {

            _graphicsWidget = item->topLevelWidget();

			if(object) {
				if ( ::strcmp(object->metaObject()->className(), "SN_LineEdit") == 0 ) {
					SN_LineEdit *sle = static_cast<SN_LineEdit *>(object);
					sle->setThePointer(this);
					_graphicsWidget = sle;
				}

                else if (::strcmp(object->metaObject()->className(), "SN_MinimizeBar") == 0) {
                    _graphicsWidget = static_cast<SN_MinimizeBar *>(object);
                }
			}
		}
    }


    //qDebug("PolygonArrow::%s() : uiclientid %u, There's BaseGraphicsWidget type under pointer", __FUNCTION__, uiclientid);
    return false;
}

void SN_PolygonArrowPointer::injectStringToItem(const QString &str) {
	if (_graphicsWidget) {
		qDebug() << "SN_PolygonArrowPointer::injectStringToItem()" << str;

		qDebug() << "current gui item is a type of" << _graphicsWidget->metaObject()->className();

		SN_LineEdit *sle = static_cast<SN_LineEdit *>(_graphicsWidget);
		Q_ASSERT(sle);
		sle->setText(str);
	}
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

