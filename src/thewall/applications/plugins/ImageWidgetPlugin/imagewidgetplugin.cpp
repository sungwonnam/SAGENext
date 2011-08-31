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

ExamplePlugin::ExamplePlugin() :
		root(0)
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

	  put your drawing code here

	  ************/


	/** overlay app information */
	if ( showInfo  &&  !infoTextItem->isVisible() ) {
#if defined(Q_OS_LINUX)
		_appInfo->setDrawingThreadCpu(sched_getcpu());
#endif
		infoTextItem->show();
	}
	else if (!showInfo && infoTextItem->isVisible()){
		infoTextItem->hide();
	}

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

ExamplePlugin::~ExamplePlugin() {
}

void ExamplePlugin::mouseClick(const QPointF &clickedScenePos, Qt::MouseButton btn) {
	QPointF pos = mapFromScene(clickedScenePos);
	qDebug() << pos;




	QGraphicsView *gview = 0;
	foreach (gview, scene()->views()) {
		/** There is only one view for now **/

		// convert event's point to viewport coordinate
		QMouseEvent *press = new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);
		QMouseEvent *release = new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);

		QGraphicsItem *mouseGrabItem = 0;

//		qDebug() << "pos " << pos;

		if ( proxy_btn_R->boundingRect().contains(  proxy_btn_R->mapFromParent(pos) ) ) {
			qDebug() << "R";

			// GUI components should be mouseGrabItem to be able to respond to mouse interaction
			mouseGrabItem = proxy_btn_R;
		}
		else if (proxy_btn_G->boundingRect().contains( proxy_btn_G->mapFromParent(pos) )) {
			qDebug() << "G";
			mouseGrabItem = proxy_btn_G;
		}
		else if (proxy_btn_B->boundingRect().contains( proxy_btn_B->mapFromParent(pos) )) {
			qDebug() << "B";
			mouseGrabItem = proxy_btn_B;
		}
		else if (! mainLayout->geometry().contains( pos ) ) {
			qDebug() << "hitting outside of mainLayout";
			mouseGrabItem = this;
		}
		else {
//			qDebug() << pos << "isn't contained";
		}

		if (mouseGrabItem) mouseGrabItem->grabMouse();

		/** Mouse event must be posted to the viewport widget **/

		// sendEvent doesn't delete event object, so event can be created in stack (local to this function)
		if ( ! QApplication::sendEvent(gview->viewport(), press) ) {
			qDebug("ExamplePlugin::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
		}
		if ( ! QApplication::sendEvent(gview->viewport(), release) ) {
			qDebug("ExamplePlugin::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
		}

		if(mouseGrabItem) mouseGrabItem->ungrabMouse();

		if (press) delete press;
		if (release) delete release;

		/*
		  // event loop will take ownership of posted event, so event must be created in heap space
		   QApplication::postEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier));
		   QApplication::postEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier));
		   */
	}
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



