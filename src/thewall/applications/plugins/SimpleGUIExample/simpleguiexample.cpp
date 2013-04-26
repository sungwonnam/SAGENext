#include "simpleguiexample.h"
#include "common/sn_commonitem.h"

SimpleGUIExample::SimpleGUIExample()
    : SN_BaseWidget(Qt::Window)
    , _label(0)
    , _btn_R(0)
    , _btn_M(0)
    , _btn_Y(0)
    , _scrollbar(0)
{
	//
	// If an application doesn't have specific native size
    // and its native resolution can be changed then set this flag.
    // When a user resizes, this application will change its resolution
    // rather than scaling the window
	//
	setWindowFlags(Qt::Window);

    //
	// The window frame is not interactable by shared pointers
	// so use contentsMargins to make a window frame
	//
	setWindowFrameMargins(0, 0, 0, 0);

	//getContentsMargins(&l, &r, &t, &b);
	//qDebug() << "ImageWidgetPlugin content margins l, r, t, b" << l << r << t << b;

    // Give some room on top of the application window so that
    // a user can move the window of this application using the shared pointer
	setContentsMargins(8,28,8,8);
}

void SimpleGUIExample::_createGUIs() {
    //
    // create the label
    //
	_label = new QLabel();
	_label->setFrameStyle(QFrame::Box);
	_label->setLineWidth(8);

	//
    // create a proxywidget for the label because QGraphicsView can't display
    // QWidget type items. QGraphicsProxyWidget needs to be used to display a QWidget instance.
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
    // and _labelProxy's size will be handled by the layout
    //
	toplayout->addItem(labelProxy);

    //
	// create the buttons and connect their signals to corresponding callback functions
    //
    _btn_R = new SN_ProxyPushButton("Red", this);
    _btn_M = new SN_ProxyPushButton("Magenta", this);
    _btn_Y = new SN_ProxyPushButton("Yellow", this);

    QObject::connect(_btn_R, SIGNAL(clicked()), this, SLOT(buttonR()));
	QObject::connect(_btn_Y, SIGNAL(clicked()), this, SLOT(buttonY()));
	QObject::connect(_btn_M, SIGNAL(clicked()), this, SLOT(buttonM()));


    //
    // do the same for the scrollbar
    //
    _scrollbar = new SN_ProxyScrollBar(Qt::Horizontal, this);
    _scrollbar->setRange(0, 255);
    _scrollbar->setMinimumHeight(32);
    _scrollbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum, QSizePolicy::Slider);
    QObject::connect(_scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scrollbarmoved(int)));


    //
	// create another layout for the buttons
    //
	QGraphicsLinearLayout *btnLayout = new QGraphicsLinearLayout(Qt::Horizontal);
	btnLayout->addItem(_btn_R);
	btnLayout->addItem(_btn_M);
	btnLayout->addItem(_btn_Y);
	btnLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::PushButton);


    //
	// create the main layout that will have toplayout and _btnLayout
    //
	QGraphicsLinearLayout *mainLayout = new QGraphicsLinearLayout(Qt::Vertical);
	mainLayout->setContentsMargins(0,0,0,0);
	mainLayout->addItem(toplayout); // label
	mainLayout->addItem(btnLayout); // buttons
    mainLayout->addItem(_scrollbar); // horizontal scroll bar

    //
	// set the main layout to be this application's root layout
	// previous layout will be deleted by this.
    // Also the widget takes ownership of layout
    //
	setLayout(mainLayout);

	///
	// Once you figure out your widget resolution, call this function.
	//
	resize(800, 600);
    //
    // or just call if it's Qt::Window type and a layout is set for the widget
    //
//    adjustSize();
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
    // Build child items (GUI components in this example)
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
    if ( ! c.isValid() ) return;

	QPixmap p(size().toSize());
    p.fill(c);
	_currentColor = c;
	_label->setPixmap(p);
}

void SimpleGUIExample::scrollbarmoved(int val) {
    _updateLabel(QColor(val,val,val));
}


// Don't forget to add this
Q_EXPORT_PLUGIN2(MouseClickExamplePlugin, SimpleGUIExample)




