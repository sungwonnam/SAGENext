#ifndef CALLINFORMATION_H
#define CALLINFORMATION_H

#include <QObject>
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppCallManager.h>
#include <qxmpp/QXmppConfiguration.h>


class VideoStreamClient : public QObject
{
    Q_OBJECT
public:
    VideoStreamClient();
    void connectToGTalk(const QString username, const QString password);
    ~VideoStreamClient();
signals:
    void haveNewCall(QXmppCall* call);
    void callTerminated(QXmppCall* call);
    void callStarted(QXmppCall* call);
    void callConnected();

public slots:
    void rosterReceived();
    void startNewCall(const QString user);
    void received_call_from_manager(QXmppCall* call);
    void call_Connected();
    void call_started(QXmppCall* call);

private:

    QXmppConfiguration _gtalkconfig;
    QXmppCallManager* _callmanager;
    QXmppClient* _client;
    QStringList _roster;
    bool _isConnected;

};

#endif // CALLINFORMATION_H
