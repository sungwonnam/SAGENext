#ifndef IMAGEWIDGETPLUGIN_H
#define IMAGEWIDGETPLUGIN_H

#include "applications/base/SN_plugininterface.h"
#include "applications/base/sn_basewidget.h"

#include <QtGui>

class SN_ProxyPushButton;
class SN_ProxyScrollBar;

/*!
  \brief A simple SAGENext plugin example.

  This example shows how to use SAGENext's GUI components
  defined in src/common/sn_commonitem.h/cpp
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
      \brief A pure virtual function that must be reimplemented.
      \return an instance of this class

      The SN_Launcher calls this function to create an instance of this plugin.
      */
	SN_BaseWidget * createInstance();

private:
    /*!
     * \brief creates GUI components
     *
     * It is highly recommend to create GUI instances NOT in the constructor.
     * Because an application instance of this class is created when the createInstance()
     * is called. By not creating GUI components in the constructor you can avoid
     * unnecessary memory usage.
     */
    void _createGUIs();

    /*!
     * \brief _label simply displays a color
     */
    QLabel *_label;

    /*!
      The color of the label
      */
    QColor _currentColor;

    SN_ProxyPushButton* _btn_R; /*!< SAGENext's button type to color the label red */
    SN_ProxyPushButton* _btn_M; /*!< SAGENext's button type to color the label magenta */
    SN_ProxyPushButton* _btn_Y; /*!< SAGENext's button type to color the label yellow */

    /*!
     * \brief _scrollbar on the bottom of the widget that changes grayscale
     */
    SN_ProxyScrollBar* _scrollbar;

    /*!
     * \brief _updateLabel changes the color of the label
     * \param QColor
     */
    void _updateLabel(const QColor &c);

private slots:
    /*!
     * \brief buttonR changes the state of the buttons and update label's color
     */
    void buttonR();

    void buttonM();

    void buttonY();

    /*!
     * \brief scrollbarmoved updates the label's color value
     */
    void scrollbarmoved(int);
};


#endif // IMAGEWIDGETPLUGIN_H
