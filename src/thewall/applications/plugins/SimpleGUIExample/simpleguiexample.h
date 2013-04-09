#ifndef IMAGEWIDGETPLUGIN_H
#define IMAGEWIDGETPLUGIN_H

#include "applications/base/SN_plugininterface.h"
#include "applications/base/sn_basewidget.h"

#include <QtGui>

class SN_ProxyPushButton;
class SN_ProxyScrollBar;

/*!
  A simple SAGENext plugin example.

  This example shows three buttons and one label.
  The buttons are used to change the color of the label panel
  */
class SimpleGUIExample : public SN_BaseWidget, SN_PluginInterface
{
	Q_OBJECT

    /*!
      A macro that tells this is a plugin
      and will implement interfaces defined in SN_PluginInterface
      */
	Q_INTERFACES(SN_PluginInterface)

public:
	SimpleGUIExample();
	virtual ~SimpleGUIExample();

    /*!
      This interface must be reimplemented.
      The SN_Launcher calls this function to create an instance of this plugin.
      */
	SN_BaseWidget * createInstance();

    /*!
      You can reimplement QGraphicsItem::paint() to draw onto this widget.
      */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    /*!
      Create the label and buttons
      */
    void _createGUIs();

    /*!
      Example GUI components
      */
    QLabel *_label;

    /*!
      The color of the label
      */
    QColor _currentColor;

//    QPushButton *_btn_R; /**< red (1,0,0) */
//    QPushButton *_btn_M; /**< magenta (1,0,1) */
//    QPushButton *_btn_Y; /**< yellow (1,1,0) */

    SN_ProxyPushButton* _btn_R;
    SN_ProxyPushButton* _btn_M;
    SN_ProxyPushButton* _btn_Y;

    /*!
     * \brief _scrollbar on the bottom of the widget
     */
    SN_ProxyScrollBar* _scrollbar;

    void _updateLabel(const QColor &c);

private slots:
    /*!
      changes the label color to red
      */
    void buttonR();

    void buttonM();

    void buttonY();

    void scrollbarmoved(int);
};


#endif // IMAGEWIDGETPLUGIN_H
