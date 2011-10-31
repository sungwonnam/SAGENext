#include "simpleguiexample.h"

#include "../../../common/commonitem.h"
//#include "../../base/perfmonitor.h"
//#include "../../base/appinfo.h"


SimpleGUIExample::SimpleGUIExample()
    : SN_BaseWidget(Qt::Window)
{
	//
	// when resized, usually native application will set this
	//
	setWindowFlags(Qt::Window);


	// create label
	label = new QLabel();
	label->setFrameStyle(QFrame::Box);
	label->setLineWidth(8);

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
void SimpleGUIExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
//	if (_perfMon)
//		_perfMon->getDrawTimer().start();

	SN_BaseWidget::paint(painter, option, widget);

	/*************
	put your drawing code here if needed
      ************/
//	painter->fillRect(boundingRect(), _currentColor.darker());


//	if (_perfMon)
//		_perfMon->updateDrawLatency();
}

SimpleGUIExample::~SimpleGUIExample() {
}

SN_BaseWidget * SimpleGUIExample::createInstance() {
	return new SimpleGUIExample;
}

void SimpleGUIExample::buttonR() {
	QPixmap pixmap(size().toSize());
	label->setText("Red clicked");
	_currentColor = QColor(Qt::red);
	pixmap.fill( _currentColor );
	label->setPixmap(pixmap);
}
void SimpleGUIExample::buttonG() {
	QPixmap pixmap(size().toSize());
	label->setText("Green clicked");
	_currentColor = QColor(Qt::green);
	pixmap.fill(_currentColor);
	label->setPixmap(pixmap);
}
void SimpleGUIExample::buttonB() {
	label->setText("Blue clicked");
	QPixmap pixmap(size().toSize());
	_currentColor = QColor(Qt::blue);
	pixmap.fill(_currentColor);
	label->setPixmap(pixmap);
}


Q_EXPORT_PLUGIN2(MouseClickExamplePlugin, SimpleGUIExample)






























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
