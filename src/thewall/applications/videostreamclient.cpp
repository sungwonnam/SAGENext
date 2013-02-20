#include "videostreamclient.h"

#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppCallManager.h>
#include <qxmpp/QXmppPresence.h>
#include <qxmpp/QXmppConfiguration.h>
#include <qxmpp/QXmppLogger.h>
#include <qxmpp/QXmppDiscoveryManager.h>
#include <qxmpp/QXmppUtils.h>

VideoStreamClient::VideoStreamClient() :
    QObject(), _isConnected(false)
{
    _callmanager = new QXmppCallManager();

    _client.addExtension(_callmanager);

    QObject::connect(_callmanager,
                     SIGNAL(callReceived(QXmppCall *)),
                     this,
                     SLOT(callReceived(QXmppCall *)));

    QObject::connect(_callmanager,
                     SIGNAL(callStarted(QXmppCall *)),
                     this,
                     SLOT(callStarted(QXmppCall *)));

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

    _client.connectToServer(_gtalkconfig);


    _isConnected = _client.isConnected();
    qDebug() << "connectToGTalk isAuthenticated" << _client.isAuthenticated();
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

}

VideoStreamClient::~VideoStreamClient(){
    // delete stuff here
}
