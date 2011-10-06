#ifndef IMAGEWIDGETPLUGIN_H
#define IMAGEWIDGETPLUGIN_H

#include "../../base/SN_plugininterface.h"
#include "../../base/basewidget.h"

#include <QtGui>

/*!
  A plugin example that inherits BaseWidget
  */
class ExamplePlugin : public SN_BaseWidget, SN_PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(SN_PluginInterface)

public:
	ExamplePlugin();
	virtual ~ExamplePlugin();

	SN_BaseWidget * createInstance();


protected:

        /*!
          You can reimplement QGraphicsWidget::paint().
          This is where your pixels are drawn on this widget.
		  This won't affect child items.
          */
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


		/**
		  To intercept wheelEvent sent to label
		  */
	bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

        /**
          BaseWidget's wheelEvent resizes the entire window if item flag is Qt::Widget
		  and nothing happens if Qt::Window
          */
//		void wheelEvent(QGraphicsSceneWheelEvent *event);

//	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
		/*!
		  This is null for the plugin that inherits BaseWidget
		  */
//        QGraphicsProxyWidget *root;

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

		QColor _currentColor;

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
