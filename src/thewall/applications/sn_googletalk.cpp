#include "sn_googletalk.h"
#include "../common/commonitem.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"


SN_GoogleTalk::SN_GoogleTalk(const quint64 globalAppId, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    :SN_BaseWidget(globalAppId, s, parent, wFlags)
{
    resize(160, 140);

    _layout = new QGraphicsLinearLayout(this);

    _cameraBuffer = new ImageBuffer(100000, true);
    _sendStream = new SN_VideoStream(globalAppId, s, rm, parent, wFlags, _cameraBuffer);

    //_recvBuffer = new ImageBuffer(10000, true);
    //_recvStream = new SN_StreamDisplay(globalAppId, s, rm, parent, wFlags, _recvBuffer);


    QObject::connect(this, SIGNAL(drawImage(QImage*)), _sendStream,  SLOT(changeImage(QImage*)));
    QObject::connect(_cameraBuffer, SIGNAL(haveNewImage(QImage*)), this, SLOT(haveNewImage(QImage*)));

    _client = new VideoStreamClient();
    _client->connectToGTalk("siddharthhsathyam", "evltestgmailaccount");

    //_layout->addItem(_sendStream);
    //_layout->addItem(_recvStream);
    //setLayout(_layout);
}

SN_GoogleTalk::~SN_GoogleTalk(){

}

void SN_GoogleTalk::haveNewImage(QImage* f){
    qDebug() << "ChangeImage:: updating frame...";
    // send this image to the user here

    emit drawImage(f);
    update();
}


void SN_GoogleTalk::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    _sendStream->paint(painter, option, widget);
}
