#ifndef IMAGEWIDGETPLUGIN_H
#define IMAGEWIDGETPLUGIN_H

#include "../../base/SN_plugininterface.h"
#include "../../base/basewidget.h"

#include <QtGui>

/*!
  A plugin example that inherits BaseWidget
  */
class SimpleGUIExample : public SN_BaseWidget, SN_PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(SN_PluginInterface)

public:
	SimpleGUIExample();
	virtual ~SimpleGUIExample();

	SN_BaseWidget * createInstance();

protected:

        /*!
          You can reimplement QGraphicsWidget::paint().
          This is where your pixels are drawn on this widget.
		  This won't affect child items.
          */
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


private:

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
