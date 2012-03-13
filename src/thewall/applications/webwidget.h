#ifndef WEBWIDGET_H
#define WEBWIDGET_H

#include "base/basewidget.h"
#include "common/commonitem.h"

#include <QFutureWatcher>
//#include <QWebPage>

class QGraphicsWebView;
//class QLineEdit;
class SN_LineEdit;
class SN_PixmapButton;
class SN_SimpleTextWidget;

class QGraphicsLinearLayout;
class QWebPage;
class QWebFrame;
class QGraphicsProxyWidget;


/**
  Use the netscape plugin wrapper to convert libflashplugin.so to enable youtube video
  */
class SN_WebWidget : public SN_BaseWidget
{
	Q_OBJECT

public:
        /**
          * Qt::Window flag is very important in WebWidget
          * Because it enables resizing through widget frames. So there must be some windowFrameMargin
          */
	SN_WebWidget(const quint64 gaid, const QSettings *setting, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = Qt::Window);
	~SN_WebWidget();


        /*!
          Reimplementing BaseWidget::mouseClick()
          This function generates mousePressEvent followed by mouseReleaseEvent, post them to the view
          */
//        void mouseClick(const QPointF &clickedScenePos, Qt::MouseButton btn= Qt::LeftButton);


		/*!
		  generate system mouse press event so that
		  gwebview can be the mouseGrabber
		  */
	void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

		/*!
		  pointer drag should generate real system mouse event so that users can interact with web contents (e.g. google map)
		  */
	void handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);


	void handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

protected:
//	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {/* do nothing */}

        /*!
          * This is needed because WebWidget is stacked before gwebview.
          * By default, child item is stacked after parent widget (child item's zvalue is higher than parent).
          * So, all mouse events happened on the gwebview's boundingRect arrive at gwebview.
          *
          * gwebview installs sceneEventFilter on the parent widget,
          * so that its events can be filtered by WebWidget before events are reached to gwebview.
          * (gwebview->installSceneEventFilter(WebWidget))
          *
          * Or WebWidget can be set to filter all child events.
          * (QGraphicsItem::setFiltersChildEvents( bool enabled ))
          *
          * Either case, WebWidget should reimplement sceneEventFilter that defines how to filter childs events.
          *
          * return false to skip filtering (just forward the event to child item)
          * return true to intercept the event
          */
	bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

//        void wheelEvent(QGraphicsSceneWheelEvent *event);

		// use BaseWidget's implementation
//        void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        /**
          * When this widget is being resized, gwebview should be resized accordingly
          */
//	void resizeEvent(QGraphicsSceneResizeEvent *event);
//	void keyPressEvent(QKeyEvent *event);
//	void keyReleaseEvent(QKeyEvent *event);


private:
	QGraphicsLinearLayout *linearLayout;
    QGraphicsLinearLayout *horizLayout;

	QGraphicsWebView *gwebview;
//	QLineEdit *urlbox;
	SN_LineEdit *_customurlbox;
	QGraphicsProxyWidget *urlboxproxy;

    QToolBar* mainBrowserToolbar;
    QGraphicsProxyWidget *toolbarProxy;

    /*
    SN_PixmapButton* backHistoryButton;
    SN_PixmapButton* forwardHistoryButton;
    SN_PixmapButton* reloadPageButton;
    SN_PixmapButton* stoploadPageButton;
    SN_PixmapButton* increaseZoomButton;
    SN_PixmapButton* decreaseZoomButton;
    */
    QAction* incZoom;
    QAction* decZoom;

    SN_SimpleTextWidget* zoomDisplay;

	QWebPage *webPage;
	QWebFrame *webFrame;
	//QFutureWatcher<void> futureWatcher;

public slots:
	/**
      sets new web URL and triggers webpage loading
      */
	void setUrl(const QString &url);

	void setUrlFromLineEdit();

	void urlChanged(const QUrl &url);

	void pageLoaded();

    void handleincZoom();
    void handledecZoom();

};







/**
  what if user clicked a html link on the page ????????????????????????????????????
  **/
/*
class QWebPage;

class WebPageThread : public QThread {
                Q_OBJECT
public:
        WebPageThread(WebWidget *ww, QObject *parent=0);
        ~WebPageThread();

private:
        QWebPage *webPage;
        WebWidget *view;

private slots:
        void newUrl();
};
*/

#endif // WEBWIDGET_H
