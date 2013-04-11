#ifndef WEBWIDGET_H
#define WEBWIDGET_H

#include "applications/base/sn_basewidget.h"
#include <QFutureWatcher>
//#include <QWebPage>

class QGraphicsWebView;
class QGraphicsProxyWidget;
class QWebInspector;

class SN_ProxyLineEdit;

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
    QGraphicsWebView *_gwebview;

	SN_ProxyLineEdit *_customurlbox;

    /*!
     * \brief _webinspector shows debug console
     */
    QWebInspector* _webinspector;

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



public slots:
	/**
      This function sets new web URL and triggers webpage loading.
      */
	void setUrl(const QString &url);

    /*!
     * \brief
     * \param
     *
     * setText() emits textChanged() signal
     * textChanged() -> setUrl()
     * setUrl() emits urlChanged()
     * urlChanged() -> this function
     */
    void urlChanged(const QUrl &url);
};


#endif // WEBWIDGET_H
