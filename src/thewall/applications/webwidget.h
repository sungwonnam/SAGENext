#ifndef WEBWIDGET_H
#define WEBWIDGET_H

#include "base/basewidget.h"
#include <QFutureWatcher>
#include <QWebPage>

class QGraphicsWebView;
class QLineEdit;
class QGraphicsLinearLayout;



/**
  * flash video (youtube for example) doesn't work with OpenGL viewport
  */
class WebWidget : public BaseWidget
{
        Q_OBJECT

public:
        /**
          * Qt::Window flag is very important in WebWidget
          * Because it enables resizing through widget frames. So there must be some windowFrameMargin
          */
        WebWidget(const quint64 gaid, const QSettings *setting, QGraphicsItem *parent=0, Qt::WindowFlags wFlags = Qt::Window);
        ~WebWidget();


        /*!
          Reimplementing BaseWidget::mouseClick()
          This function generates mousePressEvent followed by mouseReleaseEvent, post them to the view
          */
        void mouseClick(const QPointF &clickedScenePos, Qt::MouseButton btn= Qt::LeftButton);

        /**
          sets new web URL and triggers webpage loading
          */
        void setUrl(const QString &url);

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

        void wheelEvent(QGraphicsSceneWheelEvent *event);


        bool windowFrameEvent(QEvent *e);

        void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        /**
          * When this widget is being resized, gwebview should be resized accordingly
          */
//	void resizeEvent(QGraphicsSceneResizeEvent *event);
//	void keyPressEvent(QKeyEvent *event);
//	void keyReleaseEvent(QKeyEvent *event);


private:
        QGraphicsLinearLayout *linearLayout;
        QGraphicsWebView *gwebview;
        QLineEdit *urlbox;
        QGraphicsProxyWidget *urlboxproxy;


        QWebPage *webPage;
        QWebFrame *webFrame;
        QFutureWatcher<void> futureWatcher;

public slots:

        void setUrlFromLineEdit();
        void urlChanged(const QUrl &url);

        void pageLoaded();
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
