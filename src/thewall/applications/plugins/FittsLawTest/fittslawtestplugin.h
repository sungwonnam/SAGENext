#ifndef FITTSLAWTESTPLUGIN_H
#define FITTSLAWTESTPLUGIN_H

#include "applications/base/basewidget.h"
#include "applications/base/SN_plugininterface.h"
#include "fittslawteststreamrecvthread.h"

#include <QtGui>

class FittsLawTestData;









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

    static int RoundID;

    /*!
      How many round per user
      */
    static int _NUM_ROUND;

    /*!
      How may targets will appear in a round
      */
    static int _NUM_TARGET_PER_ROUND;


    static const QString _streamerIpAddr;

    /*!
      This is streaming overhead.
      So, the buffer size is width * height * 3
      */
    static const QSize _streamImageSize;


    inline QString userID() const {return _userID;}

    SN_BaseWidget * createInstance();


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


    void updateInfoTextItem();


    int setQuality(qreal newQuality);


    void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

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
//    void timerEvent(QTimerEvent *);

private:
    static FittsLawTestData *_dataObject;

    void _init();

    /*!
      Whether the test is running.
      Uesrs is moving pointer and clicking targets.

      Recv() thread will run.
      */
    bool _isRunning;

    bool _isDryRun;

    /*!
      User can now click the green icon to start the test
      */
    bool _isReady;

    /*!
      How many targets a user successfully clicked so far
      */
    int _targetHitCount;

    /*!
      How many rounds completed so far
      */
    int _roundCount;

    int _roudnId;


    /*!
      A Dummy widget where targets will appear
      */
    QGraphicsWidget *_contentWidget;

    QGraphicsSimpleTextItem *_infoText;

    /*!
      green icon or stop sign
      */
    QGraphicsPixmapItem *_startstop;

    static const QPixmap _startPixmap;
    static const QPixmap _stopPixmap;

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

    /*!
      Labels
      */
    QLabel *_lbl_userid;
    QLabel *_lbl_roundid;
    QLabel *_lbl_tgtcount;
    QLabel *_lbl_hitlatency;
    QGraphicsProxyWidget *_lblproxy_userid;
    QGraphicsProxyWidget *_lblproxy_roundid;
    QGraphicsProxyWidget *_lblproxy_tgtcount;
    QGraphicsProxyWidget *_lblproxy_hitlatency;


    /*!
      A pixmap of cursor during the test
      */
    static const QPixmap _cursorPixmap;

    QGraphicsPixmapItem *_cursor;


    /*!
      The shared pointer (a user) interacting with this widget
      */
    SN_PolygonArrowPointer *_myPointer;

    /*!
      Thread receiving image from external program
      */
    FittsLawTestStreamReceiverThread *_recvThread;


    /*!
      The ssh process of streamer program
      */
    QProcess *_extProgram;


    /*!
      string representation of user id
      */
    QString _userID;


    /*!
      QPointF of 10 targets of a single round
      */
    QList<QPointF> _targetPosList;

    qreal _tgtDistance;

    /*!
      Decide next target's position randomly
      */
    QPointF m_getRandomPos();

    /*!
      The miss count per target
      */
    int _missCountPerTarget;
    int _missCountPerRound;
    int _missCountTotal;

    qint64 _targetAppearTime;
    qint64 _targetHitTime;

    Data _data;



    /*!
      Semaphore to trigger recv() of the recvThread whenever user interact
      */
    QSemaphore _sema;

signals:
    /*!
      The user clicked the green circle.
      resume stream receiving and start showing targets
      */
    void initClicked();

    /*!
      when a single round finishes
      */
//    void roundFinished(QChar uid, int roundid);

    /*!
      user clicked the dot successfully
      */
//    void hit();

    /*!
      user clicked but missed the dot
      */
//    void miss();


public slots:
    /*!
      Sets the size of the streaming overhead.

      @returns -1 on error
      */
    /*
    bool setOverheadSize(int w, int h);

    bool setOverheadSize(const QSize &size);

    bool setStreamerIP(const QString &str);
*/

    inline void setUserID(const QString &str) {_userID = str;}


    /*!
      Upon clicking (with system's mouse not the shared pointer) the ready button.
      If the external program (streamer) isn't running then it's going to be fired in here.
      */
    void setReady(bool isDryrun = false);

    /*!
      invoked upon receiving initClicked() signal

      A round consists of 10 targets.
      The streaming thread will resume.
      */
    void startRound();

    /*!
       This is called (in handlePointerClick) after user successfully clicked 10 targets
      The streaming thread will stop.
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

    /*!
      clear all saved target position data and populate it with null values
      */
    void clearTargetPosList();

    /*!
      clear all measurement data and populate it with null values
      */
    void clearData();

    /*!
      A test consists of 4 rounds.
      */
//    void finishTest();

    /*!
      launches external program using ssh command.
      The external program will wait for connection from StreamRecvThread
      */
    void launchExternalStreamer(const QString &ipaddr);

    /*!
      This slot will just call update()
      */
    void scheduleUpdate();
};
















class FittsLawTestData : public QObject
{
    Q_OBJECT

public:
    FittsLawTestData(QObject *parent=0);
    ~FittsLawTestData();

    static int _NUM_SUBJECTS;

    /*!
      This is called by user widget (FittsLawTest)
      or in recreateAllDataFiles()
      */
    void addWidget(FittsLawTest *widget, const QChar id = QChar());

    inline QFrame * getFrame() {return _frame;}

    void writeData(const QString &id, const QString &actionType, int roundid, int targetcount = -1, const QString &data = QString());

//    void writeData(const QString &id, const QString &actionType, int targetcount = -1, const QByteArray &bytearry = QByteArray());

    void flushCloseAll();

private:
    QString _filenameBase;

    QFile *_globalDataFile;

    QTextStream *_globalOut;


    QMap<QChar, QFile *> _perAppDataFiles;

    QMap<QChar, QTextStream *> _perAppOuts;

    QMap<QChar, FittsLawTest *> _widgetMap;

    QChar _currentID;

    QFrame *_frame;

    /*!
      closes and deletes all QIODevices and QTextStream.
      This will be called after completing a test.
      */
    void _deleteFileAndStreamObjects();

    bool _openGlobalDataFile();

    bool _openPerAppDataFile(const QChar id);

    /*!
      Dry run for users to learn
      */
    bool _isDryRun;

    QReadWriteLock _rwlock;

    /*!
      A test consists of pow(2, _NUM_SUBJECTS) rounds.
      ANd there will be two tests.
      One w/ scheduler
      The other w/o scheduler
      */
//    int _testCount;

    void m_createGUI();

signals:

private slots:
    void closeAll();

    void closeApp(QObject *obj);

    /*!
      After completing a test (w or w/o sched), run this function to re-create data files
      */
    void recreateAllDataFiles();

    void advanceRound();

};

#endif // FITTSLAWTESTPLUGIN_H
