#ifndef MANDELBROTWIDGET_H
#define MANDELBROTWIDGET_H

//
// INCLUDEPATH needs to be set correctly
//
#include "applications/base/basewidget.h"
#include "applications/base/SN_plugininterface.h"

#include "mandelbrotrenderer.h"


class MandelbrotExample : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(SN_PluginInterface)

public:
    MandelbrotExample();

    SN_BaseWidget * createInstance() {return new MandelbrotExample;}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

    void handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

    void handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

    void wheelEvent(QGraphicsSceneWheelEvent *event);

private slots:
    void updatePixmap(const QImage &image, double scaleFactor);
    void zoom(double zoomFactor);

private:
    void scroll(int deltaX, int deltaY);

    MandelbrotRenderer thread;

//    QGraphicsPixmapItem *_pixmap;
    QPixmap pixmap;

    QPoint pixmapOffset;
    QPoint lastDragPos;
    double centerX;
    double centerY;
    double pixmapScale;
    double curScale;
};

#endif // MANDELBROTWIDGET_H
