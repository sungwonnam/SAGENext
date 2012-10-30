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

//    , _pixmap(new QGraphicsPixmapItem(this))

    , centerX(-0.637011f)
    , centerY(-0.0395159f)
    , pixmapScale(0.00403897f)
    , curScale(0.00403897f)
{
    qRegisterMetaType<QImage>("QImage");

    QObject::connect(&thread, SIGNAL(renderedImage(QImage,double)), this, SLOT(updatePixmap(QImage,double)));

    setContentsMargins(15, 40, 15, 15);

    setWindowTitle(tr("Mandelbrot"));
#ifndef QT_NO_CURSOR
    setCursor(Qt::CrossCursor);
#endif

//    qDebug() << "MandelbrotExample::MandelbrotExample() : boundingRect() " << boundingRect();
}
//! [1]

void MandelbrotExample::m_init() {
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Vertical, this);

    setLayout(layout);
}

SN_BaseWidget * MandelbrotExample::createInstance() {
    MandelbrotExample *bw = new MandelbrotExample;
    bw->m_init();

    //
    // This will make thread starts
    //
    bw->resize(550, 400);
    return bw;
}

//! [2]
void MandelbrotExample::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
//    painter->fillRect(rect(), Qt::black);
//    if (pixmap.isNull()) {
//        painter->setPen(Qt::white);
//        painter->drawText(rect(), Qt::AlignCenter, tr("Rendering initial image, please wait..."));
//        return;
//    }

    QRectF target(15, 40, size().width() - 30, size().height() -55);

    if (curScale == pixmapScale) {
        painter->drawPixmap(15, 40, /*pixmapOffset*/ pixmap);
    }

    else {
        double scaleFactor = pixmapScale / curScale;
        int newWidth = int(pixmap.width() * scaleFactor);
        int newHeight = int(pixmap.height() * scaleFactor);
        int newX = pixmapOffset.x() + (pixmap.width() - newWidth) / 2;
        int newY = pixmapOffset.y() + (pixmap.height() - newHeight) / 2;

        painter->save();
        painter->translate(newX, newY);
        painter->scale(scaleFactor, scaleFactor);

        QRectF exposed = painter->matrix().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);

//        qDebug() << "exposed" << exposed;

        painter->drawPixmap(target, pixmap, exposed);

        painter->restore();
    }
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
            lastDragPos = point.toPoint();

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
            pixmapOffset += point.toPoint() - lastDragPos;
            lastDragPos = point.toPoint();
            update();
        }
    }
    else {
        SN_BaseWidget::handlePointerDrag(pointer, point, pointerDeltaX, pointerDeltaY, button, modifier);
    }
}

void MandelbrotExample::handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn)
{
    if (btn == Qt::LeftButton) {

        if (!_isMoving && !_isResizing) {

            pixmapOffset += point.toPoint() - lastDragPos;
            lastDragPos = QPoint();

            int deltaX = (size().width() - pixmap.width()) / 2 - pixmapOffset.x();
            int deltaY = (size().height() - pixmap.height()) / 2 - pixmapOffset.y();
            scroll(deltaX, deltaY);
        }
    }

    //
    // _isMoving , _isResizing will be reset here
    //
    SN_BaseWidget::handlePointerRelease(pointer, point, btn);
}

//! [16]
void MandelbrotExample::updatePixmap(const QImage &image, double scaleFactor)
{
    if (!lastDragPos.isNull())
        return;

//    _pixmap->setPixmap(QPixmap::fromImage(image));
//    _pixmap->setOffset(QPointF());

    pixmap = QPixmap::fromImage(image);
    pixmapOffset = QPoint();
    lastDragPos = QPoint();

    pixmapScale = scaleFactor;

    update();
}
//! [16]

//! [17]
void MandelbrotExample::zoom(double zoomFactor)
{
    curScale *= zoomFactor;
//    update();
    thread.render(centerX, centerY, curScale, size().toSize() - QSize(30, 55));
}
//! [17]

//! [18]
void MandelbrotExample::scroll(int deltaX, int deltaY)
{
    centerX += deltaX * curScale;
    centerY += deltaY * curScale;
    update();
    thread.render(centerX, centerY, curScale, size().toSize() - QSize(30, 55));
}
//! [18]

Q_EXPORT_PLUGIN2(MandelbrotExamplePlugin, MandelbrotExample)
