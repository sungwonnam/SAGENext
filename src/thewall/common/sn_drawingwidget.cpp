#include "sn_drawingwidget.h"

#include <QtGui>

#include "sn_sharedpointer.h"

SN_DrawingWidget::SN_DrawingWidget(QGraphicsItem *parent)
	: QGraphicsWidget(parent)
{
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemIsSelectable, false);

	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_NoSystemBackground);
	setAcceptedMouseButtons(0);

	setContentsMargins(0,0,0,0);
	setWindowFrameMargins(0,0,0,0);
}

void SN_DrawingWidget::resizeEvent(QGraphicsSceneResizeEvent *event) {
	_theCanvas = QImage(event->newSize().toSize(), QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&_theCanvas);
	QBrush b(QColor(0, 0, 0, 0)); // 0 % transparent
	painter.fillRect(0, 0, _theCanvas.width(), _theCanvas.height(), b);
}

void SN_DrawingWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->drawImage(0,0,_theCanvas);
}

void SN_DrawingWidget::drawEllipse(const QRectF &r, const QColor &color) {
	QPainter painter(&_theCanvas);

//	QPen pen(b, 16);
//	painter.setPen(pen);
//	painter.drawLine(oldp, newp);

	painter.setPen(Qt::NoPen);
	painter.setBrush(color);
	painter.drawEllipse(r);
	update();
}
