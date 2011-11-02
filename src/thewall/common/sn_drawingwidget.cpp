#include "sn_drawingwidget.h"

#include <QtGui>

#include "sn_sharedpointer.h"
#include "commonitem.h"

SN_DrawingWidget::SN_DrawingWidget(QGraphicsItem *parent)
	: QGraphicsWidget(parent)
{
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemIsSelectable, false);

//	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_NoSystemBackground);
	setAcceptedMouseButtons(0);

	setContentsMargins(0,0,0,0);
	setWindowFrameMargins(0,0,0,0);

	setZValue(10000);
}

void SN_DrawingWidget::resizeEvent(QGraphicsSceneResizeEvent *event) {
	_theCanvas = QImage(event->newSize().toSize(), QImage::Format_ARGB32_Premultiplied);
//	_theCanvas = QPixmap(event->newSize().toSize());
	QPainter painter(&_theCanvas);
	QBrush b(QColor(0, 0, 0, 0));
	painter.fillRect(0, 0, _theCanvas.width(), _theCanvas.height(), b);
}

void SN_DrawingWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

//	qint64 s = QDateTime::currentMSecsSinceEpoch();

	painter->drawImage(0,0,_theCanvas);
//	painter->drawPixmap(0, 0, _theCanvas);

//	qint64 e = QDateTime::currentMSecsSinceEpoch();

//	qDebug() << e - s << "msec";
}

void SN_DrawingWidget::drawEllipse(const QRectF &r, const QColor &color, bool clear) {
	QPainter painter(&_theCanvas); // will this be ok ? this function is called quite frequently.

//	QPen pen(b, 16);
//	painter.setPen(pen);
//	painter.drawLine(oldp, newp);

	painter.setPen(Qt::NoPen);

	if (clear) {
		painter.setCompositionMode(QPainter::CompositionMode_Clear);
	}
	painter.setBrush(color);
	painter.drawEllipse(r);
	update();
}

void SN_DrawingWidget::erase(const QPointF &p, const QSizeF &s) {
	QPainter painter(&_theCanvas);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0,0,0,0));

	painter.setCompositionMode(QPainter::CompositionMode_Clear);

	painter.drawEllipse(p, s.width(), s.height());
	update();
}


void SN_DrawingWidget::drawLine(const QPointF &oldp, const QPointF &newp, const QColor &color, int penwidth) {
	QPainter painter(&_theCanvas);

	QPen pen;
	pen.setColor(color);
	pen.setWidth(penwidth);

	painter.setPen(pen);
	painter.drawLine(oldp, newp);
	update();
}




SN_DrawingTool::SN_DrawingTool(quint64 gaid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wf)
    : SN_BaseWidget(gaid, s, parent, wf)
    , _brushIcon(0)
    , _eraserIcon(0)
    , _pointerIcon(0)
{
//	setWindowFrameMargins(0,0,0,0);
	setContentsMargins(1, 64, 1, 1); // 64 pixel at the top


	_pointerIcon = new SN_PixmapButton(":/resources/cursor_arrow_48x48.png", 0, "", this);
	_brushIcon = new SN_PixmapButton(":/resources/brush_48x48.png", 0, "", this);
	_eraserIcon = new SN_PixmapButton(":/resources/eraser_48x48.png", 0, "", this);

	//
	// these buttons don't need to receive mouse events.
	// Instead this widget must be visible when SN_PolygonArrowPointer::setAppUnderPointer() is called.
	//
	//
	_pointerIcon->setFlag(QGraphicsItem::ItemStacksBehindParent);
	_brushIcon->setFlag(QGraphicsItem::ItemStacksBehindParent);
	_eraserIcon->setFlag(QGraphicsItem::ItemStacksBehindParent);

	QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal, this);
	layout->setContentsMargins(10,10,10,10);

	layout->addItem(_pointerIcon);
	layout->addItem(_brushIcon);
	layout->addItem(_eraserIcon);

	setLayout(layout);
}

void SN_DrawingTool::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
	if (!pointer) return;

	if (btn == Qt::LeftButton) {

		if (_pointerIcon->geometry().contains(point)) {
			pointer->setDrawing(false);
			pointer->setErasing(false);
		}
		else if (_brushIcon->geometry().contains(point)) {
			pointer->setDrawing();
		}
		else if (_eraserIcon->geometry().contains(point)) {
			pointer->setErasing();
		}
		else {
			// do nothing
		}
	}
}


