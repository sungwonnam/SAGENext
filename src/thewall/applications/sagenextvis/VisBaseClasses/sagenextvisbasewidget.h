#ifndef SAGENEXTVISBASEWIDGET_H
#define SAGENEXTVISBASEWIDGET_H

#include <QObject>
#include <QDebug>
#include <QGraphicsItem>

#include "../../base/basewidget.h"


class SageNextVisBaseWidget : public SN_BaseWidget
{
    Q_OBJECT

public:

    SageNextVisBaseWidget( QGraphicsItem* parent = 0);
    
};

#endif // SAGENEXTVISBASEWIDGET_H
