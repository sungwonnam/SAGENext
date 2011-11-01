#include "commonitem.h"



SN_PixmapButton::SN_PixmapButton(const QString &res, qreal desiredWidth, const QString &label, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
	QPixmap orgPixmap(res);
	if (desiredWidth) {
		orgPixmap = orgPixmap.scaledToWidth(desiredWidth);
	}
	_normalPixmap = orgPixmap;
	QGraphicsPixmapItem *p = new QGraphicsPixmapItem(_normalPixmap, this);

	// This widget (PixmapButton) has to receive mouse event
	p->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
	resize(p->pixmap().size());
//	setOpacity(0.5);

	if (!label.isNull() && !label.isEmpty()) {
		_attachLabel(label, p);
	}
}
SN_PixmapButton::SN_PixmapButton(const QPixmap &pixmap, qreal desiredWidth, const QString &label, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
	QGraphicsPixmapItem *p = 0;
	if ( desiredWidth ) {
		p = new QGraphicsPixmapItem(pixmap.scaledToWidth(desiredWidth), this);
	}
	else {
		p = new QGraphicsPixmapItem(pixmap, this);
	}

	// This widget (PixmapButton) has to receive mouse event
	p->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
	resize(p->pixmap().size());
//	setOpacity(0.5);


	if (!label.isNull() && !label.isEmpty()) {
		_attachLabel(label, p);
	}
}

SN_PixmapButton::SN_PixmapButton(const QString &res, const QSize &size, const QString &label, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
{
	QPixmap pixmap(res);
	QGraphicsPixmapItem *p = new QGraphicsPixmapItem(pixmap.scaled(size), this);

	p->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
	resize(p->pixmap().size());

	if (!label.isNull() && !label.isEmpty()) {
		_attachLabel(label, p);
	}
}

SN_PixmapButton::~SN_PixmapButton() {
}

void SN_PixmapButton::_attachLabel(const QString &labeltext, QGraphicsItem *parent) {
	QGraphicsSimpleTextItem *t = new QGraphicsSimpleTextItem(labeltext, parent);
//	t->setTransformOriginPoint(t->boundingRect().center());

	QBrush brush(Qt::white);
	t->setBrush(brush);

//	QFont f;
//	f.setBold(true);
//	t->setFont(f);

	QPointF center_delat = boundingRect().center() - t->mapRectToParent(t->boundingRect()).center();
	t->moveBy(center_delat.x(), center_delat.y());
}

void SN_PixmapButton::mousePressEvent(QGraphicsSceneMouseEvent *) {
//	setOpacity(1);
}
void SN_PixmapButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
//	setOpacity(0.5);
//	qDebug() << "pixmapbutton emitting signal";
	emit clicked();
}






SN_SimpleTextItem::SN_SimpleTextItem(int ps, const QColor &fontcolor, const QColor &bgcolor, QGraphicsItem *parent)
	: QGraphicsSimpleTextItem(parent)
    , _fontcolor(fontcolor)
    , _bgcolor(bgcolor)
{
	if ( ps > 0 ) {
		QFont f;
		f.setStyleStrategy(QFont::OpenGLCompatible);
		f.setPointSize(ps);
		setFont(f);
	}
	setBrush(_fontcolor);
//	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}

SN_SimpleTextItem::~SN_SimpleTextItem() {
}
void SN_SimpleTextItem::wheelEvent(QGraphicsSceneWheelEvent *event) {
	int numDegrees = event->delta() / 8;
	int numTicks = numDegrees / 15;
	//	qDebug("SwSimpleTextItem::%s() : delta %d numDegrees %d numTicks %d", __FUNCTION__, event->delta(), numDegrees, numTicks);

	qreal s = scale();
	if ( numTicks > 0 ) {
		s += 0.1;
	}
	else {
		s -= 0.1;
	}
	//	prepareGeometryChange();
	setScale(s);
}

void SN_SimpleTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->fillRect(boundingRect(), QBrush(_bgcolor));
	QGraphicsSimpleTextItem::paint(painter, option, widget);
}







SN_SimpleTextWidget::SN_SimpleTextWidget(int pointSize, const QColor &fontcolor, const QColor &bgcolor, QGraphicsItem *parent)
	: QGraphicsWidget(parent, Qt::Widget)
	, _textItem(0)
{
	setAttribute(Qt::WA_DeleteOnClose, true);

	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setFlag(QGraphicsItem::ItemIsMovable, false);
	setFlag(QGraphicsItem::ItemHasNoContents, true);

	setAcceptedMouseButtons(0);

	_textItem = new SN_SimpleTextItem(pointSize, fontcolor, bgcolor, this);
//	resize(_textItem->boundingRect().size());
}

void SN_SimpleTextWidget::setText(const QString &text) {
	if (_textItem) {
		_textItem->setText(text);
		resize(_textItem->boundingRect().size());
	}
}







