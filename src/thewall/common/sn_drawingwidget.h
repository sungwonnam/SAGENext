#ifndef SN_DRAWINGWIDGET_H
#define SN_DRAWINGWIDGET_H

#include <QGraphicsWidget>
#include <QImage>
#include "../applications/base/basewidget.h"

class SN_PolygonArrowPointer;

class SN_DrawingWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit SN_DrawingWidget(QGraphicsItem *parent = 0);

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	/**
	  The size of _theCanvas is determined here.
	  The reason it isn't determined in the constructor is the size info comes from the scene.
	  The graphics object can't know about the scene until it is instantiated.
	  */
	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	QImage _theCanvas;

signals:

public slots:
	void drawEllipse(const QRectF &r, const QColor &color, bool clear = false);
};


class SN_PixmapButton;

class SN_DrawingTool : public SN_BaseWidget
{
	Q_OBJECT
public:
	explicit SN_DrawingTool(quint64 gaid, const QSettings *s, QGraphicsItem *parent=0, Qt::WindowFlags wf = 0);

	/**
	  This widget doesn't resize
	  */
	QRectF resizeHandleSceneRect() {return QRectF();}

	void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
	SN_PixmapButton *_brushIcon;
	SN_PixmapButton *_eraserIcon;
	SN_PixmapButton *_pointerIcon;
};

#endif // SN_DRAWINGWIDGET_H
