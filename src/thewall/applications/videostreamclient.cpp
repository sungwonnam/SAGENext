#include "videostreamclient.h"
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
    QObject()
{
    _client = new QXmppClient(this);
    _isConnected = false;
    _callmanager = new QXmppCallManager;
    _callmanager->setStunServer(QHostAddress("stun.l.google.com"),19302);
    _client->addExtension(_callmanager);

    QObject::connect(_callmanager,
                     SIGNAL(callReceived(QXmppCall *)),
                     this,
                     SLOT(callRecvd(QXmppCall*)));

    QObject::connect(_callmanager,
                     SIGNAL(callStarted(QXmppCall *)),
                     this,
                     SLOT(callStd(QXmppCall *)));


    QXmppRosterManager* rst = _client->findExtension<QXmppRosterManager>();

    if(rst){
        QObject::connect(rst,
                         SIGNAL(rosterReceived()),
                         this,
                         SLOT(rosterReceived()));
    }

    _client->logger()->setLoggingType(QXmppLogger::StdoutLogging);
    _gtalkconfig.setPort(5222);
    _gtalkconfig.setDomain("gmail.com");
    _gtalkconfig.setHost("talk.google.com");
}

void VideoStreamClient::connectToGTalk(const QString username, const QString password){
    _gtalkconfig.setUser(username);
    _gtalkconfig.setPassword(password);
    //p.setAvailableStatusType(QXmppPresence::Online);
    // EXPERIMENTAL: TRY AND SET THE GOOGLE CAPABILITY FOR VOICE + VIDEO
    _client->connectToServer(_gtalkconfig);
}

void VideoStreamClient::rosterReceived(){
    _client->sendPacket(GVideoPresence());
}

void VideoStreamClient::recieved_call_from_manager(QXmppCall *call){
    emit haveNewCall(call);
}

void VideoStreamClient::call_Connected(){
    emit callConnected();
}

void VideoStreamClient::startNewCall(const QString user){

}

VideoStreamClient::~VideoStreamClient(){
    // delete stuff here
    delete _callmanager;
    delete _client;
}
