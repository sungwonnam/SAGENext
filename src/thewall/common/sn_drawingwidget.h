#ifndef SN_DRAWINGWIDGET_H
#define SN_DRAWINGWIDGET_H

#include <QGraphicsWidget>
#include <QImage>

class SN_PolygonArrowPointer;

class SN_DrawingWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit SN_DrawingWidget(QGraphicsItem *parent = 0);

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	QImage _theCanvas;

signals:

public slots:
	void drawEllipse(const QRectF &r, const QColor &color);
};

#endif // SN_DRAWINGWIDGET_H
