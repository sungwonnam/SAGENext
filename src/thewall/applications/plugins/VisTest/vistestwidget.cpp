#include "vistestwidget.h"

VisTestWidget::VisTestWidget( QGraphicsItem* parent )
  : VisBaseWidget( parent )
{

}

VisTestWidget::~VisTestWidget()
{

}

/**
  Implementing the interface.
  All the child items will be instantiated in here
*/
SN_BaseWidget * VisTestWidget::createInstance() 
{

  VisTestWidget *instance = new VisTestWidget;

  return instance;
}

void VisTestWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->drawRect( 0.0, 0.0, 100.0, 100.0 );
}

Q_EXPORT_PLUGIN2(VisTest, VisTestWidget)
