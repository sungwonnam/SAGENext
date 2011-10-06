#include "imagewidgetplugin.h"

#include "../../../common/commonitem.h"
//#include "../../base/perfmonitor.h"
//#include "../../base/appinfo.h"


ExamplePlugin::ExamplePlugin()
    : SN_BaseWidget(Qt::Window)
{
	//
	// when resized, usually native application will set this
	//
	setWindowFlags(Qt::Window);

	//
	// I want to intercept wheelEvent sent to label and do something.
	// It is sent to the label because label (child) is stacking above me (parent)
	//
	setFiltersChildEvents(true);

	// create label
	label = new QLabel();
//	label->setPixmap(QPixmap(800,600));
	label->setText("Hello World");
	label->setFrameStyle(QFrame::NoFrame);
//	label->setLineWidth(6);
	label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

		//
        // create proxywidget for label
		//
	labelProxy = new QGraphicsProxyWidget(this, Qt::Widget);
	labelProxy->setWidget(label);
	labelProxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Label);


	// create buttons and connect to corresponding callback functions
	btn_R = new QPushButton("Red");
	btn_G = new QPushButton("Green");
	btn_B = new QPushButton("Blue");
	connect(btn_R, SIGNAL(clicked()), this, SLOT(buttonR()));
	connect(btn_G, SIGNAL(clicked()), this, SLOT(buttonG()));
	connect(btn_B, SIGNAL(clicked()), this, SLOT(buttonB()));

		//
        // create proxywidget for buttons
		//
	proxy_btn_R = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_G = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_B = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_R->setWidget(btn_R);
	proxy_btn_G->setWidget(btn_G);
	proxy_btn_B->setWidget(btn_B);

	// create layout for buttons (proxywidgets for buttons more precisely)
	btnLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	btnLayout->addItem(proxy_btn_R);
	btnLayout->addItem(proxy_btn_G);
	btnLayout->addItem(proxy_btn_B);
	btnLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::PushButton);


	// create main layout
	mainLayout = new QGraphicsLinearLayout(Qt::Vertical);
//	mainLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::DefaultType);
	mainLayout->setContentsMargins(0,0,0,0);

	// add GUI components in it
	mainLayout->addItem(labelProxy);
	mainLayout->addItem(btnLayout);
//	mainLayout->setItemSpacing(0, 2);

	// set the main layout to be this application's root layout
	// previous layout will be deleted by this
	setLayout(mainLayout);

	/*!
      Once you figure out your widget resolution, call this function. This is important.
      */
	resize(800, 400);

//	_appInfo->setFrameSize(800, 600, 24);

        /* appInfo->setFrameSize() takes care of it */
//	appInfo->setNativeSize(QSize(800, 600)); // duplicate

//		qreal l, r, t, b;
//		getWindowFrameMargins(&l, &t, &r, &b);
//		qDebug() << "ImageWidgetPlugin frame margins l, r, t, b" << l << r << t << b;

	//
	// window frame is not interactable by shared pointers
	// so use contentsMargins for window frame
	//
	setWindowFrameMargins(0, 0, 0, 0);

	//getContentsMargins(&l, &r, &t, &b);
	//qDebug() << "ImageWidgetPlugin content margins l, r, t, b" << l << r << t << b;

	// Qt::Window might want to define mouse dragging. For that case, give more room to top margin so that window can be moved with shared pointers
	setContentsMargins(8,28,8,8);
}


/*!
  Reimplement my own paint function
  */
void ExamplePlugin::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem *, QWidget *)
{
        /** starts timer */
        /*
        if (_perfMon)
                _perfMon->getDrawTimer().start();
        */


        /*************

          put your drawing code here if needed

          ************/


        /** overlay app information */
	//BaseWidget::paint(painter, option, widget);

        /** measure latency */
        /*
        if (_perfMon)
                _perfMon->updateDrawLatency();
        */
}

ExamplePlugin::~ExamplePlugin() {
}

SN_BaseWidget * ExamplePlugin::createInstance() {
	return new ExamplePlugin;
}

//QString ExamplePlugin::name() const {
//	//
//    // Please return parent class name.
//    // Don't change this !!!
//	//
//	return "BaseWidget";
//}

/*!
  THis plugin doesn't have to have proxy widget
  Because it reimplements paint() and manually draw everything.
  So you don't have to modify this function.
  */
//QGraphicsProxyWidget * ExamplePlugin::rootWidget() {
//	return root;
//}


bool ExamplePlugin::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {
	if (watched == labelProxy) {
		if ( event->type() == QEvent::GraphicsSceneWheel) {

			QGraphicsSceneWheelEvent *we = static_cast<QGraphicsSceneWheelEvent *>(event);
			int numDegrees = we->delta() / 8;
			int numTicks = numDegrees / 15;
			qDebug() << numTicks;

			QColor newcolor;
			if (numTicks>0) {
				newcolor  = _currentColor.lighter(101);
			}
			else {
				newcolor = _currentColor.darker(101);
			}

			QPixmap pixmap(size().toSize());
			pixmap.fill( _currentColor );
			label->setPixmap(pixmap);

			// event is not going to be forwarded to label
			return true;
		}
	}

	// everything else should be sent to proper child items
	return false;
}


void ExamplePlugin::buttonR() {
	QPixmap pixmap(size().toSize());
	label->setText("Red clicked");
	_currentColor = QColor(Qt::red);
	pixmap.fill( _currentColor );
	label->setPixmap(pixmap);
}
void ExamplePlugin::buttonG() {
	QPixmap pixmap(size().toSize());
	label->setText("Green clicked");
	_currentColor = QColor(Qt::green);
	pixmap.fill(_currentColor);
	label->setPixmap(pixmap);
}
void ExamplePlugin::buttonB() {
	label->setText("Blue clicked");
	QPixmap pixmap(size().toSize());
	_currentColor = QColor(Qt::blue);
	pixmap.fill(_currentColor);
	label->setPixmap(pixmap);
}


Q_EXPORT_PLUGIN2(MouseClickExamplePlugin, ExamplePlugin)






























/*
void ExamplePlugin::mouseClick(const QPointF &clickedScenePos, Qt::MouseButton btn) {
	QGraphicsView *view = 0;
	Q_ASSERT(scene());
	foreach(QGraphicsView *v, scene()->views()) {

		// geometry of widget relative to its parent
		//        v->geometry();

		// internal geometry of widget
		//        v->rect();

		// Figure out which viewport will receive the mouse event
		if ( v->rect().contains( v->mapFromScene(clickedScenePos) ) ) {
			// mouse click position is within this view's bounding rectangle
			view = v;
			break;
		}
	}

	if (!view) {
		qDebug() << "ExamplePlugin::mouseClick() : Couldn't find the viewport with clickedScenePos" << clickedScenePos;
		return;
	}


	// The event will be sent to the viewport so the pos should be translated
	QPointF clickedViewPos = view->mapFromScene( clickedScenePos );
	QPointF pos = mapFromScene(clickedScenePos);
	qDebug() << "ExamplePlugin::mouseClick on this item's coordinate" << pos << " scene coord" << clickedScenePos << " viewport coord" << clickedViewPos;

	// create event objects
	QMouseEvent *press = new QMouseEvent(QEvent::MouseButtonPress, clickedViewPos.toPoint(), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);
	QMouseEvent *release = new QMouseEvent(QEvent::MouseButtonRelease, clickedViewPos.toPoint(), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);


	// figure out which button should receive the mouse event based on the clicked position
	QGraphicsItem *mouseGrabItem = 0;
	if ( proxy_btn_R->boundingRect().contains( proxy_btn_R->mapFromScene(clickedScenePos) ) ) {
		qDebug() << "R";

		// GUI components should be mouseGrabItem to be able to respond to mouse interaction
		mouseGrabItem = proxy_btn_R;
	}
	else if (proxy_btn_G->boundingRect().contains( proxy_btn_G->mapFromScene(clickedScenePos) )) {
		qDebug() << "G";
		mouseGrabItem = proxy_btn_G;
	}
	else if (proxy_btn_B->boundingRect().contains( proxy_btn_B->mapFromScene(clickedScenePos) )) {
		qDebug() << "btn 3 will become mouseGrabber";
		mouseGrabItem = proxy_btn_B;
	}
	else if (! mainLayout->geometry().contains( pos ) ) {
		qDebug() << "hitting outside of mainLayout";
		mouseGrabItem = this;
	}
	else {
	}


	if (mouseGrabItem) mouseGrabItem->grabMouse();
	qDebug() << "mouse grabbed";

	// Mouse events must be posted to the viewport widget

	// sendEvent BLOCKS and thus it doesn't delete event object,
	// so events can be created in stack (local to this function) when using sendEvent
	if ( ! QApplication::sendEvent(view->viewport(), press) ) {
		qDebug("ExamplePlugin::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
	}
	else {
		if ( ! QApplication::sendEvent(view->viewport(), release) ) {
			qDebug("ExamplePlugin::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
		}
	}
	if(mouseGrabItem) mouseGrabItem->ungrabMouse();
	qDebug() << "mouse ungrabbed";

	// sendEvent doesn't delete the event object
	if (press) delete press;
	if (release) delete release;
}
*/
