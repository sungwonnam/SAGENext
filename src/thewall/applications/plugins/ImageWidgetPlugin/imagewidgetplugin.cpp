#include "imagewidgetplugin.h"

#include "../../../common/commonitem.h"
#include "../../base/perfmonitor.h"
#include "../../base/appinfo.h"
//#include "../../base/affinityinfo.h"

//#include "../../../system/resourcemonitor.h"
//#include "../../../system/sagenextscheduler.h"

#include <QtGui>

/*
int AffinityInfo::Num_Numa_Nodes = 0;
int AffinityInfo::Cpu_Per_Numa_Node = 0;
int AffinityInfo::SwThread_Per_Cpu = 0;
int AffinityInfo::HwThread_Per_Cpu = 0;
int AffinityInfo::Num_Cpus = 0;
*/

ExamplePlugin::ExamplePlugin()
    : BaseWidget(Qt::Window)
    , root(0)
{

//	image = new QImage(800, 600, QImage::Format_RGB888);
//	for (int i=0; i<image->byteCount(); ++i) {
//		image->bits()[i] = 130;
//	}
//	image->fill(130);


        // create label
        label = new QLabel();
//	label->setPixmap(QPixmap(800,600));
        label->setText("Hello World");
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        // create proxywidget for label
        labelProxy = new QGraphicsProxyWidget(this, Qt::Widget);
        labelProxy->setWidget(label);
        labelProxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Label);


        // create buttons and connect to corresponding callback functions
        btn_R = new QPushButton("button1");
        btn_G = new QPushButton("button2");
        btn_B = new QPushButton("button3");
        connect(btn_R, SIGNAL(clicked()), this, SLOT(buttonR()));
        connect(btn_G, SIGNAL(clicked()), this, SLOT(buttonG()));
        connect(btn_B, SIGNAL(clicked()), this, SLOT(buttonB()));

        // create proxywidget for buttons
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

        // add GUI components in it
        mainLayout->addItem(labelProxy);
        mainLayout->addItem(btnLayout);

        // set the main layout to be this application's root layout
        setLayout(mainLayout);

        // Window will have title bar and window frame
        setWindowFlags(Qt::Window);
        resize(800, 600);

        /*!
          Once you figure out your widget resolution, call this
          */
//	resize(800, 600);

//	_appInfo->setFrameSize(800, 600, 24);

        /* appInfo->setFrameSize() takes care of it */
//	appInfo->setNativeSize(QSize(800, 600)); // duplicate

//		qreal l, r, t, b;
//		getWindowFrameMargins(&l, &t, &r, &b);
//		qDebug() << "ImageWidgetPlugin frame margins l, r, t, b" << l << r << t << b;

		// window frame is not interactible by shared pointers
		setWindowFrameMargins(0, 0, 0, 0);

//		getContentsMargins(&l, &r, &t, &b);
//		qDebug() << "ImageWidgetPlugin content margins l, r, t, b" << l << r << t << b;

		// Qt::Window might want to define mouse dragging. For that case, give more room to top margin so that window can be moved with shared pointers
		setContentsMargins(4,24,4,4);
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
//        BaseWidget::paint(painter, option, widget);

        /** measure latency */
        /*
        if (_perfMon)
                _perfMon->updateDrawLatency();
        */
}

//void ExamplePlugin::resizeEvent(QGraphicsSceneResizeEvent *event) {
//	qDebug() << "boundingRect" << boundingRect();
//	qDebug() << "main layout geometry" << mainLayout->geometry();
//	qDebug() << "main layout contect rect" << mainLayout->contentsRect();
//	qDebug() << "window frame rect (local)" << windowFrameRect();
//}


void ExamplePlugin::wheelEvent(QGraphicsSceneWheelEvent *event) {
    Q_UNUSED(event);
    // do nothing here and keep the base implementation
	BaseWidget::wheelEvent(event);
}

ExamplePlugin::~ExamplePlugin() {
}

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

QString ExamplePlugin::name() const {
        // Please return parent class name.
        // Don't change this !!!
        return "BaseWidget";
}

/*!
  THis plugin doesn't have to have proxy widget
  Because it reimplements paint() and manually draw everything.
  So you don't have to modify this function.
  */
QGraphicsProxyWidget * ExamplePlugin::rootWidget() {
        return root;
}

void ExamplePlugin::buttonR() {
        label->setText("Button 1");
//	label->resize(640, 480);

//	prepareGeometryChange();
//	resize(640, 480);

//	prepareGeometryChange();
//	labelProxy->resize(640,480);
//	updateGeometry();
//	labelProxy->updateGeometry();

//	labelProxy->setGeometry(QRectF(0,0,640,480));
//	mainLayout->itemAt(0)->setGeometry(QRectF(0,0,640,480));
//	mainLayout->geometry( 0,0, mainLayout->geometry().size() );
//	mainLayout->setGeometry();
}
void ExamplePlugin::buttonG() {
        label->setText("Button 2");
}
void ExamplePlugin::buttonB() {
        label->setText("Button 3");
}


Q_EXPORT_PLUGIN2(ImageWidgetPlugin, ExamplePlugin)



