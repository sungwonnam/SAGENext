/*
 * Base class to display streams from an imagebuffer
 *
 */

#ifndef SN_DISPLAYSTREAMBASE_H
#define SN_DISPLAYSTREAMBASE_H

#undef max

#include "base/basewidget.h"
#include "imagebuffer.h"

#if defined(Q_OS_LINUX)
//#define GLEW_STATIC 1
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>

//#include <GL/glut.h>
//#include <GL/glext.h>
#elif defined(Q_OS_MAC)
#define GL_GLEXT_PROTOTYPES
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

/**
  below is for when sage app streams YUV
  */
#define GLSLVertexShader   1
#define GLSLFragmentShader 2
int GLSLreadShaderSource(char *fileName, GLchar **vertexShader, GLchar **fragmentShader);
GLuint GLSLinstallShaders(const GLchar *Vertex, const GLchar *Fragment);

class SN_DisplayStreamBase: public SN_BaseWidget
{
    Q_OBJECT
public:
    SN_DisplayStreamBase(const quint64 globalAppId, const QSettings* s, SN_ResourceMonitor* rm = 0, QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0, ImageBuffer* buf=0);
    ~SN_DisplayStreamBase();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void changeImage(QImage* img);

protected:
    QSize _imageSize;
    ImageBuffer* _sharedBuffer;
    bool _framePainted;
    QImage _currFrame;
};

#endif // SN_DisplayStreamBase_H
