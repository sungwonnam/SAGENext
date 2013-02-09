#include "vistestwidget.h"

VisTestWidget::VisTestWidget( )
    : SN_BaseWidget(Qt::Window)
{

}

VisTestWidget::~VisTestWidget()
{

}

void VisTestWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->drawRect( 0.0, 0.0, 100.0, 100.0 );
}
