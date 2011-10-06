#ifndef FILERECEIVINGTHREAD_H
#define FILERECEIVINGTHREAD_H

#include <QtCore>
#include <QTcpServer>

class SN_Launcher;



class FileReceivingTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit FileReceivingTcpServer(SN_Launcher *l, int recvport);

    ~FileReceivingTcpServer();


    void setFileInfo(const QString &fname, int fsize);

protected:
    void incomingConnection(int handle);

private:
    bool _end;

    SN_Launcher *_launcher;

//    QString _recvAddr;
    int _recvPort;

    QString _filename;
    int _filesize;

    QDir _destDir;

    /**
      to listen on
      */
    int _serversocket;

    /**
      to recv
      */
    int _recvsocket;

    QMutex mutex;

    void receivingFunction();

};

#endif // FILERECEIVINGTHREAD_H
