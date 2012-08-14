#include "sn_fittslawtest.h"
#include "system/resourcemonitor.h"
#include "common/sn_sharedpointer.h"
#include "applications/base/sn_priority.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"
#include "common/imagedoublebuffer.h"

int SN_FittsLawTestData::_NUM_SUBJECTS = 3;
int SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND = 20;

// 131.193.78.176 (bigdaddy 100 Mbps)
// 131.193.78.142 (venom 100 Mbps)
// 67.58.62.57 (bigdaddy 10 Gbps)
// 67.58.62.45 (venom 10 Gbps)
//const QString FittsLawTest::_streamerIpAddr = QString("131.193.78.142");
//QString SN_FittsLawTest::_streamerIpAddr = QString("67.58.62.45");
//QSize SN_SageFittsLawTest::_streamImageSize = QSize(1920, 1080);


SN_FittsLawTestData * SN_SageFittsLawTest::_dataObject = 0;

/*!
  This will be updated by SN_SageFittsLawTestData::advanceRound()
  */
int SN_SageFittsLawTest::RoundID = -1;

/*!
  This will be calculated in _init() after reading the FittsLawTest.conf file
  */
int SN_SageFittsLawTest::_NUM_ROUND_PER_USER = -1;


SN_SageFittsLawTest::SN_SageFittsLawTest(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_SageStreamWidget(globalappid, s, rm, parent, wFlags)
    , _isRunning(false)
    , _isDryRun(false)
    , _isReady(false)
    , _targetHitCount(0)
    , _roundCount(0)
    , _contentWidget(0)
    , _infoText(0)
    , _startstop(0)
    , _target(0)

    , _resetRoundButton(0)
    , _resetTargetPosList(0)
    , _readybutton(0)
    , _donebutton(0)

    , _cursor(0)

    , _myPointer(0)

    , _missCountPerTarget(0)
    , _missCountPerRound(0)
    , _missCountTotal(0)

    , _sum_norm_latency(0)
    , _sum_latency(0)
    , _sum_Rq(0)
    , _sum_Rc(0)

    , _targetAppearTime(0.0)
    , _targetHitTime(0.0)

    , _screenUpdateFlag(0)
	, _numFramesForScreenUpdate(3)
    , _numFramesForScreenUpdateConfigured(3)

    , _isMissClickPenalty(false)
{
    //
    // sagepixelreceiver will wait for the sema
    //
    __sema = new QSemaphore;

    _useOpenGL = false;
    _useShader = false;
    _usePbo = false;


    //
    // set Schedulable widget
    // SN_Launcher will be able to add this widget to rMonitor's widget list
    // upon launching this widget.
    //
//    setSchedulable(); // This is done in SN_RailawareWidget

    setContentsMargins(16, 40, 16, 16);

    _startPixmap = QPixmap(":/resources/greenplay128.png");
    _stopPixmap = QPixmap(":/resources/stopsign48.png").scaledToWidth(128);
//    _cursorPixmap = QPixmap(":/resources/cursor_arrow_48x48.png");
    _cursorPixmap = QPixmap(":/resources/blackarrow_upleft128.png");

    _init();

//    qDebug() << "SN_SageFittsLawTest() : # rounds per user is" << SN_SageFittsLawTest::_NUM_ROUND_PER_USER;
}

void SN_SageFittsLawTest::_init() {

    int winwidth = 1920;
    int winheight = 1080;
    QFile f(QDir::homePath() + "/.sagenext/FittsLawTest.conf");
    if (f.exists()) {
        if (!f.open(QIODevice::ReadOnly))  {
            qDebug() << "SN_FittsLawTest::_init() : failed to open a config file";
        }
        else {
            forever {
                QByteArray line = f.readLine();
                if (line.isEmpty()) break;
                if (line.at(0) == '#') continue;

                int num_subject, num_targets_per_round;
    //            QByteArray streamerip(64, '\0');
                int num_frame_for_update = 0;

                sscanf(line.data(), "%d %d %d %d %d", &num_subject, &num_targets_per_round, &winwidth, &winheight, &num_frame_for_update);

                SN_FittsLawTestData::_NUM_SUBJECTS = num_subject;
                SN_SageFittsLawTest::_NUM_ROUND_PER_USER = pow(2, num_subject - 1); // not including the dry round
                SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND = num_targets_per_round;

    //            SN_FittsLawTest::_streamerIpAddr = QString(streamerip);
    //            SN_SageFittsLawTest::_streamImageSize = QSize(overheadwidth, overheadheight);

                //
                // read the configured value for NoSched
                //
                if (num_frame_for_update > 0)
                    _numFramesForScreenUpdateConfigured = num_frame_for_update;
            }

            f.close();
//            qDebug() << SN_FittsLawTestData::_NUM_SUBJECTS << SN_SageFittsLawTest::_NUM_ROUND_PER_USER << SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND << _numFramesForScreenUpdate;
        }
    }
    else {
        qDebug() << "SN_FittsLawTest::createInstance() : config file doesn't exist. Using default values";
    }

    clearTargetPosList();

    QObject::connect(_rMonitor, SIGNAL(schedulerStateChanged(bool)), this, SLOT(respondToSchedulerState(bool)));

    qDebug("%s::%s() : %d Subjects, %d Tgts/Rnd, %d Rnds/User, Window %dx%d, %d frame per update (NoSched)\n"
           , metaObject()->className()
           , __FUNCTION__
           , SN_FittsLawTestData::_NUM_SUBJECTS
           , SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND
           , SN_SageFittsLawTest::_NUM_ROUND_PER_USER
//           , SN_SageFittsLawTest::_streamImageSize.width()
//           , SN_SageFittsLawTest::_streamImageSize.height()
           , winwidth, winheight
           , _numFramesForScreenUpdate
           );


    if (!_dataObject) {
        SN_SageFittsLawTest::RoundID = -1;
        _dataObject = new SN_FittsLawTestData;
    }
    _dataObject->addWidget(this);




    //
    // content area wher targets will appear
    //
    _contentWidget = new QGraphicsWidget;
    _contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::DefaultType);
    _contentWidget->setAcceptedMouseButtons(0);

/**
    QPushButton *resetRndBtn = new QPushButton("Reset roundCount");
    QPushButton *resetList = new QPushButton("Reset targetList");
    QPushButton *ready = new QPushButton("set READY");
    QPushButton *done = new QPushButton("Write Data");

    QObject::connect(ready, SIGNAL(clicked()), this, SLOT(setReady()));
    QObject::connect(resetRndBtn, SIGNAL(clicked()), this, SLOT(resetTest()));
    QObject::connect(resetList, SIGNAL(clicked()), this, SLOT(clearTargetPosList()));
    QObject::connect(done, SIGNAL(clicked()), this, SLOT(finishTest()));

    _resetRoundButton = new QGraphicsProxyWidget(this);
    _resetTargetPosList = new QGraphicsProxyWidget(this);
    _readybutton = new QGraphicsProxyWidget(this);
    _donebutton = new QGraphicsProxyWidget(this);

    _resetTargetPosList->setWidget(resetList);
    _resetRoundButton->setWidget(resetRndBtn);
    _readybutton->setWidget(ready);
    _donebutton->setWidget(done);
**/

    _lbl_userid = new QLabel(_userID);
    _lbl_roundid = new QLabel("Round ID");
    _lbl_rndcount = new QLabel("Round Count");
    _lbl_tgtcount = new QLabel("Target Count");
    _lbl_distance = new QLabel("distance");

    _lblproxy_userid = new QGraphicsProxyWidget(this);
    _lblproxy_roundid = new QGraphicsProxyWidget(this);
    _lblproxy_rndcount = new QGraphicsProxyWidget(this);
    _lblproxy_tgtcount = new QGraphicsProxyWidget(this);
//    _lblproxy_distance = new QGraphicsProxyWidget(this);
    _lblproxy_userid->setWidget(_lbl_userid);
    _lblproxy_roundid->setWidget(_lbl_roundid);
    _lblproxy_rndcount->setWidget(_lbl_rndcount);
    _lblproxy_tgtcount->setWidget(_lbl_tgtcount);
//    _lblproxy_distance->setWidget(_lbl_distance);


    QGraphicsLinearLayout *toplayout = new QGraphicsLinearLayout(Qt::Horizontal);
    /*
    buttonlayout->addItem(_readybutton);
    buttonlayout->addItem(_resetRoundButton);
    buttonlayout->addItem(_resetTargetPosList);
    buttonlayout->addItem(_donebutton);
    */
    toplayout->addItem(_lblproxy_userid);
    toplayout->addItem(_lblproxy_roundid);
    toplayout->addItem(_lblproxy_rndcount);
    toplayout->addItem(_lblproxy_tgtcount);
//    toplayout->addItem(_lblproxy_hitlatency);
//    toplayout->addItem(_lblproxy_distance);


    //
    /// the green round start button
    //
    _startstop = new QGraphicsPixmapItem(_contentWidget);
    _startstop->setPixmap(_stopPixmap);
    _startstop->setAcceptedMouseButtons(0);

    //
    // yellow targets that the user will click
    //
    _target = new QGraphicsRectItem(0, 0, 64, 64, _contentWidget);
    _target->setFlag(QGraphicsItem::ItemStacksBehindParent);
    _target->setAcceptedMouseButtons(0);
    _target->setBrush(Qt::yellow);
    _target->hide();
//    _target->setOpacity(0.5);


    _cursor = new QGraphicsPixmapItem(_contentWidget);
    _cursor->setPixmap(_cursorPixmap);
    _cursor->setAcceptedMouseButtons(0);
    _cursor->hide();


    _infoText = new QGraphicsSimpleTextItem(_contentWidget);
    _infoText->setAcceptedMouseButtons(0);
    _infoText->setFlag(QGraphicsItem::ItemStacksBehindParent);
    QFont fnt;
    fnt.setPointSize(24);
    _infoText->setFont(fnt);
    _showInfo = false;


    //
    // buttons on the top
    // content area on the bottom
    //
    QGraphicsLinearLayout *mainlayout = new QGraphicsLinearLayout(Qt::Vertical);
    mainlayout->addItem(toplayout);
    mainlayout->addItem(_contentWidget);
    mainlayout->setStretchFactor(toplayout, 0);
    mainlayout->setStretchFactor(_contentWidget, 3);

    setLayout(mainlayout);

    resize(winwidth, winheight);

    //
    // signal slot connection for start/stop round
    //
    QObject::connect(this, SIGNAL(initClicked()), this, SLOT(startRound()));

    QObject::connect(this, SIGNAL(destroyed(QObject *)), _dataObject, SLOT(closeApp(QObject *)));
}

SN_SageFittsLawTest::~SN_SageFittsLawTest() {
    if (__sema) {
        __sema->release(10);
    }

    if (_receiverThread) {
        _receiverThread->setEnd();
    }

    if (_myPointer) {
        _myPointer->setTrapWidget(0);
        _myPointer->setOpacity(1.0);
        _myPointer = 0;
    }

    if (_dataObject) {
        delete _dataObject;
        _dataObject = 0;
    }

    if (__sema) delete __sema;
}


void SN_SageFittsLawTest::scheduleDummyUpdate() {
    if (doubleBuffer)
        doubleBuffer->releaseBackBuffer();

    //
    // multiple frames might be need to be received to update the screen
    //
    if (_screenUpdateFlag <= 0) {

        // reset
        _screenUpdateFlag = _numFramesForScreenUpdate - 1; 

		//
		// This shouldn't be in the paint() function !
		// Because if any widget calls update(), all widgets' paint() will be called.
		//
        _cursor->setPos( mapToItem(_contentWidget, _cursorPoint) );
        update();
    }
    else {
        _screenUpdateFlag--;
    }

}


int SN_SageFittsLawTest::setQuality(qreal newQuality) {

    if (!_perfMon) return -1;

    if (_quality == newQuality) {
        // No change, there's nothing to do
        return 0;
    }


    unsigned long thedelay = 0;


    //
    // The scheduler will set 0 quality
    // only if _perfMon->getRequiredBW_Mbps() <= 0
    //
    // Note that a widget w/o a priori starts with its requiredBW 0
    //
    if (newQuality <= 0) {
        //
        // ignore the quality the scheduler demands
        // because the widget set its required BW 0
        // meaning it's not consuming resource
        //
        _quality = 0;
        return -1;
    }
    else if ( newQuality >= 1.0 ) {

		_quality = 1.0;
        thedelay = 0;
	}

    //
    // 0 < newQuality < 1
    //
	else {
		_quality = newQuality;

        qreal bw_demanded = _perfMon->getRequiredBW_Mbps(_quality); // demanded by the scheduler

        qreal fps_demanded =  1e+6 * bw_demanded / (_appInfo->frameSizeInByte() * 8.0f); // demanded by the scheduler

        qreal delay_demanded = 1000.0f / fps_demanded; // in msec
        thedelay = delay_demanded;
	}


    if (_receiverThread) {
        if ( ! QMetaObject::invokeMethod(_receiverThread, "setDelay_msec", Qt::QueuedConnection, Q_ARG(qint64, thedelay)) ) {
            qDebug() << "SN_SageFittsLawTest::setQuality() : failed to invoke _receiverThread->setDelay_msec(qint64)";
            return -1;
        }
//        qDebug() << "SN_SageFittsLawTest::setQuality() : Dq" << _quality << "Delay" << thedelay;
        return 0;
    }
    else {
        qDebug() << "SN_SageFittsLawTest::setQuality() : _recvThread is null";
        return -1;
    }
}



void SN_SageFittsLawTest::resizeEvent(QGraphicsSceneResizeEvent *event) {
    Q_UNUSED(event);
//    qDebug() << boundingRect() << _contentWidget->size();
    _startstop->setPos( _contentWidget->size().width() / 2 - 64 , _contentWidget->size().height() / 2 - 64);
}

void SN_SageFittsLawTest::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    // do nothing
//    qDebug() << "DBL CLICK";
}

/*!
 * THis function will be called whenever ANY widget issues the update()
 */
void SN_SageFittsLawTest::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {

    if (_isRunning) {
        if (_isDryRun) {
            painter->drawText( _contentWidget->geometry(), Qt::AlignCenter, "PRACTICE RUN");
            _cursor->setPos( mapToItem(_contentWidget, _cursorPoint) );
        }
    }

    SN_BaseWidget::paint(painter, o, w);
}

bool SN_SageFittsLawTest::handlePointerClick(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
    Q_UNUSED(btn);

    if (!_isRunning) {
        //
        // User need to click the green triangle to start a round
        //
        if (_isReady) {
            //
            // See if the click occurred on the initating green icon
            // if yes then start the process
            //
            if ( _startstop->contains(_startstop->mapFromItem(this, point).toPoint())) {

                _prevTargetPosition = _startstop->pos(); // _contentWidget's coordinate

                _cursorPoint = point; // update the cursor position
                _cursor->show();

                _myPointer = pointer;

                //
                // trap the pointer so that it can only move within this widget
                //
                pointer->setTrapWidget(this);

                emit initClicked(); // connected to startRound()

                //
                // shared pointer is invisible during a round
                //
                pointer->setOpacity(0);

                //
                // the first target
                //
                determineNextTargetPosition(_roundCount, _targetHitCount);

                _target->show();
            }
        }

        //
        // prevent shared pointer from generating real mouse events
        //
        return true;
    }

    //
    // see if the click cocurred on the target (HIT)
    // then show next target
    //
    if ( _myPointer == pointer && _target->contains( _target->mapFromItem(this, point).toPoint() )) {

        //            qDebug() << "FittsLawTest::handlePointerClick() : hit";

        //
        // hit latency
        //
        _targetHitTime = QDateTime::currentMSecsSinceEpoch(); // msec

        //            (_data.misscount)[ _roundCount * FittsLawTest::_NUM_TARGET_PER_ROUND + _targetHitCount] = _missCountPerTarget;
        //            (_data.hitlatency)[_roundCount * FittsLawTest::_NUM_TARGET_PER_ROUND + _targetHitCount] = _targetHitTime - _targetAppearTime;

        qint64 hitlatency = _targetHitTime - _targetAppearTime;
        _sum_latency += hitlatency;


        //
        // The distance from the previous target
        // if it's the first target then distance from the startstop button
        //
        qreal distance = qSqrt( qPow(_target->x() - _prevTargetPosition.x(), 2) + qPow(_target->y()-_prevTargetPosition.y() ,2) );
        _prevTargetPosition = _target->pos();
        //            _lbl_distance->setText(QString::number(distance));

        //            qDebug() << "ID" << _globalAppId << "R#" << _roundCount << "T#" << _targetHitCount << "miss" << _data.misscount.at(_roundCount *10 + _targetHitCount) << "delay" << _data.hitlatency.at(_roundCount *10 + _targetHitCount) << "msec";

        _sum_norm_latency += (hitlatency / distance);


        qreal Rq = _perfMon->getRequiredBW_Mbps();
        qreal Rc = _perfMon->getCurrBW_Mbps();
        _sum_Rq += Rq; // this is reset in finishRound()
        _sum_Rc += Rc; // this is reset in finishRound()
        _dataObject->writeData(_userID, "HIT", SN_SageFittsLawTest::RoundID, _targetHitCount, hitlatency, distance, _missCountPerTarget, -1, -1, -1, Rq, Rc);

        //
        // reset the miss count upon a successful hit
        //
        _missCountPerTarget = 0;

        _targetHitCount++; // increment for next target


        //
        // if current round is compeleted
        //
        if (_targetHitCount == SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND) {
            //                qDebug() << "FittsLawTest::handlePointerClick() : round #" << _roundCount << "finished";
            pointer->setOpacity(1);
            finishRound();
        }

        //
        // otherwise, show next target
        //
        else {
//            qDebug() << "HIT" << _roundCount << _targetHitCount - 1 << _target->pos();
            determineNextTargetPosition(_roundCount, _targetHitCount);
        }
    }

    //
    // otherwise it's MISS
    //
    else {
        setTargetCursorPixmap(":/resources/redarrow_missed_upleft128.png");
        QTimer::singleShot(500, this, SLOT(setTargetCursorPixmap()));

        _missCountPerTarget++;

        _dataObject->writeData(_userID, "MISS", SN_SageFittsLawTest::RoundID, _targetHitCount);

        _missCountPerRound += _missCountPerTarget;
        _missCountTotal += _missCountPerTarget;

        if (_isMissClickPenalty) {

            //
            // save current missed target's position
            //
            _prevTargetPosition = _target->pos();

            // show another target
            // target count shouldn't be increased
            determineNextTargetPosition(_roundCount, _targetHitCount, true);
        }
    }


    //
    // prevent shared pointer from generating real mouse events
    //
    return true;
}



void SN_SageFittsLawTest::handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal dx, qreal dy, Qt::MouseButton btn, Qt::KeyboardModifier mod) {
    if (_isRunning) {
        if (pointer == _myPointer) {

			if (_priorityData) {
				qint64 currTS = QDateTime::currentMSecsSinceEpoch();
				_priorityData->setLastInteraction(SN_Priority::CONTENTS, currTS);
			}


            //
            // variable frame rate based on mouse interaction
            //
            if (_receiverThread && _receiverThread->isRunning() && !_isDryRun) {
                //
                // create one resource for the stream receiver
                // upon receiving a frame, the receiver thread will emit frameReceived()
                // which is connected up scheduleUpdate()
                //
				if (__sema && __sema->available() < 1)
                	__sema->release(1);
            }
            else {
                update();
            }


            /*
            if (_recvObject && _thread.isRunning()) {
                if ( ! QMetaObject::invokeMethod(_recvObject, "recvFrame", Qt::QueuedConnection) ) {
                    qDebug() << "FittsLawTest::handlePointerDrag() : Failed to invoke recvFrame()";
                }
            }
            */
            _cursorPoint = point;
        }
    }

    else {
        SN_BaseWidget::handlePointerDrag(pointer, point, dx, dy, btn, mod);
    }
}

void SN_SageFittsLawTest::handlePointerPress(SN_PolygonArrowPointer *, const QPointF &point, Qt::MouseButton btn) {

    setTopmost();

    // can't resize all times
    _isResizing = false;

    _isMoving = false;

    if (_isRunning) {
        // can't even moving while test is running
    }
    else {
        if (_contentWidget->contains( mapToItem(_contentWidget, point) )) {
            // do nothing
        }
        else {
            if (btn == Qt::LeftButton)
                _isMoving = true;
        }
    }
}

void SN_SageFittsLawTest::setPerfMonRwParameters(int wakeUpGuessFps, qreal overPerformMult, qreal normPerformMult, int underPerformEndur) {
    if (!_perfMon) {
        qDebug() << "SN_SageFittsLawTest::setPerfMonRwParameters() : _perfMon is null";
        return;
    }

    _perfMon->setWakeUpGuessFps(wakeUpGuessFps);
    _perfMon->setOverPerformMult(overPerformMult);
    _perfMon->setNormPerformMult(normPerformMult);
    _perfMon->setUnderPerformEndur(underPerformEndur);

    qDebug() << "UserID" << _userID << "Rw params" << _perfMon->wakeUpGuessFps() << _perfMon->overPerformMult() << _perfMon->normPerformMult() << _perfMon->underPerformEndur();
}

/*!
  This is invoked by the test controller (not a user).
  User will see the green play icon
  */
void SN_SageFittsLawTest::setReady(bool isDryrun /* false */) {
//    qDebug() << "setReady() : " << SN_SageFittsLawTest::_NUM_ROUND_PER_USER << SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND;
    Q_ASSERT(_targetPosList.size() == SN_SageFittsLawTest::_NUM_ROUND_PER_USER * SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND);

    if (isDryrun)
        _lbl_roundid->setText("Practice Round");
    else {
        _lbl_roundid->setText("Round "+QString::number(SN_SageFittsLawTest::RoundID));
        _lbl_rndcount->setText("Rnd Count " + QString::number(1+_roundCount)+"/"+QString::number(SN_SageFittsLawTest::_NUM_ROUND_PER_USER));
    }


    _isDryRun = isDryrun;

    if (_isRunning) {
        finishRound();
    }

    _isReady = true;
    _targetHitCount = 0;

    if (_roundCount >= SN_SageFittsLawTest::_NUM_ROUND_PER_USER) {
        _roundCount = 0;
    }

    _startstop->setPixmap(_startPixmap);
    _startstop->show();

    qDebug() << "UserID" << _userID << "Rw params" << _perfMon->wakeUpGuessFps() << _perfMon->overPerformMult() << _perfMon->normPerformMult() << _perfMon->underPerformEndur();
}

/*!
  A user clicked the green play icon.
  (Upon receiving initClicked() signal)
  */
void SN_SageFittsLawTest::startRound() {

    _targetHitCount = 0;
    _isRunning = true;
    _startstop->hide();
    _target->show();

    if (_isDryRun) {
        _dataObject->writeData(_userID, "DRY_RND", SN_SageFittsLawTest::RoundID);
    }
    else {
        _dataObject->writeData(_userID, "START_RND", SN_SageFittsLawTest::RoundID);

        //
        // streaming resumes
        // will call QThread::start()
		//
		///_recvThread->resumeThreadLoop();
		QMetaObject::invokeMethod(_receiverThread, "resumeThreadLoop", Qt::AutoConnection);
		QApplication::sendPostedEvents();
		QCoreApplication::processEvents();




        //
        // let the rMonitor knows the user is interacting. This is just for rMonitor::printData()
        // The system can know this just by monitoring currBW
        //
        _perfMon->setInteracting(true);
    }


    /***
      * if you uncomment this, then you only need to uncomment the block in handlePointerDragg()
    if (!_recvObject) {
        _recvObject = new FittsLawTestStreamReceiver(_streamImageSize, _streamerIpAddr, _globalAppId + 60000, _perfMon);

        QObject::connect(_recvObject, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()), Qt::QueuedConnection);

//        QObject::connect(&_thread, SIGNAL(started()), _recvObject, SLOT(connectToStreamer()), Qt::QueuedConnection);

        _recvObject->moveToThread(&_thread);

        if (!_thread.isRunning()) {
            _thread.start();
        }

        if ( ! QMetaObject::invokeMethod(_recvObject, "connectToStreamer", Qt::QueuedConnection) ) {
            qDebug() << "FittsLawTest::startRound() : Failed to invoke connectToStreamer()";
        }
    }
    ***/
}

//
// This is called after user successfully clicked 10 targets
//
void SN_SageFittsLawTest::finishRound() {
    /*
    if (_perfMon) {
        //
        // user interaction stops until next round starts
        //
        _perfMon->setRequiredBW_Mbps(0);

        //
        //  This is hacking..
        // There's no way to keep track of current resource usage (currFrameRate)
        // when the receiving thread is not running
        //
        //
        _perfMon->overrideCurrBW_Mbps(0);


        //
        // However, if I keep track of cumulative byte received with a global timer
        // I can know above two even the thread is not running
        //
    }
*/

    _perfMon->setInteracting(false);

    _cursor->hide();

	_screenUpdateFlag = 0;

    _lbl_rndcount->setText("Round Count");
    _lbl_tgtcount->setText("Target Count");


    if (_receiverThread) {
        //_recvThread->endThreadLoop();
//        QMetaObject::invokeMethod(_receiverThread, "setEnd", Qt::AutoConnection);
//        QApplication::sendPostedEvents();
//        QCoreApplication::processEvents();
        _receiverThread->setEnd();
	}

    _isRunning = false;
    _isReady = false;
    _targetHitCount = 0;


    if (_myPointer) {
        _myPointer->setOpacity(1);
        _myPointer->setTrapWidget(0); // reset the trapped pointer
        _myPointer = 0;
    }

    _target->hide();
    _startstop->setPixmap(_stopPixmap);
    _startstop->show();



    _dataObject->writeData(_userID, "FINISH_RND", SN_SageFittsLawTest::RoundID, -1, -1, -1, -1
                           , _sum_latency/SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND
                           , _sum_norm_latency/SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND
                           , _missCountPerRound

                           , _sum_Rq / SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND
                           , _sum_Rc / SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND
                           );

    if (!_isDryRun) {
        _roundCount++;

        if (_roundCount >= SN_SageFittsLawTest::_NUM_ROUND_PER_USER) {
            qDebug() << "FittsLawTest::finishRound() : A test has completed for user" << _userID;

            //
            // reset round count
            //
            _roundCount = 0;

            //
            // close files
            //
            _dataObject->flushAll(_userID);
        }
        else {
            qDebug() << "FittsLawTest::finishRound() : A round" << _roundCount - 1 << "has completed for user" << _userID;
        }
    }

    //
    // dry run doesn't increment local round count
    //
    else {
        qDebug() << "FittsLawTest::finishRound() : A DRY round (round 0) has finished for user" << _userID;
    }

    _missCountPerTarget = 0;
    _missCountPerRound = 0;
    _sum_latency = 0;
    _sum_norm_latency = 0;
    _sum_Rq = 0;
    _sum_Rc = 0;
}

void SN_SageFittsLawTest::determineNextTargetPosition(int local_round_count, int next_target_id, bool force_random_determine /* false */) {

    int idx = local_round_count * SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND + next_target_id;

    QPointF nextPos;
    if (!force_random_determine) {
        nextPos = _targetPosList.value(idx);
    }

    //
    // there's saved value
    //
    if (! nextPos.isNull() ) {
//        qDebug() << "determineNextTargetPosition()" << _roundCount << _targetHitCount << "has point data" << nextPos;
//        qDebug() << _userID << "Round" << SN_SageFittsLawTest::RoundID << "(" << _roundCount << "," << _targetHitCount << ") has saved pos data";
    }

    //
    // get value and save it
    //
    else {
        nextPos = m_getRandomPos();
//        qDebug() << "determineNextTargetPosition() : determined target pos for" << local_round_count << next_target_id;

        //
        // Assumes the list is already filled with null data (call clearTargetPosList() !!)
        //
        if (!_isDryRun)
            _targetPosList[ idx ] = nextPos;

//        qDebug() << "determineNextTargetPosition()" << _roundCount << _targetHitCount << "choosing random pos" << nextPos;
    }

    Q_ASSERT(_target);

    qreal dx = nextPos.x() - _target->pos().x();
    qreal dy = nextPos.y() - _target->pos().y();
    _tgtDistance = sqrt(dx*dx + dy*dy);

    _target->setPos(nextPos);

    _lbl_tgtcount->setText("Tgt Cnt " + QString::number(1+_targetHitCount)+"/"+QString::number(SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND));

    _targetAppearTime = QDateTime::currentMSecsSinceEpoch(); // msec
}


void SN_SageFittsLawTest::resetTest() {
    finishRound();
    _roundCount = 0;
}

void SN_SageFittsLawTest::clearTargetPosList() {
    _roundCount = 0;
    _targetPosList.clear();

    for (int i=0; i< SN_SageFittsLawTest::_NUM_ROUND_PER_USER * SN_SageFittsLawTest::_NUM_TARGET_PER_ROUND; i++) {
        _targetPosList.append( QPointF() ); // init with null point
    }

    qDebug() << _userID << "cleared Tgt position data";
}


QPointF SN_SageFittsLawTest::m_getRandomPos() {
    Q_ASSERT(_contentWidget);
    Q_ASSERT(_target);

    int possible_x = _contentWidget->size().width() - _target->boundingRect().width();
    int possible_y = _contentWidget->size().height() - _target->boundingRect().height();

    int randi = qrand();
    qreal rand_x = (qreal)possible_x * ( (qreal)randi / (qreal)RAND_MAX ); // 0 ~ width

    randi = qrand();
    qreal rand_y = (qreal)possible_y * ( (qreal)randi / (qreal)RAND_MAX ); // 0 ~ height

    return QPointF(rand_x, rand_y);
}

void SN_SageFittsLawTest::updateInfoTextItem() {
    if (! _infoText  ||  ! _showInfo) return;

    QByteArray text(2048, '\0');

    sprintf(text.data(),
            "Round %d, Target %d \n P %.2f (Win %hu, Wal %hu, ipm %.3f) \n OQ_Rq %.1f OQ_Dq %.1f DQ %.1f \n CurBW %.3f, ReqBW %.3f \n allowedBW %.3f"
            , _roundCount, _targetHitCount

            , priority() /* qreal */
            , (_priorityData) ? _priorityData->evrToWin() : 0 /* unsigned short - quint16 */
            , (_priorityData) ? _priorityData->evrToWall() : 0 /* unsigned short - quint16 */
            , (_priorityData) ? _priorityData->ipm() : 0 /* qreal */

            , observedQuality_Rq()
			, observedQuality_Dq()
			, _quality
            , (_perfMon) ? _perfMon->getCurrBW_Mbps() : -1
			, (_perfMon) ? _perfMon->getRequiredBW_Mbps() : -1
            , (_perfMon) ? _perfMon->getRequiredBW_Mbps(_quality) : -1
            );

    _infoText->setText(QString(text));
}

void SN_SageFittsLawTest::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setBrush(Qt::lightGray);
    painter->drawRect(windowFrameRect());
}

void SN_SageFittsLawTest::startReceivingThread() {
    Q_ASSERT(streamsocket > 0);

    //
    // initialize OpenGL
    //
    /*
	if (_useOpenGL) {
        m_initOpenGL();
	}
    */

    //
    // create the recv thread
    //
	_receiverThread = new SN_SagePixelReceiver(_streamProtocol, streamsocket, doubleBuffer, _usePbo, _pbobufarray, _pbomutex, _pbobufferready, this, _settings);
    Q_ASSERT(_receiverThread);


    //
    // signal slot connections with the thread
    //

//    QObject::connect(_receiverThread, SIGNAL(finished()), this, SLOT(close())); // WA_Delete_on_close is defined
    /***
    if (!_blameXinerama) {
        if (_usePbo) {
            if ( ! QObject::connect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(schedulePboUpdate())) ) {
                qCritical("%s::%s() : Failed to connect frameReceived() signal and schedulePboUpdate() slot", metaObject()->className(), __FUNCTION__);
                return;
            }
        }
        else {
            if ( ! QObject::connect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate())) ) {
                qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleUpdate() slot", metaObject()->className(), __FUNCTION__);
                return;
            }
        }
    }
    else {
        // I think Xinerama makes graphics performance bad..
        // On venom, five 1080p videos can't sustain 24 fps.. So reboot the machine
//        qDebug() << "SN_SageFittsLawTest::startReceivingThread() : BLAME_XINERAMA defined. no frameReceived/scheduleUpdate connection";
        QObject::connect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleDummyUpdate()));
    }
    ***/

    if ( ! QObject::connect(_receiverThread, SIGNAL(frameReceived()), this, SLOT(scheduleDummyUpdate())) ) {
        qCritical("%s::%s() : Failed to connect frameReceived() signal and scheduleDummyUpdate() slot", metaObject()->className(), __FUNCTION__);
        return;
    }


    qDebug() << "SN_SageFittsLawTest (" << _globalAppId << _sageAppId << ") ready for streaming.";
	qDebug() << "\t" << "app name" << _appInfo->executableName() << ",media file" << _appInfo->fileInfo().fileName() << "from" << _appInfo->srcAddr();
	qDebug() << "\t" << _appInfo->nativeSize().width() <<"x" << _appInfo->nativeSize().height() << _appInfo->bitPerPixel() << "bpp" << _appInfo->frameSizeInByte() << "Byte/frame at" << _perfMon->getExpetctedFps() << "fps";
	qDebug() << "\t" << "network user buffer length (groupsize)" << _appInfo->networkUserBufferLength() << "Byte";
	qDebug() << "\t" << "GL pixel format" << _pixelFormat << ",use SHADER (for YUV format)" << _useShader << ",use OpenGL PBO" << _usePbo;


    //
    // thread will start in startRound()
    //
//    _receiverThread->start();
}

void SN_SageFittsLawTest::respondToSchedulerState(bool started) {
    if (started) {
        if (_numFramesForScreenUpdateConfigured > 0) {
            _numFramesForScreenUpdate = _numFramesForScreenUpdateConfigured - 1;
            qDebug() << "The Scheduler has started !!" << _numFramesForScreenUpdate;
        }
    }
    else {
        _numFramesForScreenUpdate = _numFramesForScreenUpdateConfigured;
        qDebug() << "The Scheduler has stopped !!" << _numFramesForScreenUpdate;
    }
}





























SN_FittsLawTestData::SN_FittsLawTestData(QObject *parent)
    : QObject(parent)
    , _globalDataFile(0)
    , _globalOut(0)
    , _currentID(QChar())
    , _frame(0)
    , _isDryRun(true)
{

    SN_SageFittsLawTest::RoundID = -1;
    _filenameBase = QDir::homePath().append("/.sagenext/FLTdata_");
    qDebug() << "SN_FittsLawTestData::SN_FittsLawTestData() : # subjects" << SN_FittsLawTestData::_NUM_SUBJECTS;
}

SN_FittsLawTestData::~SN_FittsLawTestData() {
    if (_frame) {
        delete _frame;
    }
    _deleteFileAndStreamObjects();

    _widgetMap.clear();
    _perAppDataFiles.clear();
    _perAppOuts.clear();

    qDebug() << "SN_FittsLawTestData::~SN_FittsLawTestData()";
}

/*!
  This function is called after
  all the test widgets are added
  */
void SN_FittsLawTestData::m_createGUI() {

    if (!_frame) {
        _frame = new QFrame;

        _frame->setWindowTitle("FittsLawTest Controller");

        QPushButton *nextRnd = new QPushButton("Next Round");
        QObject::connect(nextRnd, SIGNAL(clicked()), this, SLOT(advanceRound()));

        QPushButton *pracRnd = new QPushButton("Practice Round");
        QObject::connect(pracRnd, SIGNAL(clicked()), this, SLOT(practiceRound()));

//        QPushButton *finalRnd = new QPushButton("_Final Round_");
//        QObject::connect(finalRnd, SIGNAL(clicked()), this, SLOT(finalRound()));

//        QPushButton *clearTgtPos = new QPushButton("Clear Saved Tgt Pos");
//        QObject::connect(clearTgtPos, SIGNAL(clicked()), this, SLOT(clearAllSavedTgtPos()));

        QPushButton *rwparam = new QPushButton("set Rw conservatively");
        QObject::connect(rwparam, SIGNAL(clicked()), this, SLOT(setRwParamConservatively()));

        QPushButton *close = new QPushButton("Finish");
        QObject::connect(close, SIGNAL(clicked()), this, SLOT(closeAll()));

        QCheckBox *isMissClickPenalty = new QCheckBox("MissClick Penalty");
        isMissClickPenalty->setChecked(false);
        QObject::connect(isMissClickPenalty, SIGNAL(toggled(bool)), this, SLOT(toggleMissClickPenalty(bool)));

        QVBoxLayout *hl = new QVBoxLayout;
        hl->addWidget(nextRnd);
        hl->addWidget(pracRnd);
        hl->addWidget(rwparam);
//        hl->addWidget(finalRnd);
//        hl->addWidget(clearTgtPos);
        hl->addWidget(close);
        hl->addWidget(isMissClickPenalty);
        _frame->setLayout(hl);
    }

    _frame->show();
    _frame->move(QPoint(10,10));
}

void SN_FittsLawTestData::setRwParamConservatively() {
    QMap<QChar, SN_SageFittsLawTest *>::iterator it;
    for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
        SN_SageFittsLawTest *widget = it.value();
        if (widget) {
            // wakeUpGuessFps
            // overPerformMultiplier
            // normPerformMultiplier
            // underPerformEndurance
            widget->setPerfMonRwParameters(20, 1.05, 1.1, 2);
        }
    }
}

void SN_FittsLawTestData::toggleMissClickPenalty(bool b) {
    QMap<QChar, SN_SageFittsLawTest *>::iterator it;
    for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
        SN_SageFittsLawTest *widget = it.value();
        if (widget) {

            //
            // if b is true then
            // No missclick penalty (target will stay even if user miss clicked)
            //
            widget->setMissClickPenalty(b);
        }
    }
}

void SN_FittsLawTestData::clearAllSavedTgtPos() {
    QMap<QChar, SN_SageFittsLawTest *>::iterator it;
    for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
        SN_SageFittsLawTest *widget = it.value();
        if (widget) {
            widget->clearTargetPosList();
        }
    }
}

void SN_FittsLawTestData::closeAll() {
    QMap<QChar, SN_SageFittsLawTest *>::iterator it;
    for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
        SN_SageFittsLawTest *widget = it.value();
        if (widget) {
            widget->close();
            widget->deleteLater();
        }
    }
    SN_SageFittsLawTest::RoundID = -1;
    deleteLater();
}

void SN_FittsLawTestData::closeApp(QObject *obj) {
    SN_SageFittsLawTest *widget = dynamic_cast<SN_SageFittsLawTest *>(obj);
    if (widget) {
        QChar uid = _widgetMap.key(widget, QChar());
        if (!uid.isNull()) {
            _widgetMap.remove(uid);

            QTextStream *ts = _perAppOuts.value(uid, 0);
            if (ts) {
                ts->flush();
                delete ts;
            }
            _perAppOuts.remove(uid);

            QFile *f = _perAppDataFiles.value(uid, 0);
            if (f) {
                f->close();
                delete f;
            }
            _perAppDataFiles.remove(uid);
        }
    }
}

void SN_FittsLawTestData::advanceRound() {

    Q_ASSERT(_widgetMap.size() == SN_FittsLawTestData::_NUM_SUBJECTS);

    if (_globalOut) _globalOut->flush();

    QTextStream *ts = 0;
    ts = _perAppOuts.value('A', 0);
    if (ts) ts->flush();
    ts = _perAppOuts.value('B', 0);
    if (ts) ts->flush();
    ts = _perAppOuts.value('C', 0);
    if (ts) ts->flush();

    //
    // increment the RoundID, it's initialized with -1
    //
    SN_SageFittsLawTest::RoundID += 1;

    if ( SN_SageFittsLawTest::RoundID > (pow(2, SN_FittsLawTestData::_NUM_SUBJECTS) - 1) ) {
        qDebug() << "advanceRound() : reset RoundID";

        SN_SageFittsLawTest::RoundID = 1; // no more dry run is needed.
    }
    qDebug() << "\nSN_FittsLawTestData::advanceRound() : Ready for Round ID" << SN_SageFittsLawTest::RoundID;

    switch (SN_SageFittsLawTest::RoundID) {
    case 0: {
        if (_isDryRun) {

            //
            // setReady all
            //
            QMap<QChar, SN_SageFittsLawTest *>::iterator it;
            for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
                SN_SageFittsLawTest *widget = it.value();
                widget->setReady(true); // DRY run
            }

            //
            // now reset the dryrun flag
            //
            _isDryRun = false;

        }
        break;
    }

        //
        // Final round if _NUM_SUBJECTS == 1
        //
    case 1: {
        Q_ASSERT(_widgetMap.value('A', 0));
        (_widgetMap['A'])->setReady();


        break;
    }
    case 2: {
        Q_ASSERT(_widgetMap.value('B', 0));
        (_widgetMap['B'])->setReady();

        break;
    }

        //
        // Final round if _NUM_SUBJECTS == 2
        //
    case 3: {
        if (SN_FittsLawTestData::_NUM_SUBJECTS == 3) {
            (_widgetMap['C'])->setReady();
        }
        else if (SN_FittsLawTestData::_NUM_SUBJECTS == 2) {
            //
            // the final round
            //
            (_widgetMap['B'])->setReady();
            (_widgetMap['A'])->setReady();
        }

        break;
    }
    case 4: {
//        (_widgetMap['C'])->setReady();

        (_widgetMap['B'])->setReady();
        (_widgetMap['A'])->setReady();

        break;
    }
    case 5: {
        (_widgetMap['C'])->setReady();
        (_widgetMap['A'])->setReady();
        break;
    }
    case 6: {
        (_widgetMap['C'])->setReady();
        (_widgetMap['B'])->setReady();
        break;
    }

        //
        // Final round if _NUM_SUBJECTS == 3
        //
    case 7: {
        (_widgetMap['C'])->setReady();
        (_widgetMap['B'])->setReady();
        (_widgetMap['A'])->setReady();
        break;
    }
    }
}

void SN_FittsLawTestData::practiceRound() {
    if (_globalOut) _globalOut->flush();

    _isDryRun = true;

    //
    // Set it to the practice Round
    //
    SN_SageFittsLawTest::RoundID = -1;

    advanceRound();
}

void SN_FittsLawTestData::finalRound() {
    if (_globalOut) _globalOut->flush();

    _isDryRun = false;

    //
    // Set it to the final Round
    //
    SN_SageFittsLawTest::RoundID = (pow(2, SN_FittsLawTestData::_NUM_SUBJECTS) - 1);

    SN_SageFittsLawTest *widget = 0;
    widget = _widgetMap.value('A',0);
    if (widget) widget->setReady();

    widget = _widgetMap.value('B',0);
    if (widget) widget->setReady();

    widget = _widgetMap.value('C',0);
    if (widget) widget->setReady();
}

bool SN_FittsLawTestData::_openGlobalDataFile() {
    QString fname = _filenameBase;
    _globalDataFile = new QFile(fname.append("global.csv"));
    if ( ! _globalDataFile->open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        qDebug() << "FittsLawTestData::openFile() failed to open" << _globalDataFile->fileName();
        return false;
    }
    _globalOut = new QTextStream(_globalDataFile);

//    (*_globalOut) << "# users" << FittsLawTestData::_NUM_SUBJECTS << "StreamerIP" << FittsLawTest::_streamerIpAddr << "Overhead" << FittsLawTest::_streamImageSize;
//    (*_globalOut) << "# TimeStamp, ActionType, UserID, RoundID, TargetCount, HitLatency, NormalizedHitsLatency\n";

    return true;
}

bool SN_FittsLawTestData::_openPerAppDataFile(const QChar id) {
    QString fname = _filenameBase;

    QFile *f = new QFile(fname.append(QString(id)).append(".csv"));
    QTextStream *out = new QTextStream(f);

    if ( ! f->open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        qDebug() << "FittsLawTestData::openPerAppDataFile() : failed to open a file" << f->fileName();
        return false;
    }

    _perAppDataFiles.insert(id, f);
    _perAppOuts.insert(id, out);

    return true;
}

void SN_FittsLawTestData::addWidget(SN_SageFittsLawTest *widget, const QChar id) {
    Q_ASSERT(widget);

    if (id.isNull()) {
        if (_currentID.isNull()) {
            _currentID = 'A';
        }
        else if (_currentID == 'A') {
            _currentID = 'B';
        }
        else if (_currentID == 'B') {
            _currentID = 'C';
        }
    }
    else {
        _currentID = id;
    }

    widget->setUserID(QString(_currentID));
    _widgetMap.insert(_currentID, widget);


    if ( ! _openPerAppDataFile(_currentID) ) {
        qDebug() << "FittsLawTestData::addWidget() : failed to open a file for id" << _currentID;
    }
    else {
        qDebug() << "FittsLawTestData::addWidget() : a user data file created for id" << _currentID;
    }

    if (_widgetMap.size() == SN_FittsLawTestData::_NUM_SUBJECTS) {
        if ( ! _openGlobalDataFile() ) {
            qDebug() << "FittsLawTestData::addWidget() : failed to create the globalDataFile ! " << _globalDataFile->fileName();
        }
        else {
            qDebug() << "FittsLawTestData::addWidget() : the global data file created" << _globalDataFile->fileName();
        }

        m_createGUI();
    }
}


//void FittsLawTestData::writeData(const QString &id, const QString &actionType, int targetcount, const QByteArray &bytearry) {
//    writeData(id, actionType, targetcount, QString(bytearry));
//}

void SN_FittsLawTestData::writeData(const QString &id
                                    , const QString &actionType
                                    , int roundid
                                    , int targetcount/*-1*/
                                    , qint64 latency /* -1 */
                                    , qreal distance /* -1 */
                                    , int missfortarget/*-1*/
                                    , qreal avg_latency/*-1*/
                                    , qreal avg_norm_latency/*-1*/
                                    , int misscountround /*-1*/
                                    , qreal Rq /*-1*/
                                    , qreal Rc /*-1*/
                                    )
{

    qint64 ts = QDateTime::currentMSecsSinceEpoch();

    // START_RND
    // FINISH_RND
    // HIT , targetcount, latency
    // MISS, targetcount

    if (_globalOut) {

        _rwlock.lockForWrite();

//        (*_globalOut) << "TimeStamp, ActionType, UserID, RoundID [,TargetCount, HitLatency, Normalized HitLatency, Miss count]\n";

        //
        // save only the final results
        //
        if (actionType == "FINISH_RND") {
            (*_globalOut) << ts << "," << actionType << "," << id << "," << roundid;


            if (targetcount >= 0) {
                (*_globalOut) << "," << targetcount;
            }
            if (latency > 0) {
                (*_globalOut) << "," << latency;
            }
            if (distance > 0) {
//                (*_globalOut) << "," << distance;
                (*_globalOut) << "," << latency/distance; // normalized latency
            }
            if (missfortarget >= 0) {
                (*_globalOut) << "," << missfortarget;
            }




            //
            // The final results
            //
            if (avg_latency > 0) {
                (*_globalOut) << ",," << avg_latency;
            }
            if (avg_norm_latency > 0) {
                (*_globalOut) << "," << avg_norm_latency;
            }
            if (misscountround >= 0) {
                (*_globalOut) << "," << misscountround;
            }

            if (Rq >=0) {
                (*_globalOut) << "," << Rq;
            }
            if (Rc >=0) {
                (*_globalOut) << "," << Rc;
            }

            (*_globalOut) << "\n";
        }

        _rwlock.unlock();
    }

    QTextStream *appOut= 0;
    QMap<QChar, QTextStream *>::iterator it = _perAppOuts.find(id[0]);
    if (it != _perAppOuts.end()) {
        appOut = it.value();
    }
    else {
        qDebug() << "SN_FittsLawTestData::writeData() : Error !! There's no QTextStream for user" << id;
    }

    if (appOut) {
        if (actionType == "MISS") {
        }
        else {
            (*appOut) << ts << "," << actionType << "," << id << "," << roundid;

            if (actionType == "HIT"){

                if (targetcount >= 0) {
                    (*appOut) << "," << targetcount;
                }
                if (latency > 0) {
                    (*appOut) << "," << latency;
                }
                if (distance > 0) {
//                    (*appOut) << "," << distance;
                    (*appOut) << "," << latency/distance;
                }
                if (missfortarget >= 0) {
                    (*appOut) << "," << missfortarget;
                }

                if (Rq >= 0) {
                    (*appOut) << "," << Rq;
                }
                if (Rc >= 0) {
                    (*appOut) << "," << Rc;
                }
            }
            else if (actionType == "FINISH_RND") {

                (*appOut) << ",," << avg_latency;
    //            if (avg_norm_latency > 0) {
                    (*appOut) << "," << avg_norm_latency;
    //            }
    //            if (misscountround > 0) {
                    (*appOut) << "," << misscountround;
    //            }

                if (Rq >=0) {
                    (*appOut) << "," << Rq;
                }
                if (Rc >=0) {
                    (*appOut) << "," << Rc;
                }


				if (roundid == 0 || roundid == pow(2, SN_FittsLawTestData::_NUM_SUBJECTS) - 1) {
					(*appOut) << "\n";
				}
            }
            (*appOut) << "\n";
        }
    }
}

void SN_FittsLawTestData::recreateAllDataFiles() {

    _deleteFileAndStreamObjects();

    _currentID = QChar(); // reset

    _filenameBase.append("second_");

    QMap<QChar, SN_SageFittsLawTest *>::const_iterator it;
    for (it=_widgetMap.constBegin(); it!=_widgetMap.constEnd(); it++) {
        addWidget(it.value(), it.key());
    }
}

void SN_FittsLawTestData::_deleteFileAndStreamObjects() {
    if (_globalOut) {
        _globalOut->flush();
        delete _globalOut;
        _globalOut = 0;
    }
    if (_globalDataFile) {
        _globalDataFile->close();
        delete _globalDataFile;
        _globalDataFile = 0;
    }

    QMap<QChar, QTextStream *>::iterator itt;
    for (itt=_perAppOuts.begin(); itt!=_perAppOuts.end(); itt++) {
        QTextStream *out = itt.value();
        out->flush();
        delete out;
        _perAppOuts[itt.key()] = 0;
    }

    QMap<QChar, QFile *>::iterator itf;
    for (itf = _perAppDataFiles.begin(); itf!=_perAppDataFiles.end(); itf++) {
        QFile *f = itf.value();
        f->close();
        delete f;
        _perAppDataFiles[itf.key()] = 0;
    }
}

/*!
  This is called by each user whenever a test finished for the user
  */
void SN_FittsLawTestData::flushAll(const QString &id) {
    static int count = 0;

    count += 1;

    if (count == SN_FittsLawTestData::_NUM_SUBJECTS) {
        count = 0; // reset

        qDebug() << "\n===============================================================================";
        qDebug() << "FittsLawTestData::flushAll() : The test finished for all subjects !";
        qDebug() << "===============================================================================\n";

        if (_globalOut) _globalOut->flush();
//        if (_globalDataFile) _globalDataFile->close();

        QMap<QChar, QTextStream *>::iterator itt;
        for (itt=_perAppOuts.begin(); itt!=_perAppOuts.end(); itt++) {
            QTextStream *out = itt.value();
            out->flush();
        }

        QMap<QChar, QFile *>::iterator itf;
        for (itf = _perAppDataFiles.begin(); itf!=_perAppDataFiles.end(); itf++) {
            QFile *f = itf.value();
            f->flush();
//            f->close();
        }
    }
}
