#include "webwidget.h"
#include "base/appinfo.h"

#include <QtGui>
#include <QtWebKit>

WebWidget::WebWidget(const quint64 gaid, const QSettings *setting, QGraphicsItem *parent /*0*/, Qt::WindowFlags wFlags/* Qt::Window */) :

                BaseWidget(gaid, setting, parent, wFlags),
                linearLayout(0),
                gwebview(0),
                urlbox(0),
                urlboxproxy(0)
{
        setWidgetType(BaseWidget::Widget_Web);

        /**
          When you create child object, it is by default top most item (stacking on top of the parent)
          So, all the mouse/keyboard events are grabbed by the child item (because it's top most)
          To prevent this, you can call
//	childItem->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
          and then reimplement event handlers of this item


          OR

          You can intercept events by calling
// childItem->installSceneEventFilter(this); // childItem's events are filtered by this->sceneEventFilter()
// setFileterChildEvents(true); // This object(WebWidget) will filter all events for all its children, so childs don't need to install event filter (installSceneEventFilter()) explicitly
         **/
        setFiltersChildEvents(true);

        // more top for mouse handle
        setWindowFrameMargins(4, 25, 4, 4); // left, top, right, bottom



        connect(&futureWatcher, SIGNAL(finished()), this, SLOT(pageLoaded()));



        /* url box */
        QFont f;
        f.setPointSize(18);

        urlbox = new QLineEdit("http://");
        urlbox->setFont(f);
        connect(urlbox, SIGNAL(returnPressed()), this, SLOT(setUrlFromLineEdit()));

        urlboxproxy = new QGraphicsProxyWidget(); // this is very bad for graphics perfomance
//	proxyWidget->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

        // urlboxproxy takes ownership of urlbox
        urlboxproxy->setWidget(urlbox); // widget(urlbox) must be top-level widget whose parent is 0
        urlboxproxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred, QSizePolicy::LineEdit);



        /* webkit */
        QWebSettings *ws = QWebSettings::globalSettings();
        ws->setAttribute(QWebSettings::JavaEnabled, true);
        ws->setAttribute(QWebSettings::JavascriptEnabled, true);
        ws->setAttribute(QWebSettings::PluginsEnabled, true);
        ws->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

        gwebview = new QGraphicsWebView(); // it is now the top most item unless ItemStacksBehindParent is true
//	gwebview->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
//	gwebview->installSceneEventFilter(this);
//	webPage = gwebview->page();

        gwebview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Frame);
        connect(gwebview, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));


        linearLayout = new QGraphicsLinearLayout(Qt::Vertical, this);

        // The layout takes ownership of the items.
        linearLayout->addItem(urlboxproxy);
        linearLayout->addItem(gwebview);
        linearLayout->setSpacing(4);

        // Any existing layout is deleted before the new layuout is assigned, and this widget will take ownership of the layout
        setLayout(linearLayout);


        /* This means nothing. Because wheel event won't be handled by BaseGraphicsWidget,
           and this widget redefines resizeEvent */
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



/** This won't be executed if gwebview is set to be stacked behind the parent
 * ,and the parent (which is this object) accept the event
 */
bool WebWidget::sceneEventFilter(QGraphicsItem *watched, QEvent *event) {

        /* I'll only filter gwebview events */
        if (watched == gwebview ) {
//		qDebug("WebWidget::%s() : gwebview is going to receive event %d", __FUNCTION__, event->type());

                if ( event->type() == QEvent::GraphicsSceneContextMenu) {
                        QGraphicsSceneContextMenuEvent *e = static_cast<QGraphicsSceneContextMenuEvent *>(event);
                        BaseWidget::contextMenuEvent(e);
                        return true; // context menu is not going to be handled by gwebview
                }

                else if (event->type() == QEvent::GraphicsSceneMousePress) {
//			BaseWidget::setTopmost();
                        BaseWidget::mousePressEvent((QGraphicsSceneMouseEvent *)event);
                        return false; // Don't filter this event. press event should reach to gwebview
                }


                /*
                // resizing of the WebWidget is done by window frame. Notice that windowFlag of WebWidget is Qt::Window
                else if (event->type() == QEvent::GraphicsSceneWheel) {
                        return false; // don't filter so that gwebview can grab this event
                }
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

        return false; // event will be propagated to gwebview
}

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
//		gwebview->setUrl(url);
                gwebview->load(url);

//		webPage = new QWebPage;
//		webFrame = webPage->mainFrame();
//		QFuture<void> future = QtConcurrent::run(webFrame, &QWebFrame::load, url);
//		futureWatcher.setFuture(future);

                /**
QObject: Cannot create children for a parent that is in a different thread.
(Parent is QGraphicsWebView(0x87ddd0), parent's thread is QThread(0x6877a0), current thread is QThread(0x8c5bd0)
**/
//		QFuture<void> future = QtConcurrent::run(gwebview, &QGraphicsWebView::load, url);
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





void WebWidget::mouseClick(const QPointF &clickedScenePos, Qt::MouseButton btn/* = Qt::LeftButton */) {

        QPointF pos = mapFromScene(clickedScenePos);

        /** There is only one view ? **/

        QGraphicsView *gview = 0;
        foreach (gview, scene()->views()) {
                if ( gwebview->boundingRect().contains(pos) ) {

                        QMouseEvent *press = new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);
                        QMouseEvent *release = new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier);

                        gwebview->grabMouse();
                        // sendEvent doesn't delete event object, so event can be created in stack (local to this function)
                        if ( ! QApplication::sendEvent(gview->viewport(), press) ) {
                                qDebug("WebWidget::%s() : sendEvent MouseButtonPress failed", __FUNCTION__);
                        }
                        if ( ! QApplication::sendEvent(gview->viewport(), release) ) {
                                qDebug("WebWidget::%s() : sendEvent MouseButtonRelease failed", __FUNCTION__);
                        }
                        gwebview->ungrabMouse();

                        if (press) delete press;
                        if (release) delete release;

                        /*
                          // event loop will take ownership of posted event, so event must be created in heap space
                        QApplication::postEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonPress, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier));
                        QApplication::postEvent(gview->viewport(), new QMouseEvent(QEvent::MouseButtonRelease, gview->mapFromScene(clickedScenePos), btn, Qt::NoButton | Qt::LeftButton, Qt::NoModifier));
                        */
                }
        }
}

void WebWidget::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->setBrush(QBrush(Qt::lightGray, Qt::Dense6Pattern));
        painter->drawRect(windowFrameRect());
}


//Q_EXPORT_PLUGIN2(webBrowser, WebWidget)
