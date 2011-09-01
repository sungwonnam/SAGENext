#ifndef IMAGEWIDGETPLUGIN_H
#define IMAGEWIDGETPLUGIN_H

#include "../../base/dummyplugininterface.h"
#include "../../base/basewidget.h"

#include <QtGui>

/*!
  A plugin example that inherits BaseWidget
  */
class ExamplePlugin : public BaseWidget, DummyPluginInterface
{
        Q_OBJECT
        Q_INTERFACES(DummyPluginInterface)

public:
        ExamplePlugin();
        virtual ~ExamplePlugin();

        /*!
          must implement the interface (DummyPluginInterface)
          */
        QString name() const;
        QGraphicsProxyWidget * rootWidget();

        /*!
          reimplement BaseWidget::mouseClick() to receive mouse event
          */
        void mouseClick(const QPointF &, Qt::MouseButton);


protected:

        /*!
          Reimplement QGraphicsWidget::paint().
          THis is where your pixels are drawn.
          */
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        /**
          BaseWidget's wheelEvent resizes the entire window
          It is recommended to provide custom implementation on this event when window flag is set to Qt::Window
          */
        void wheelEvent(QGraphicsSceneWheelEvent *event);

//	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
        QGraphicsProxyWidget *root;

        /*!
          your paint device
          */
//	QImage *image;

        /*!
          Try using pixmap as much as possible because it's faster !
          */
//	QPixmap *pixmap;
//	QGraphicsPixmapItem *pixmapItem;


        /*!
          Example GUI components
          */
        QLabel *label;

        /*!
          Every GUI components requires QGraphicsProxyWidget to be functioning on QGraphics framework
          */
        QGraphicsProxyWidget *labelProxy;

        QPushButton *btn_R;
        QPushButton *btn_G;
        QPushButton *btn_B;
        QGraphicsProxyWidget *proxy_btn_R;
        QGraphicsProxyWidget *proxy_btn_G;
        QGraphicsProxyWidget *proxy_btn_B;

        /*!
          Layout widget to layout GUI components nicely
          */
        QGraphicsLinearLayout *btnLayout;
        QGraphicsLinearLayout *mainLayout;

private slots:
        void buttonR();
        void buttonG();
        void buttonB();

};


#endif // IMAGEWIDGETPLUGIN_H
