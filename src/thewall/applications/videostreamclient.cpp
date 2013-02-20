#include "videostreamclient.h"
//#include "googletalkpresence.h"
#include "gvideopresence.h"
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppCallManager.h>
#include <qxmpp/QXmppPresence.h>
#include <qxmpp/QXmppConfiguration.h>
#include <qxmpp/QXmppLogger.h>
#include <qxmpp/QXmppDiscoveryManager.h>
#include <qxmpp/QXmppUtils.h>
#include <QHostAddress>

VideoStreamClient::VideoStreamClient() :
    QObject(), _isConnected(false)
{
    _callmanager = new QXmppCallManager;
    _callmanager->setStunServer(QHostAddress("stun.l.google.com"), 19302);
    _client.addExtension(_callmanager);

    QObject::connect(_callmanager,
                     SIGNAL(callReceived(QXmppCall *)),
                     this,
                     SLOT(callReceived(QXmppCall *)));

    QObject::connect(_callmanager,
                     SIGNAL(callStarted(QXmppCall *)),
                     this,
                     SLOT(callStarted(QXmppCall *)));


    QXmppRosterManager* rst = _client.findExtension<QXmppRosterManager>();
    if(rst){
        QObject::connect(rst,
                         SIGNAL(rosterReceived()),
                         this,
                         SLOT(rosterReceived()));
    }

    _client.logger()->setLoggingType(QXmppLogger::StdoutLogging);
    _gtalkconfig.setPort(5222);
    _gtalkconfig.setDomain("gmail.com");
    _gtalkconfig.setHost("talk.google.com");
    _gtalkconfig.setStreamSecurityMode(_gtalkconfig.TLSRequired);
}

void VideoStreamClient::connectToGTalk(const QString username, const QString password){
    _gtalkconfig.setUser(username);
    _gtalkconfig.setPassword(password);
    //p.setAvailableStatusType(QXmppPresence::Online);
    // EXPERIMENTAL: TRY AND SET THE GOOGLE CAPABILITY FOR VOICE + VIDEO
    _client.connectToServer(_gtalkconfig, GVideoPresence());
}

void VideoStreamClient::rosterReceived(){
    _client.sendPacket(GVideoPresence());
}

void VideoStreamClient::callReceived(QXmppCall *call){
    _currCall = call;

    QObject::connect(_currCall,
                     SIGNAL(connected()),
                     this,
                     SLOT(callConnected()));


    QObject::connect(_currCall, SIGNAL(videoModeChanged(QIODevice::OpenMode)), this, SLOT(videoModeChanged(QIODevice::OpenMode)));

    call->accept();
}

void VideoStreamClient::callStarted(QXmppCall *call){
    _currCall = call;
}

void VideoStreamClient::callConnected(){
    if (_currCall->direction() == QXmppCall::OutgoingDirection)
            _currCall->startVideo();

}

void VideoStreamClient::videoModeChanged(QIODevice::OpenMode){
    int i = 0;
}

VideoStreamClient::~VideoStreamClient(){
    // delete stuff here
}
