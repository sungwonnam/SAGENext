#include "visbasewidget.h"

VisBaseWidget::VisBaseWidget(QGraphicsItem *parent )
    : SageNextVisBaseWidget(parent)
{

}

void VisBaseWidget::addSageVisParent( SageVis* sv )
{
     sageVisParent = sv;
}

void VisBaseWidget::addDataFiles(QStringList df )
{
    dataFiles = df;
}
