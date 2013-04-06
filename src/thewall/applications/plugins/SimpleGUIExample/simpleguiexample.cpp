#include "simpleguiexample.h"
#include "common/commonitem.h"

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
    if ( ! c.isValid() ) return;

	QPixmap p(size().toSize());
    p.fill(c);
	_currentColor = c;
	_label->setPixmap(p);
}

void SimpleGUIExample::scrollbarmoved(int val) {
    _updateLabel(QColor(val,val,val));
}

#ifndef QT5
Q_EXPORT_PLUGIN2(MouseClickExamplePlugin, SimpleGUIExample)
#endif



