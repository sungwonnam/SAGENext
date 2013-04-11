#ifndef MANDELBROTRENDERER_H
#define MANDELBROTRENDERER_H

#include <QtCore>

class MandelbrotRenderer : public QThread
{
    Q_OBJECT

public:
    MandelbrotRenderer(QObject *parent=0);
    ~MandelbrotRenderer();

    void render(double centerX, double centerY, double scaleFactor, QSize resultSize);

protected:
    void run();

private:
    uint rgbFromWaveLength(double wave);

    QMutex mutex;
    QWaitCondition condition;
    double centerX;
    double centerY;
    double scaleFactor;
    QSize resultSize;
    bool restart;
    bool abort;

    enum { ColormapSize = 512 };
    uint colormap[ColormapSize];

signals:
    void renderedImage(const QImage &image, double scaleFactor);
};

#endif // MANDELBROTRENDERER_H
