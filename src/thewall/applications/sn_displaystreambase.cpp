#include "sn_displaystreambase.h"
#include "../common/commonitem.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"

SN_DisplayStreamBase::SN_DisplayStreamBase(const quint64 globalAppId, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags, ImageBuffer *buf)
    :SN_BaseWidget(globalAppId, s, parent, wFlags)
    , _framePainted(false)
    , _sharedBuffer(buf)
{
    //_appInfo->setMediaType(SAGENext::MEDIA_TYPE_VNC);
    //What is the widget type for this and how does it affect paint?
    //setWidgetType(SN_BaseWidget::Widget_RealTime);

    // TODO: Gotta change this later...not the right way to do this,
    // have to get this programatically...

    resize(160, 140);

}

SN_DisplayStreamBase::~SN_DisplayStreamBase(){

}

void SN_DisplayStreamBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (_perfMon) {
        _perfMon->getDrawTimer().start();
    }
    //QVideoFrame frame;

    //if(_sharedBuffer->getSizeOfImageBuffer() > 0){
      //  frame = _sharedBuffer->getFrame();
    //}

    if (_currFrame.size().height() > 0){
        /*if (painter->paintEngine()->type() == QPaintEngine::OpenGL2){
            // Copied this native painted straight from sagestreamwidget...idk about all the black magic going on here
            painter->beginNativePainting();

            if(_useShader){
                glUseProgramObjectARB(_shaderProgHandle);
                glActiveTexture(GL_TEXTURE0);
                int h=glGetUniformLocationARB(_shaderProgHandle,"yuvtex");
                glUniform1iARB(h,0);  /* Bind yuvtex to texture unit 0
            }

            glDisable(GL_TEXTURE_2D);
            glEnable(GL_TEXTURE_RECTANGLE_ARB);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textureid);

            glBegin(GL_QUADS);

            glTexCoord2f(0.0,            size().height()); glVertex2f(0,              size().height());
            glTexCoord2f(size().width(), size().height()); glVertex2f(size().width(), size().height());
            glTexCoord2f(size().width(), 0.0);             glVertex2f(size().width(), 0);
            glTexCoord2f(0.0,            0.0);             glVertex2f(0,              0);

            glEn
            qDebug() << "Paint-> _imageSize.width(): " << _imageSize.width();
            qDebug() << "Paint-> _imageSize.height(): " << _imageSize.height();
            //QImage m(_currFrame.bits(), _imageSize.width(), _imageSize.height(),_imageFormat);

        }else{*/
                painter->drawImage(QPoint(0,0), _currFrame);
                //painter->setTransform(oldTransform);
                _framePainted = true;
                // _currFrame.unmap();
                //}

    }

    if (_perfMon)
        _perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called

}

void SN_DisplayStreamBase::changeImage(QImage *img){
    _currFrame = *(img);
}



