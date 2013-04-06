#include "mandelbrotwidget.h"

#include <QtGui>


//! [0]
const double DefaultCenterX = -0.637011f;
const double DefaultCenterY = -0.0395159f;
const double DefaultScale = 0.00403897f;

const double ZoomInFactor = 1.8f;
const double ZoomOutFactor = 1 / ZoomInFactor;
const int ScrollStep = 20;
//! [0]

//! [1]
MandelbrotExample::MandelbrotExample()

    : SN_BaseWidget(Qt::Window)

    , centerX(-0.637011f)
    , centerY(-0.0395159f)
    , pixmapScale(0.00403897f)
    , curScale(0.00403897f)

    , _resetBtn(0)
    , _zoomInBtn(0)
    , _zoomOutBtn(0)
    , _closeBtn(0)
{
}
//! [1]

void MandelbrotExample::m_init() {
    qRegisterMetaType<QImage>("QImage");

    QObject::connect(&thread, SIGNAL(renderedImage(QImage,double)), this, SLOT(updatePixmap(QImage,double)));

    setContentsMargins(15, 40, 15, 15);

    _resetBtn = new QPushButton("Reset");
    QObject::connect(_resetBtn, SIGNAL(clicked()), this, SLOT(reset()));

    _zoomInBtn = new QPushButton("Zoom In");
    QObject::connect(_zoomInBtn, SIGNAL(clicked()), this, SLOT(zoomIn()));

    _zoomOutBtn = new QPushButton("Zoom Out");
    QObject::connect(_zoomOutBtn, SIGNAL(clicked()), this, SLOT(zoomOut()));

    _closeBtn = new QPushButton("Close");
    QObject::connect(_closeBtn, SIGNAL(clicked()), this, SLOT(close()));

    QGraphicsProxyWidget* resetProxy = new QGraphicsProxyWidget(this);
    resetProxy->setWidget(_resetBtn);
    resetProxy->setPos(15, 40);

    QGraphicsProxyWidget* ziProxy = new QGraphicsProxyWidget(this);
    ziProxy->setWidget(_zoomInBtn);
    ziProxy->setPos(resetProxy->pos() + QPointF(resetProxy->size().width(), 0));

    QGraphicsProxyWidget* zoProxy = new QGraphicsProxyWidget(this);
    zoProxy->setWidget(_zoomOutBtn);
    zoProxy->setPos(ziProxy->pos() + QPointF(ziProxy->size().width(), 0));

    QGraphicsProxyWidget* closeProxy = new QGraphicsProxyWidget(this);
    closeProxy->setWidget(_closeBtn);
    closeProxy->setPos(zoProxy->pos() + QPointF(zoProxy->size().width(), 0));
}

void MandelbrotExample::reset() {
    centerX= -0.637011f;
    centerY= -0.0395159f;
    pixmapScale = 0.00403897f;
    curScale = 0.00403897f;

    thread.render(centerX, centerY, curScale, size().toSize() - QSize(30, 55));
}

SN_BaseWidget * MandelbrotExample::createInstance() {
    MandelbrotExample *bw = new MandelbrotExample;
    bw->m_init();

    //
    // This will make the render thread starts
    //
    bw->resize(1024, 768);
    return bw;
}

//! [2]
void MandelbrotExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // offset content margin left and top
    painter->drawPixmap(15, 40, pixmap);
}
//! [9]

//! [10]
void MandelbrotExample::resizeEvent(QGraphicsSceneResizeEvent *e)
{
    thread.render(centerX, centerY, curScale, e->newSize().toSize() - QSize(30, 55) );
}
//! [10]

/*
//! [11]
void MandelbrotExample::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoom(ZoomInFactor);
        break;
    case Qt::Key_Minus:
        zoom(ZoomOutFactor);
        break;
    case Qt::Key_Left:
        scroll(-ScrollStep, 0);
        break;
    case Qt::Key_Right:
        scroll(+ScrollStep, 0);
        break;
    case Qt::Key_Down:
        scroll(0, -ScrollStep);
        break;
    case Qt::Key_Up:
        scroll(0, +ScrollStep);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
//! [11]
*/

//! [12]
void MandelbrotExample::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    int numDegrees = event->delta() / 8;
    double numSteps = numDegrees / 15.0f;
    zoom(pow(ZoomInFactor, numSteps));
}
//! [12]

//! [13]
void MandelbrotExample::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn)
{
    if (btn == Qt::LeftButton) {

        //
        // the rectangle of the content area
        //
        QRectF content(15, 40, size().width()-30, size().height()-55);

        //
        // if the pointer button is pressed on the content region
        // then this interaction isn't for window moving
        //
        if (content.contains(point)) {
            lastDragPos = point.toPoint(); // is about to start dragging from this pressed point

            //
            // It's for interaction. Not for window moving
            //
            _isMoving = false;
        }

        //
        // if pointer button is pressed on the window frame
        // keep the base implementatoin
        //
        else {
            SN_BaseWidget::handlePointerPress(pointer, point, btn);
        }
    }
}
//! [13]

//! [14]
void MandelbrotExample::handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier)
{
    //
    // if the pointer press was on the content area
    // then interact with the visualization
    //
    if (!_isMoving && !_isResizing) {

        if (button == Qt::LeftButton) {

            pixmapOffset = lastDragPos - point.toPoint();
            lastDragPos = point.toPoint(); // the pointer's last position

            scroll(pixmapOffset);
        }
    }
    else {
        SN_BaseWidget::handlePointerDrag(pointer, point, pointerDeltaX, pointerDeltaY, button, modifier);
    }
}


//! [16]
void MandelbrotExample::updatePixmap(const QImage &image, double scaleFactor)
{
//    _pixmap->setPixmap(QPixmap::fromImage(image));
//    _pixmap->setOffset(QPointF());

    pixmap = QPixmap::fromImage(image);

    pixmapScale = scaleFactor;

    update();
}
//! [16]

//! [17]
void MandelbrotExample::zoom(double zoomFactor)
{
    curScale *= zoomFactor;

    // compute again
    thread.render(centerX, centerY, curScale, size().toSize() - QSize(30, 55));
}
//! [17]

void MandelbrotExample::scroll(const QPoint &delta) {
    scroll(delta.x(), delta.y());
}

//! [18]
void MandelbrotExample::scroll(int deltaX, int deltaY)
{
    centerX += (deltaX * curScale);
    centerY += (deltaY * curScale);

    thread.render(centerX, centerY, curScale, size().toSize() - QSize(30, 55));
}
//! [18]

#ifndef QT5
Q_EXPORT_PLUGIN2(MandelbrotExamplePlugin, MandelbrotExample)
#endif
