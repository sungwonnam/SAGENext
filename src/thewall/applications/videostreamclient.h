#ifndef CALLINFORMATION_H
#define CALLINFORMATION_H

#include <QObject>
#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppRosterManager.h>
#include <qxmpp/QXmppCallManager.h>
#include <qxmpp/QXmppPresence.h>
#include <qxmpp/QXmppConfiguration.h>

class VideoStreamClient : public QObject
{
    Q_OBJECT
public:
    VideoStreamClient();
    void connectToGTalk(const QString username, const QString password);
    ~VideoStreamClient();
signals:

public slots:
    void callReceived(QXmppCall* call);
    void callStarted(QXmppCall* call);
    void callConnected();
    void videoModeChanged(QIODevice::OpenMode);

private:

    QXmppConfiguration _gtalkconfig;

    QXmppCallManager* _callmanager;
    QXmppCall* _currCall;

    QXmppClient _client;
    QStringList _roster;
    bool _isConnected;

};

#endif // CALLINFORMATION_H
