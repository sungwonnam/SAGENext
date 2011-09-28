#include "webwidget.h"
#include "base/appinfo.h"

#include <QtGui>
#include <QtWebKit>

WebWidget::WebWidget(const quint64 gaid, const QSettings *setting, QGraphicsItem *parent /*0*/, Qt::WindowFlags wFlags/* Qt::Window */)
    : BaseWidget(gaid, setting, parent, wFlags)
    , linearLayout(0)
    , gwebview(0)
    , urlbox(0)
    , urlboxproxy(0)
{
	setWidgetType(BaseWidget::Widget_Web);

        /**
          When you create a child item, it is by default top most item (stacking on top of the parent)
          So, all the mouse/keyboard events are grabbed by the child item (because it's top most).

          To prevent this, you can call
                  TheChildItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
          and then reimplement event handlers of this item.


          OR just hide() the child item (QGraphicsItem::hide()). Refer BaseWidget's infoTextItem


          OR
          You can intercept all the events before it even gets to the childs by doing
// TheChildItem->installSceneEventFilter(this); // TheChildItem's events are filtered by this->sceneEventFilter(). This is goold if you want to filter specific child item only.



          OR
// this->setFileterChildEvents(true); // This object(WebWidget) will filter *ALL* events for all its children, so childs don't need to install event filter (installSceneEventFilter()) explicitly
         **/
	setFiltersChildEvents(true);

        // more top for mouse handle
        // default is 4, 25, 4, 4
        // I have to do this because BaseWidget sets this
	setWindowFrameMargins(4, 25, 4, 4); // left, top, right, bottom


//	connect(&futureWatcher, SIGNAL(finished()), this, SLOT(pageLoaded()));


	/* URL box with QLineEdit widget */
	urlbox = new QLineEdit("http://");
	QFont f;
	f.setPointSize(18);
	urlbox->setFont(f);
	connect(urlbox, SIGNAL(returnPressed()), this, SLOT(setUrlFromLineEdit()));


	/* Corresponding proxy widget for the URL box */
	urlboxproxy = new QGraphicsProxyWidget(); // this is bad for graphics perfomance. But it's the only way
	//proxyWidget->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

	// urlboxproxy takes ownership of urlbox
	urlboxproxy->setWidget(urlbox); // widget(urlbox) must be top-level widget whose parent is 0
	urlboxproxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred, QSizePolicy::LineEdit);



	/* webkit related */
	QWebSettings *ws = QWebSettings::globalSettings();
	ws->setAttribute(QWebSettings::JavaEnabled, true);
	ws->setAttribute(QWebSettings::JavascriptEnabled, true);
	ws->setAttribute(QWebSettings::PluginsEnabled, true);
	ws->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

	/**
      gwebview needs to be able to receive mouse events because users want to click links on web pages
      That's why webwidget filter childs' event instead of stacking children behind the parent
     */
	gwebview = new QGraphicsWebView(); // it is now the top most item unless ItemStacksBehindParent is true
//	gwebview->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
//	gwebview->installSceneEventFilter(this);
//	webPage = gwebview->page();

	gwebview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Frame);
	connect(gwebview, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));


	linearLayout = new QGraphicsLinearLayout(Qt::Vertical, this);
	linearLayout->setSpacing(4);

	// The layout takes ownership of these items.
	linearLayout->addItem(urlboxproxy);
	linearLayout->addItem(gwebview);

	// Any existing layout is deleted before the new layuout is assigned, and this widget will take ownership of the layout
	setLayout(linearLayout);


	/* This means nothing. Because wheel event won't be handled by BaseGraphicsWidget, and this widget redefines resizeEvent */
	//	setNativeSize(810,650); // w/o frame
}


WebWidget::~WebWidget() {
//	if ( gwebview ) delete gwebview;
//	delete urlbox;

	/** I shouldn't do below. Below objects are deleted by Qt **/
//	if (linearLayout) delete linearLayout;
//	if (gwebview) delete gwebview;
//	if (urlboxproxy) delete urlboxproxy;
//	if (urlbox) delete urlbox;

	qDebug("WebWidget::%s()", __FUNCTION__);
}



/**
  Because what this function does is intercepting events delivered to its children, the events must be delivered to the children.
  Therefore, this function won't work if childs are stacked behind the parent (WebWidget)
 */
bool WebWidget::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {

	//
	// I'll only filter events delivered to the gwebview
	// So this won't be executed if the gwebview is set to be stacked behind the parent
	//
	if (watched == gwebview ) {
		//	qDebug("WebWidget::%s() : gwebview is going to receive event %d", __FUNCTION__, event->type());

		if ( event->type() == QEvent::GraphicsSceneContextMenu) {
			QGraphicsSceneContextMenuEvent *e = static_cast<QGraphicsSceneContextMenuEvent *>(event);

			BaseWidget::contextMenuEvent(e);

			//
			// Don't forward this event to gwebview
			//
			return true;
		}

		else if (event->type() == QEvent::GraphicsSceneMousePress) {
//			qDebug() << "gwebview received pressEvent";

			//
			// First, deliver this event to BaseWidget to keep the behaviors defined in the BaseWidget
			//
			BaseWidget::mousePressEvent((QGraphicsSceneMouseEvent *)event);

			//
			// The press event should reach to gwebview otherwise no mouse click can be done on gwebview.
			// So, return false to forward this event to the gwebview.
			//
			return false;
		}

		else if (event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
//			qDebug() << "webwidget received doubleClick event";
			BaseWidget::mouseDoubleClickEvent((QGraphicsSceneMouseEvent *)event);

			//
			// gwebview will not handle doubleclick
			// The WebWidget should handle this.
			//
			return true;
		}


		/*
 // resizing of the WebWidget is done by window frame. Notice that windowFlag of WebWidget is Qt::Window
 else if (event->type() == QEvent::GraphicsSceneWheel) {
   return false; // don't filter so that gwebview can grab this event
 }
  // release event should reach gwebview to fire mouse clicking
 else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
   return false; // Don't filter this event. release event should reach to gwebview
 }
 else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
   return false; // Don't filter this event. key press should be handled by gwebview
 }
	     */
	}

	if (watched == urlboxproxy) {
		if (event->type() == QEvent::GraphicsSceneMousePress)
			BaseWidget::setTopmost();
	}

	return false; // All other events won't be filtered by WebWidget
}



/**
 If mouse wheel events are occuring on gwebview then below won't be called anyway
 */
void WebWidget::wheelEvent(QGraphicsSceneWheelEvent *event) {
        // WebWidget will not respond to this event.
        // shouldn't do anything..
	event->ignore();
}

bool WebWidget::windowFrameEvent(QEvent *e) {
//	qDebug() << "frame event" << e << boundingRect() << frame->boundingRect();
	return QGraphicsWidget::windowFrameEvent(e);
}


void WebWidget::pageLoaded() {
	//	qDebug() << ";sfkja;fklasjdf;";
	//	gwebview->setContent( gwebview->page()->mainFrame()->toHtml().toAscii() );

	gwebview->setPage( webPage );
	qDebug() << gwebview->page()->mainFrame()->toHtml().toAscii();

	update(); // schedule paint()
}

void WebWidget::setUrl(const QString &u) {
	QUrl url;
	if ( u.startsWith("http://", Qt::CaseInsensitive) || u.startsWith("https://", Qt::CaseInsensitive) ) {
		url.setUrl(u, QUrl::TolerantMode);
	}
	else {
		QString http("http://");
		url.setUrl(http.append(u), QUrl::TolerantMode);
	}

	if (!url.isEmpty() && url.isValid()) {

		appInfo()->setWebUrl( url );
		//	gwebview->setUrl(url);
		gwebview->load(url);

		_appInfo->setWebUrl(url);

		//	webPage = new QWebPage;
		//	webFrame = webPage->mainFrame();
		//	QFuture<void> future = QtConcurrent::run(webFrame, &QWebFrame::load, url);
		//	futureWatcher.setFuture(future);

		/**
          QObject: Cannot create children for a parent that is in a different thread.
          (Parent is QGraphicsWebView(0x87ddd0), parent's thread is QThread(0x6877a0), current thread is QThread(0x8c5bd0)
         */
		//	QFuture<void> future = QtConcurrent::run(gwebview, &QGraphicsWebView::load, url);
	}
	else {
		qWarning("WebWidget::%s() : URL [%s] is not valid", __FUNCTION__, qPrintable(url.toString()) );
	}
}

void WebWidget::setUrlFromLineEdit() {
	setUrl(urlbox->text());
}

void WebWidget::urlChanged(const QUrl &url) {
	urlbox->setText( url.toString());
}



void WebWidget::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

//    qDebug() << "size" << size() << "boundingrect" << boundingRect() << "geometry" << geometry();

    QLinearGradient lg;
    lg.setFinalStop(0, 0);
    lg.setStart(boundingRect().width(), boundingRect().height());
    lg.setColorAt(0, QColor::fromRgbF(1, 1, 0, 1));
    lg.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));
    QBrush b(lg);

    painter->setBrush(b);
    painter->drawRect(windowFrameRect());
//    QGraphicsWidget::paintWindowFrame(painter, option, widget);
}
