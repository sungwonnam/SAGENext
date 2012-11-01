#ifndef MANDELBROTWIDGET_H
#define MANDELBROTWIDGET_H

//
// INCLUDEPATH needs to be set correctly
//
#include "applications/base/basewidget.h"
#include "applications/base/SN_plugininterface.h"

#include "mandelbrotrenderer.h"

#include <QtGui>

class MandelbrotExample : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(SN_PluginInterface)

public:
    MandelbrotExample();

    SN_BaseWidget * createInstance();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

    void handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

    void wheelEvent(QGraphicsSceneWheelEvent *event);

private slots:
    /*!
     * \brief updatePixmap converts QImage returned by the thread to Pixmap for drawing. This slot in invoked after the thread finishes computation.
     * \param image
     * \param scaleFactor
     */
    void updatePixmap(const QImage &image, double scaleFactor);

    /*!
     * \brief zoom changes scale factor and trigger the thread to re-render
     * \param zoomFactor
     */
    void zoom(double zoomFactor);

    inline void zoomIn() { zoom(0.5);}
    inline void zoomOut() { zoom(1.8);}

    void reset();

private:
    /*!
     * \brief m_init instantiates all the object
     */
    void m_init();

    /*!
     * \brief scroll moves the contents by make the thread re-rendering with new position
     * \param delta
     */
    void scroll(const QPoint &delta);
    void scroll(int deltaX, int deltaY);

    /*!
     * \brief thread that computes and render into a QImage
     * updatePixmap() slot will be invoked after the thread finishes rendering
     */
    MandelbrotRenderer thread;

    /*!
     * \brief pixmap that will be painted onto the screen
     */
    QPixmap pixmap;

    QPoint pixmapOffset;
    QPoint lastDragPos;
    double centerX;
    double centerY;
    double pixmapScale;
    double curScale;

    QPushButton* _resetBtn;
    QPushButton* _zoomInBtn;
    QPushButton* _zoomOutBtn;
    QPushButton* _closeBtn;
};

#endif // MANDELBROTWIDGET_H
