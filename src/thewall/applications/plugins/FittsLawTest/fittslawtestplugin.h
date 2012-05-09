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


typedef struct {
    QList<int> misscount;
    QList<qint64> hitlatency; // msec
} Data;



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
    void handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button, Qt::KeyboardModifier modifier);


    /*!
      Draw the cursor
      */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event);

    /*!
      The base implementation doesn maximize/restore.
      I'm doing nothing.
      */
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);

    /*!
      keep track of cumulative byte received to find out

      Resource required (d/dt of cumulative resource usage)
      Current resource consumption (  currentBandwidth )
      */
    void timerEvent(QTimerEvent *);

private:
    /*!
      How many round
      */
    int _NUM_ROUND;

    /*!
      How may targets will appear in a round
      */
    int _NUM_TARGET_PER_ROUND;

    /*!
      Whether the test is running.
      Uesrs is moving pointer and clicking targets.

      Recv() thread will run.
      */
    bool _isRunning;


    /*!
      User can now click the green icon to start the test
      */
    bool _isReady;

    /*!
      How many targets a user successfully clicked so far
      */
    int _targetCount;

    /*!
      How many rounds completed so far
      */
    int _roundCount;

    /*!
      A Dummy widget where targets will appear
      */
    QGraphicsWidget *_contentWidget;

    QGraphicsSimpleTextItem *_simpleText;

    /*!
      green icon or stop sign
      */
    QGraphicsPixmapItem *_startstop;

    QPixmap _startPixmap;
    QPixmap _stopPixmap;

    /*!
      This is the target user will click
      */
    QGraphicsRectItem *_target;


    /*!
      user's mouse movement will update this value
      _cursorPixmap will be drawin in paint() on this position
      */
    QPointF _cursorPoint;

    /*!
      buttons
      */
    QGraphicsProxyWidget *_resetRoundButton;
    QGraphicsProxyWidget *_resetTargetPosList;
    QGraphicsProxyWidget *_readybutton;
    QGraphicsProxyWidget *_donebutton;
    QGraphicsProxyWidget *_initbutton;

    /*!
      lineEdits
      */
    QGraphicsProxyWidget *_numRound;
    QGraphicsProxyWidget *_numTargets;
    QGraphicsProxyWidget *_userid;

    /*!
      A pixmap of cursor during the test
      */
    QPixmap _cursorPixmap;


    SN_PolygonArrowPointer *_myPointer;

    /*!
      Thread receiving image from external program
      */
    FittsLawTestStreamReceiverThread *_recvThread;



    QThread _thread;
    FittsLawTestStreamReceiver *_recvObject;



    /*!
      The ssh process of streamer program
      */
    QProcess *_extProgram;

    QString _streamerIpAddr;

    /*!
      This is streaming overhead.
      So, the buffer size is width * height * 3
      */
    QSize _streamImageSize;

    /*!
      string representation of user id
      */
    QString _userID;


    /*!
      QPointF of 10 targets of a single round
      */
    QList<QPointF> _targetPosList;


    /*!
      Decide next target's position randomly
      */
    QPointF m_getRandomPos();

    /*!
      The miss count per target
      */
    int _missCount;

    qint64 _targetAppearTime;
    qint64 _targetHitTime;

    Data _data;

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
    void init();

    /*!
      This slot resizes the widget.
      Note that the size of the widget doesn't affect streaming overhead.
      */
    void setSize(int w, int h);

    /*!
      Upon clicking (with system's mouse not the shared pointer) the ready button.
      If the external program (streamer) isn't running then it's going to be fired in here.
      */
    void setReady();

    /*!
      A round consists of 10 targets.
      The streaming thread will resume.
      */
    void startRound();

    /*!
      The streaming thread will stop
      */
    void finishRound();

    /*!
      If there's no saved position then randomly choose one by calling m_getRandomPos()
      otherwise, use the saved value in the _targetPosList.
      */
    void determineNextTargetPosition();

    /*!
      reset _roundCount
      */
    void resetTest();

    void clearTargetPosList();

    void clearData();

    /*!
      A test consists of 4 rounds.

      write data to a file
      */
    void finishTest();

    /*!
      launches external program using ssh command.
      The external program will wait for connection from StreamRecvThread
      */
    void launchExternalStreamer(const QString &ipaddr);

    /*!
      This slot will just call update()
      */
    void scheduleUpdate();

    inline void setUserID(const QString &str) {_userID = str;}
};

#endif // FITTSLAWTESTPLUGIN_H
