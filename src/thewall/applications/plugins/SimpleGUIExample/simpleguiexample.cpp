#include "simpleguiexample.h"

//#include "../../../common/commonitem.h"
//#include "../../base/perfmonitor.h"
//#include "../../base/appinfo.h"


SimpleGUIExample::SimpleGUIExample()
    : SN_BaseWidget(Qt::Window)
    , _label(0)
    , _labelProxy(0)

    , btn_R(0)
    , btn_M(0)
    , btn_Y(0)
    , proxy_btn_R(0)
    , proxy_btn_M(0)
    , proxy_btn_Y(0)

    , _invert(0)
    , _proxy_invert(0)

    , _btnLayout(0)
    , _mainLayout(0)

    , _isInvertOn(false)
{
	//
	// when resized, usually native application will set this
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

    // create label
	_label = new QLabel();
	_label->setFrameStyle(QFrame::Box);
	_label->setLineWidth(8);

		//
        // create proxywidget for label
		//
	_labelProxy = new QGraphicsProxyWidget(this, Qt::Widget);
	_labelProxy->setWidget(_label);
	_labelProxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Label);


	/*
	_invert = new QCheckBox("Invert");
	connect(_invert, SIGNAL(stateChanged(int)), this, SLOT(toggleInvert(int)));
	_proxy_invert = new QGraphicsProxyWidget(this, Qt::Widget);
	_proxy_invert->setWidget(_invert);
	*/


	QGraphicsLinearLayout *toplayout = new QGraphicsLinearLayout(Qt::Horizontal);
	toplayout->addItem(_labelProxy);
//	toplayout->addItem(_proxy_invert);


	// create buttons and connect to corresponding callback functions
	btn_R = new QPushButton("Red");
	btn_Y = new QPushButton("Yellow");
	btn_M = new QPushButton("Magenta");
	connect(btn_R, SIGNAL(clicked()), this, SLOT(buttonR()));
	connect(btn_Y, SIGNAL(clicked()), this, SLOT(buttonY()));
	connect(btn_M, SIGNAL(clicked()), this, SLOT(buttonM()));
		//
        // create proxywidget for buttons
		//
	proxy_btn_R = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_M = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_Y = new QGraphicsProxyWidget(this, Qt::Widget);
	proxy_btn_R->setWidget(btn_R);
	proxy_btn_M->setWidget(btn_M);
	proxy_btn_Y->setWidget(btn_Y);

	// create layout for buttons (proxywidgets for buttons more precisely)
	_btnLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	_btnLayout->addItem(proxy_btn_R);
	_btnLayout->addItem(proxy_btn_M);
	_btnLayout->addItem(proxy_btn_Y);
	_btnLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::PushButton);


	// create main layout
	_mainLayout = new QGraphicsLinearLayout(Qt::Vertical);
//	mainLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::DefaultType);
	_mainLayout->setContentsMargins(0,0,0,0);

	// add GUI components in it
	_mainLayout->addItem(toplayout);
	_mainLayout->addItem(_btnLayout);
//	mainLayout->setItemSpacing(0, 2);

	// set the main layout to be this application's root layout
	// previous layout will be deleted by this
	setLayout(_mainLayout);

	/*!
      Once you figure out your widget resolution, call this function. This is important.
      */
	resize(800, 400);
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
    SimpleGUIExample *instance = new SimpleGUIExample;

    instance->_createGUIs();

	return instance;
}

void SimpleGUIExample::buttonR() {
	btn_M->setDown(false);
	btn_Y->setDown(false);
	btn_R->setDown(true);
	_updateLabel(QColor(Qt::red));
}
void SimpleGUIExample::buttonM() {
	btn_M->setDown(true);
	btn_Y->setDown(false);
	btn_R->setDown(false);
	_updateLabel(QColor(Qt::magenta));
}
void SimpleGUIExample::buttonY() {
	btn_M->setDown(false);
	btn_Y->setDown(true);
	btn_R->setDown(false);
	_updateLabel(QColor(Qt::yellow));
}

void SimpleGUIExample::toggleInvert(int) {
	if (_isInvertOn)
		_isInvertOn = false;
	else
		_isInvertOn = true;

	_updateLabel(_currentColor);
}

void SimpleGUIExample::_updateLabel(const QColor &c) {
	QPixmap p(size().toSize());

	if (_isInvertOn) {
		int r,g,b;
		c.getRgb(&r, &g, &b);
		r = 255 - r;
		g = 255 - g;
		b = 255 - b;
		p.fill(QColor(r,g,b));
	}
	else {
		p.fill(c);
	}
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
