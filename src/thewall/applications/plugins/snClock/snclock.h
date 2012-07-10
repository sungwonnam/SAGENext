#ifndef SNCLOCK_H
#define SNCLOCK_H

#include "applications/base/SN_plugininterface.h"
#include "applications/base/basewidget.h"

#include <QtGui>

class snClock : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(SN_PluginInterface)

public:
   snClock();
   virtual ~snClock();

   SN_BaseWidget * createInstance();

protected:

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


private:
    QLabel *time_label;
    QGraphicsProxyWidget *proxy_time_label;
    QGraphicsLinearLayout* layout;

    void m_init();

private slots:
    void update();

};


#endif // SNCLOCK_H

