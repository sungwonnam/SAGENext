#ifndef IMAGEWIDGETPLUGIN_H
#define IMAGEWIDGETPLUGIN_H

#include "applications/base/SN_plugininterface.h"
#include "applications/base/basewidget.h"

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

    int test;


private:
    void _createGUIs();

        /*!
          Example GUI components
          */
        QLabel *_label;

		QColor _currentColor;

        /*!
          Every GUI components requires QGraphicsProxyWidget to be functioning on QGraphics framework
          */
        QGraphicsProxyWidget *_labelProxy;

        QPushButton *btn_R; /**< red (1,0,0) */
        QPushButton *btn_M; /**< magenta (1,0,1) */
		QPushButton *btn_Y; /**< yellow (1,1,0) */

        QGraphicsProxyWidget *proxy_btn_R;
        QGraphicsProxyWidget *proxy_btn_M;
        QGraphicsProxyWidget *proxy_btn_Y;

		QCheckBox *_invert;
		QGraphicsProxyWidget *_proxy_invert;

        /*!
          Layout widget to layout GUI components nicely
          */
        QGraphicsLinearLayout *_btnLayout;
        QGraphicsLinearLayout *_mainLayout;

		bool _isInvertOn;

		void _updateLabel(const QColor &c);

private slots:
        void buttonR();
        void buttonM();
        void buttonY();

		void toggleInvert(int state);
};


#endif // IMAGEWIDGETPLUGIN_H
