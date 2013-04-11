#include "common/sn_sharedpointer.h"
#include "common/sn_commonitem.h"
#include "common/sn_commondefinitions.h"
#include "common/sn_layoutwidget.h"
#include "applications/base/sn_basewidget.h"
#include "sn_scene.h"
#include "sn_sagenextlauncher.h"

#include <QGraphicsWebView>

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




SN_PolygonArrowPointer::SN_PolygonArrowPointer(const quint32 uicid, SN_UiMsgThread *msgthread, const QSettings *s, SN_TheScene *scene, SN_Launcher *l, const QString &name, const QColor &c, QFile *scenarioFile, QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent)
    , _scene(scene)
    , _theLauncher(l)
    , _uimsgthread(msgthread)
    , _uiclientid(uicid)
    , _settings(s)
    , _textItem(0)
	, _color(c)
    , _trapWidget(0)
    , _basewidget(0)
    , _graphicsItem(0)
    , _graphicsWidget(0)
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
    // If _trapWidget then the widget wants to trap the pointer within its window
    // This is for SN_FittsLawTest to draw its own cursor in its application window
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
    }



    //
    // move the pointer itself.
	//
    setPos(_scenePos);



	//////
	///
	// handle hovering if No button is pressed
	///
	//////
	if (btnFlags == 0 || btnFlags & Qt::NoButton) {
		//
		// iterate over items in the list of widgets that registered for hover
		// if the pointer is hovering one of them
		// set hover flag for the item
		// otherwise unset hover flag
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

		/* get the topmost item on the cursor's position */
        /*
		QGraphicsItem *topmost = _scene->itemAt(_scenePos);
		SN_BaseWidget *firstUnderPointer = dynamic_cast<SN_BaseWidget *>(topmost); // QGraphicsItem is polymorphic class so dynamic cast from Base to Derived is ok
*/

		/* For each SN_BaseWidget that registered for hovering */
		foreach(SN_BaseWidget *bw, _scene->hoverAcceptingApps) {
			if (!bw) {
				_scene->hoverAcceptingApps.removeOne(bw);
				continue;
			}
	
			if (bw == firstUnderPointer/* && bw->contains(bw->mapFromScene(_scenePos))*/ ) {
				bw->handlePointerHover(this, bw->mapFromScene(_scenePos), true);
			}
			else {
				// Hover has left
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
        // if _graphicsItem != 0 then the QGraphicsItem under the pointer is interactable
		//
		else if (_graphicsItem) {
//			QGraphicsLineItem *l = qgraphicsitem_cast<QGraphicsLineItem *>(_item);

            //
            // dynamic_cast from Base to Derived is not allowed (compile error) unless the Base class is polymorphic (A class that declares or inherits virtual functions).
            //
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
        else if (_graphicsWidget) {
            QGraphicsProxyWidget* pw = dynamic_cast<QGraphicsProxyWidget*>(_graphicsWidget);
            if (pw) {
                SN_ProxyScrollBar* psb = dynamic_cast<SN_ProxyScrollBar*>(pw);
                if (psb) {
                    psb->drag(psb->mapFromScene(_scenePos));
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


            //
            // Do not call press() when the pointer is pressed on the minimized bar
            // because it's the press() function that saves widget's geometry
            //
            if ( ! _scene->isOnMinimizeBar(scenePos)) {
                _basewidget->handlePointerPress(this, _basewidget->mapFromScene(scenePos), btn);
            }
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
            // If it's dropped onto the minimizeBar on the bottom of the scene
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
	//
	// If Recording is set
	//
	if (_scenarioFile && _scenarioFile->isOpen() && _scenarioFile->isWritable() && _settings->value("misc/record_pointer", false).toBool()) {
		char record[64];
		sprintf(record, "%lld %d %u %d %d %d\n", QDateTime::currentMSecsSinceEpoch(), 5, _uiclientid, scenePos.toPoint().x(), scenePos.toPoint().y(), btn);
		_scenarioFile->write(record);
	}


    bool realMouseEventFlag = true;

	//
    // mouse Right click
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
	// mouse Left click
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

                // no more action is needed
                realMouseEventFlag = false;
            }

            //
            // if handlePointerClick() returns true then
            // no system's click event will be generated
            //
            if (_basewidget->handlePointerClick(this, _basewidget->mapFromScene(scenePos), btn)) {
                //
                // if the function returns true then
                // return without generating system's mouse event
                //
                realMouseEventFlag = false;
            }
		}

        //
        // SN_PartitionBar
        //
        else if (_graphicsItem) {
            // SN_PartitionBar doesn't need a real mouse click event so return here
            realMouseEventFlag = false;
        }

        //
        // Pretty much everything else (mostly GUI components)
        //
        else if (_graphicsWidget) {
            SN_PixmapButton *btn = 0;
            SN_ProxyGUIBase *pbtn = 0;
            if ( btn = dynamic_cast<SN_PixmapButton *>(_graphicsWidget) ) {
                //qDebug() << "SN_PolygonArrowPointer::pointerClick() : SN_PixmapButton under the pointer. calling its handlePointerClick()";
                btn->click();
                realMouseEventFlag = false;
            }
            else if (pbtn = dynamic_cast<SN_ProxyGUIBase*>(_graphicsWidget)) {
                pbtn->click( pbtn->mapFromScene(scenePos).toPoint() );
                realMouseEventFlag = false;
            }
        }


        //
        // Everything has handled so no mouse event will be generated.
        //
        if (realMouseEventFlag) {
            //
            // If control reaches here, then I have no idea what the item under the pointer is..
            //
            // Generate system's mouse events for standard Qt widgets..
            // Press -> Relese
            //
            QGraphicsView *view = eventReceivingViewport(scenePos);
            if ( !view ) {
                qDebug() << "pointerClick: no view is available";
                return;
            }
            QPointF clickedViewPos = view->mapFromScene( scenePos );

            /*
             * Note that below code block (mouse press and release) won't work with Qt version higher than 4.8.0
             *
            QMouseEvent mpe(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, btn | Qt::NoButton, modifier);
            QMouseEvent mre(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, btn | Qt::NoButton, modifier);

            if ( ! QApplication::sendEvent(view->viewport(), &mpe) ) {
                // Upon receiving mousePressEvent, the item will become mouseGrabber if the item reimplement mousePressEvent
                //qDebug("SN_PolygonArrowPointer::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
                //qDebug() << "\tcurrent mouse grabber:" << _scene->mouseGrabberItem();
            }
            else {
                //qDebug() << "press delievered";
                if ( ! QApplication::sendEvent(view->viewport(), &mre) ) {
                    //qDebug("SN_PolygonArrowPointer::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
                }
                else {
                    //qDebug() << "release delievered";
                    // who should ungrabmouse() ??
                    QGraphicsItem *mouseGrabberItem = _scene->mouseGrabberItem();
                    if (mouseGrabberItem) mouseGrabberItem->ungrabMouse();

                }
            }
            */


            /*
             * Below will work with Qt 4.8.1 and higher
             */
            QGraphicsSceneMouseEvent mpress(QEvent::GraphicsSceneMousePress);
            QGraphicsSceneMouseEvent mouseevent(QEvent::GraphicsSceneMouseRelease);

            mpress.setScenePos(scenePos);
            mpress.setPos(scenePos);
            Q_ASSERT(view && view->viewport());
            mpress.setScreenPos(view->viewport()->mapToGlobal(clickedViewPos.toPoint()));
            mpress.setButton(Qt::LeftButton);
            mpress.setButtons(Qt::LeftButton);

            mouseevent.setScenePos(scenePos);
            mouseevent.setPos(scenePos);
            Q_ASSERT(view && view->viewport());
            mouseevent.setScreenPos(view->viewport()->mapToGlobal(clickedViewPos.toPoint()));
            mouseevent.setButton(Qt::LeftButton);
            mouseevent.setButtons(Qt::LeftButton);

            if ( ! qApp->sendEvent(_scene, &mpress) ) {
                qDebug() << "SN_PolygonArrowPointer::pointerClick() : sendevent() press failed";
            }
            if ( ! qApp->sendEvent(_scene, &mouseevent) ) {
                qDebug() << "SN_PolygonArrowPointer::pointerClick() : sendevent() release failed";
            }

            if (_scene->mouseGrabberItem())
                _scene->mouseGrabberItem()->ungrabMouse();
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


    /**
     * User clicked on an empty space
     */
    if (!_basewidget && !_graphicsItem && !_graphicsWidget) {

        if (!_theLauncher) {
            qDebug() << "SN_PolygonArrowPointer::pointerDoubleClick() SN_Launcher isn't available";
        }
        else {
            _theLauncher->launchMediaBrowser(scenePos, _uiclientid, _textItem->text());
        }
        return;
    }


    // Generate system's mouse double click event directly from here
    QGraphicsView *gview = eventReceivingViewport(scenePos);

    if (gview) {
        QPoint posInViewport = gview->mapFromScene(scenePos);

        /**
         * below code block doesn't work with Qt version higher than 4.8.0

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

        **/

        /*
         * Below will work with Qt 4.8.1 and higher
         */
        QGraphicsSceneMouseEvent devent(QEvent::GraphicsSceneMouseDoubleClick);
        devent.setScenePos(scenePos);
        devent.setPos(scenePos);
        Q_ASSERT(gview && gview->viewport());
        devent.setScreenPos(gview->viewport()->mapToGlobal(posInViewport));
        devent.setButton(Qt::LeftButton);
        devent.setButtons(Qt::LeftButton);

        if ( ! qApp->sendEvent(_scene, &devent) ) {
            qDebug() << "SN_PolygonArrowPointer::pointerDoubleClick() : sendevent() dblclick failed";
        }

        if (_scene->mouseGrabberItem())
            _scene->mouseGrabberItem()->ungrabMouse();
    }
    else {
        qDebug("SN_PolygonArrowPointer::%s() : there is no viewport widget on %.1f, %.1f", __FUNCTION__, scenePos.x(), scenePos.y());
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

        /***
		QWheelEvent we(posInViewport,  _delta, Qt::NoButton, Qt::NoModifier);

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
        ***/


        QGraphicsSceneWheelEvent wheel(QEvent::GraphicsSceneWheel);
        wheel.setScenePos(scenePos);
        wheel.setPos(scenePos);
        wheel.setScreenPos(gview->viewport()->mapToGlobal(posInViewport));
        wheel.setDelta(_delta);
        if ( ! qApp->sendEvent(_scene, &wheel) ) {
            qDebug() << "SN_PolygonArrowPointer::pointerWheel() : sendevent() wheel failed";
        }

        if (_scene->mouseGrabberItem())
            _scene->mouseGrabberItem()->ungrabMouse();
    }
}

bool SN_PolygonArrowPointer::setAppUnderPointer(const QPointF &scenePos) {
    Q_ASSERT(scene());

    QList<QGraphicsItem *> list = scene()->items(scenePos, Qt::ContainsItemBoundingRect, Qt::DescendingOrder);
    //app = static_cast<BaseGraphicsWidget *>(scene()->itemAt(scenePosOfPointer, QTransform()));

	_basewidget = 0;
	_graphicsItem = 0; // QGraphicsItem type
	_graphicsWidget = 0; // QGraphicsWidget type

    foreach (QGraphicsItem *item, list) {

        //
        // this will ignore any widget that sets setAcceptedMouseButtons(0)
        // e.g. SN_LayoutWidget, SN_PolygonArrowPointer, SN_FittsLawTest, SN_SimpleTextWidget
        //
		if ( item->acceptedMouseButtons() == 0 ) continue;

//        qDebug() << item;

        /*!
         * There can be a child QGraphicsObject attached to the QGraphicsWebView.
         * WebGL is a such example.
         * This is to make SN_WebWidget's handlePointerDrag() can be alled in such case.
         */
        QGraphicsObject *go = item->toGraphicsObject();
        if (go) {
            /*
            qDebug() << go;
            if (go->parentObject()) qDebug() << "parentObject" << go->parentObject();
            if (go->parentWidget()) qDebug() << "parentWidget" << go->parentWidget();
            if (go->parentItem()) qDebug() << "parentItem" << go->parentItem();
            if (go->parent()) qDebug() << "parent" << go->parent();
                */
            if (go->parentWidget() )  {
                QGraphicsWebView *gwv = dynamic_cast<QGraphicsWebView *>(go->parentWidget());
                if (gwv) {
                    Q_ASSERT(gwv->parentWidget()); // SN_WebWidget
                    _basewidget = dynamic_cast<SN_BaseWidget*>(gwv->parentWidget());
                    return true;
                }
            }
        }


        //
        // User application (any widget that inherits SN_BaseWidget)
        //
        if ( item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {
            _basewidget = static_cast<SN_BaseWidget *>(item);
            //qDebug("PolygonArrow::%s() : uiclientid %u, appid %llu", __FUNCTION__, uiclientid, app->globalAppId());
            return true;
        }

        //
        // A QGraphicsItem type that doesn't inherit SN_BaseWidget.
        // But still can be interacted with users' shared pointers.
        //
        // SN_PartitionBar
        //
		else if (item->type() >= QGraphicsItem::UserType + INTERACTIVE_ITEM) {
			_graphicsItem = item;
			return true;
		}

        //
        // A QGraphicsWidget type that is interatable with pointer
        // SN_PixmapWidget, SN_LineEdit
        //
        // QGraphicsProxyWidget , QGraphicsWebView types are belong to this block too.
        //
		else {
            _graphicsWidget = dynamic_cast<QGraphicsWidget *>(item);
            if (_graphicsWidget) {

                //
                // check if this is QGraphicsView
                // QGraphicsView of SN_WebWidget should be on top of its parent (SN_WebWidget) in order to receive real mouse events
                //
                // cast from Parent to Child isn't usually allowed unless Parent is polymorphic class
                QGraphicsWebView* webview = dynamic_cast<QGraphicsWebView*>(_graphicsWidget);
                if (webview) {
                    _basewidget = _getBaseWidgetFromChildWidget(webview->parentWidget());
                    _graphicsWidget = 0;
                    if (!_basewidget) return false;
                }


                // A item under the pointer could be a GUI component that uses proxy widget.
                QGraphicsProxyWidget* pwidget = dynamic_cast<QGraphicsProxyWidget*>(_graphicsWidget);
                if (pwidget) {
                    // SN_ProxyScrollBar, SN_ProxyPushButton
                    // Because the proxyWidget is implementing handlePointerClick()/Drag()....
                    _graphicsWidget = pwidget;
                    return true;
                }


                return true;
            }
		}
    }

    //qDebug("PolygonArrow::%s() : uiclientid %u, There's BaseGraphicsWidget type under pointer", __FUNCTION__, uiclientid);
    return false;
}

void SN_PolygonArrowPointer::injectStringToItem(const QString &str) {
	if (_graphicsWidget) {
		qDebug() << "SN_PolygonArrowPointer::injectStringToItem()" << str;
//		qDebug() << "And the current gui item is a type of" << _graphicsWidget->metaObject()->className();

		SN_ProxyLineEdit *sle = dynamic_cast<SN_ProxyLineEdit *>(_graphicsWidget);
		if(sle)
            sle->setText(str); // this will trigger textChanged() signal
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

SN_BaseWidget* SN_PolygonArrowPointer::_getBaseWidgetFromChildWidget( QGraphicsWidget *childwidget) {
    if (!childwidget) return 0;

    if (!childwidget->parentWidget()) return 0;

    SN_BaseWidget* basewidget = dynamic_cast<SN_BaseWidget*>(childwidget);

    if (basewidget) {
        return basewidget;
    }
    else {
        return _getBaseWidgetFromChildWidget(childwidget->parentWidget());
    }
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

