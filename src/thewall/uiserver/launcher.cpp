#include "launcher.h"

Launcher::Launcher(GraphicsViewMain *gvm, QGraphicsItem *parent /* 0 */, Qt::WindowFlags wflags /*Qt::Window*/)
	: QGraphicsWidget(parent, wflags), gvMain(gvm)
{

	/* create icons */
	// these are child object so they will receive mouse event
//	webIcon = new QGraphicsPixmapItem(QPixmap(":/resources/webkit_128x128.png"), this);
//	pluginIcon = new QGraphicsPixmapItem(QPixmap(":/resources/plugin_200x200.png"), this);

//	imageIcon = new QGraphicsPixmapItem(QPixmap(":/resources/image.png"), this);
//	videoIcon = new QGraphicsPixmapItem(QPixmap(":/resources/video.png"), this);


	webButton = new QToolButton();
	webButton->setIcon(QPixmap(":/resources/webkit_128x128.png"));
	webButton->setIconSize(QSize(128, 128));

	QGraphicsProxyWidget *webButtonProxy = new QGraphicsProxyWidget(this);
	webButtonProxy->setWidget(webButton);
	webButtonProxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::PushButton);

	/* layout */
	linearLayout = new QGraphicsLinearLayout(Qt::Vertical, this);

	linearLayout->addItem(webButtonProxy);
	linearLayout->setSpacing(4);

	setLayout(linearLayout);


	// launcher is always top most
	setZValue(1000);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, false);

}

Launcher::~Launcher() {

}
