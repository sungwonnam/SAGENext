#include "simpleguiexample.h"

SimpleGUIExample::SimpleGUIExample()
    : SN_BaseWidget(Qt::Window)
    , _label(0)
    , _btn_R(0)
    , _btn_M(0)
    , _btn_Y(0)
{
	//
	// If a widget's size can be changed then set this flag
	//
	setWindowFlags(Qt::Window);

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

void SimpleGUIExample::_createGUIs() {
    //
    // create the label displaying selected color
    //
	_label = new QLabel();
	_label->setFrameStyle(QFrame::Box);
	_label->setLineWidth(8);

	//
    // create a proxywidget for the label.
    // A widget needs to have a corresponding proxywidget in Qt Graphics Framework.
    //
	QGraphicsProxyWidget *labelProxy = new QGraphicsProxyWidget(this, Qt::Widget);
	labelProxy->setWidget(_label);
	labelProxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Label);


    //
    // use layout widget to juxtapose items.
    //
	QGraphicsLinearLayout *toplayout = new QGraphicsLinearLayout(Qt::Horizontal);

    //
    // now the toplayout has ownership of the _labelProxy
    //
	toplayout->addItem(labelProxy);

    //
	// create the buttons and connect their signals to corresponding callback functions
    //
	_btn_R = new QPushButton("Red");
	_btn_Y = new QPushButton("Yellow");
	_btn_M = new QPushButton("Magenta");
	QObject::connect(_btn_R, SIGNAL(clicked()), this, SLOT(buttonR()));
	QObject::connect(_btn_Y, SIGNAL(clicked()), this, SLOT(buttonY()));
	QObject::connect(_btn_M, SIGNAL(clicked()), this, SLOT(buttonM()));

		//
        // Again, create proxywidgets for the buttons
		//
	QGraphicsProxyWidget *proxy_btn_R = new QGraphicsProxyWidget(this, Qt::Widget);
	QGraphicsProxyWidget *proxy_btn_M = new QGraphicsProxyWidget(this, Qt::Widget);
	QGraphicsProxyWidget *proxy_btn_Y = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_R->setWidget(_btn_R);
	proxy_btn_M->setWidget(_btn_M);
	proxy_btn_Y->setWidget(_btn_Y);

    //
	// create another layout for the buttons
    //
	QGraphicsLinearLayout *btnLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	btnLayout->addItem(proxy_btn_R);
	btnLayout->addItem(proxy_btn_M);
	btnLayout->addItem(proxy_btn_Y);
	btnLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::PushButton);


    //
	// create the main layout that will have toplayout and _btnLayout
    //
	QGraphicsLinearLayout *mainLayout = new QGraphicsLinearLayout(Qt::Vertical);
	mainLayout->setContentsMargins(0,0,0,0);
	mainLayout->addItem(toplayout);
	mainLayout->addItem(btnLayout);

    //
	// set the main layout to be this application's root layout
	// previous layout will be deleted by this.
    // Also the widget takes ownership of layout
    //
	setLayout(mainLayout);

	///
	// Once you figure out your widget resolution, call this function.
	//
//	resize(800, 400);
    //
    // or just call if it's Qt::Window type and a layout is set for the widget
    //
    adjustSize();
}


/*!
  Reimplement my own paint function
  */
void SimpleGUIExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //
    // The base implementation draws border around the widget when selected with a sagenextPointer
    //
	SN_BaseWidget::paint(painter, option, widget);

	/*************
	put your drawing code here if needed
      ************/
//	painter->fillRect(boundingRect(), _currentColor.darker());
}

SimpleGUIExample::~SimpleGUIExample() {
    //
    // Any child object of the widget (which is Q_OBJECT) will be deleted automatically
    //
}

SN_BaseWidget * SimpleGUIExample::createInstance() {
    //
    // Create a new instance
    //
    SimpleGUIExample *instance = new SimpleGUIExample;

    //
    // Build child items
    //
    instance->_createGUIs();

	return instance;
}

void SimpleGUIExample::buttonR() {
	_btn_M->setDown(false);
	_btn_Y->setDown(false);
	_btn_R->setDown(true);
	_updateLabel(QColor(Qt::red));
}
void SimpleGUIExample::buttonM() {
	_btn_M->setDown(true);
	_btn_Y->setDown(false);
	_btn_R->setDown(false);
	_updateLabel(QColor(Qt::magenta));
}
void SimpleGUIExample::buttonY() {
	_btn_M->setDown(false);
	_btn_Y->setDown(true);
	_btn_R->setDown(false);
	_updateLabel(QColor(Qt::yellow));
}


void SimpleGUIExample::_updateLabel(const QColor &c) {
	QPixmap p(size().toSize());
    p.fill(c);
	_currentColor = c;
	_label->setPixmap(p);
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
