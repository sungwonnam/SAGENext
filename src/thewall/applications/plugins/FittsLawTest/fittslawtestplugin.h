#ifndef FITTSLAWTESTPLUGIN_H
#define FITTSLAWTESTPLUGIN_H

#include "applications/base/basewidget.h"
#include "applications/base/SN_plugininterface.h"
#include "fittslawteststreamrecvthread.h"

#include <QtGui>

/*
class FittsLawTestMain : public SN_BaseWidget , SN_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(SN_PluginInterface)

public:
    FittsLawTestMain();
    ~FittsLawTestMain();

    SN_BaseWidget * createInstance() {return new FittsLawTestMain;}
};
*/



class FittsLawTest : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(SN_PluginInterface)

public:
    FittsLawTest();
    ~FittsLawTest();

    SN_BaseWidget * createInstance() {return new FittsLawTest;}

    /*!
      Only a user application will have the value >= UserType + BASEWIDGET_USER.
      So, please use a number greater than 12 for your own custom application if you're to reimplement this.

      Note that SAGENext-generic graphics items will use the value between UserType and UserType + 12
      e.g. SN_PartitionBar
     */
    enum { Type = QGraphicsItem::UserType + BASEWIDGET_USER + 76};
    int type() const { return Type;}

    void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

    /*!
      If clicked the initiating circle then hide SN_PolygonArrowPointer draw pointer within this application
      */
    bool handlePointerClick(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn);

    /*!
      If user clicked the initiating circle then pointer movement will be handled within this app
      */
    void handlePointerHover(SN_PolygonArrowPointer * pointer, const QPointF & point, bool isHovering);


    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
    /*!
      FittsLawTest is running.
      Uesrs is moving pointer and clicking dots
      */
    bool _isRunning;


    bool _isReady;

    /*!
      How many dots a user have to click
      */
    int _testCount;

    QGraphicsWidget *_contentWidget;

    QGraphicsRectItem *_initCircle;

    QGraphicsRectItem *_target;

    /*!
      cursor within the app
      */
//    QGraphicsPixmapItem *_handle;


    QPointF _cursorPoint;

    QGraphicsProxyWidget *_resetbutton;
    QGraphicsProxyWidget *_readybutton;
    QGraphicsProxyWidget *_donebutton;

    QPixmap _cursorPixmap;


    SN_PolygonArrowPointer *_myPointer;

    /*!
      Thread receiving image from external program
      */
    FittsLawTestStreamReceiver *_recvThread;

    /*!
      ssh process
      */
    QProcess *_extProgram;

    QString _streamerIpAddr;

    QSize _streamImageSize;

    /*!
      string representation of user id
      */
    QString _userID;

signals:
    /*!
      The user clicked the green circle.
      resume stream receiving and start showing targets
      */
    void initClicked();

    /*!
      when a single round finishes
      */
    void roundFinished();

    /*!
      user clicked the dot successfully
      */
    void hit();

    /*!
      user clicked but missed the dot
      */
    void miss();

public slots:
    void setSize(int w, int h);

    void setReady();

    void startRound();

    void determineNextTargetPosition();

    void resetTest();

    /*!
      write data to a file
      */
    void finishTest();

    /*!
      launches external program using ssh command.
      The external program will wait for connection from StreamRecvThread
      */
    void launchExternalStreamer(const QString &ipaddr);


    void scheduleUpdate();

    inline void setUserID(const QString &str) {_userID = str;}
};

#endif // FITTSLAWTESTPLUGIN_H
