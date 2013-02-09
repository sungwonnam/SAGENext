#ifndef SAGENEXTVISBASEWIDGET_H
#define SAGENEXTVISBASEWIDGET_H

#include <QObject>
#include <QDebug>

#include "../base/basewidget.h"
#include "../base/SN_plugininterface.h"

class SageNextVisBaseWidget : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(SN_PluginInterface)
public:

        SageNextVisBaseWidget(Qt::WindowFlags wflags = Qt::Window);

        SageNextVisBaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);
};

#endif // SAGENEXTVISBASEWIDGET_H
