#include "fittslawtestplugin.h"
#include "common/sn_sharedpointer.h"
#include "system/resourcemonitor.h"
#include "applications/base/appinfo.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/sn_priority.h"

#include <QtGui>

int FittsLawTestData::_NUM_SUBJECTS = 1;
int FittsLawTest::_NUM_TARGET_PER_ROUND = 2;

// 131.193.78.176 (bigdaddy 100 Mbps)
// 67.58.62.57 (bigdaddy 10 Gbps)
// 67.58.62.45 (venom 10 Gbps)
const QString FittsLawTest::_streamerIpAddr = QString("127.0.0.1");
const QSize FittsLawTest::_streamImageSize = QSize(1920, 1080);







FittsLawTestData * FittsLawTest::_dataObject = new FittsLawTestData;
int FittsLawTest::RoundID = -1;
int FittsLawTest::_NUM_ROUND = pow(2, FittsLawTestData::_NUM_SUBJECTS - 1);
const QPixmap FittsLawTest::_startPixmap = QPixmap(":/greenplay128.png");
const QPixmap FittsLawTest::_stopPixmap = QPixmap(":/stopsign48.png").scaledToWidth(128);
const QPixmap FittsLawTest::_cursorPixmap = QPixmap(":/blackarrow_upleft128.png");



FittsLawTest::FittsLawTest()

    : SN_BaseWidget(Qt::Window)

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

    , _recvThread(0)
    , _extProgram(0)

    , _missCountPerTarget(0)
    , _missCountTotal(0)
    , _targetAppearTime(0.0)
    , _targetHitTime(0.0)
{
    //
    // set Schedulable widget
    // SN_Launcher will be able to add this widget to rMonitor's widget list
    // upon launching this widget.
    //
    setSchedulable();

    setContentsMargins(16, 40, 16, 16);
}

SN_BaseWidget * FittsLawTest::createInstance() {
    FittsLawTest *newwidget = new FittsLawTest;

    _dataObject->addWidget(newwidget);

    newwidget->_init();

    return newwidget;
}

void FittsLawTest::_init() {
    clearTargetPosList();
    clearData();

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
//    _lbl_roundid = new QLabel(QString::number(FittsLawTest::RoundID));
    _lbl_tgtcount = new QLabel("tgt cnt");
    _lbl_hitlatency = new QLabel("hit lat");

    _lblproxy_userid = new QGraphicsProxyWidget(this);
    _lblproxy_tgtcount = new QGraphicsProxyWidget(this);
    _lblproxy_hitlatency = new QGraphicsProxyWidget(this);
    _lblproxy_userid->setWidget(_lbl_userid);
    _lblproxy_tgtcount->setWidget(_lbl_tgtcount);
    _lblproxy_hitlatency->setWidget(_lbl_hitlatency);

    QGraphicsLinearLayout *toplayout = new QGraphicsLinearLayout(Qt::Horizontal);
    /*
    buttonlayout->addItem(_readybutton);
    buttonlayout->addItem(_resetRoundButton);
    buttonlayout->addItem(_resetTargetPosList);
    buttonlayout->addItem(_donebutton);
    */
    toplayout->addItem(_lblproxy_userid);
    toplayout->addItem(_lblproxy_tgtcount);
    toplayout->addItem(_lblproxy_hitlatency);


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
    QFont f;
    f.setPointSize(24);
    _infoText->setFont(f);
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

    resize(640, 480);

    _appInfo->setExecutableName("fittslawteststreamer");
    _appInfo->setSrcAddr(_streamerIpAddr);

    //
    // signal slot connection for start/stop round
    //
    QObject::connect(this, SIGNAL(initClicked()), this, SLOT(startRound()));

    QObject::connect(this, SIGNAL(destroyed(QObject *)), _dataObject, SLOT(closeApp(QObject *)));
}

FittsLawTest::~FittsLawTest() {
    if (isSchedulable() && _rMonitor) {
        qDebug() << "~FittsLawTest() : remove from rMonitor";
        _rMonitor->removeSchedulableWidget(this);
    }

    _myPointer = 0;

    disconnect(this);

    if (_dataObject) {
        delete _dataObject;
        _dataObject = 0;
    }

    if (_recvThread) {
        if (_recvThread->isRunning()) {
            _recvThread->endThreadLoop();
            _sema.release();
        }
        delete _recvThread;
    }

    /*
    if (_recvObject) {
        if (_thread.isRunning()) {
            _thread.quit();
        }
        delete _recvObject;
    }
    */

    if (_extProgram) {
        if (_extProgram->state() == QProcess::Running) {
            _extProgram->kill();
        }
//        delete _extProgram; // this is automatic since _extProgram is child of this widget
    }
}

void FittsLawTest::scheduleUpdate() {
    update();
}


int FittsLawTest::setQuality(qreal newQuality) {

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

        qreal delay_demanded = 1000 / fps_demanded; // in msec



        qreal fps_current = 1e+6 * _perfMon->getCurrBW_Mbps() / (_appInfo->frameSizeInByte() * 8.0f); // curent effective fps

        qreal delay_current = 1000 / fps_current; // in msec


        thedelay = delay_demanded - delay_current;
	}


    if (_recvThread) {
        if ( ! QMetaObject::invokeMethod(_recvThread, "setExtraDelay_Msec", Qt::QueuedConnection, Q_ARG(unsigned long, thedelay)) ) {
            qDebug() << "FittsLawTest::setQuality() : failed to invoke _recvThread->setExtraDelay_Msec()";
            return -1;
        }
        return 0;
    }
    else {
        qDebug() << "FittsLawTest::setQuality() : _recvThread is null";
        return -1;
    }
}


/*!
  setReady() will call this
  if _extProgram doesn't exist
  */
void FittsLawTest::launchExternalStreamer(const QString &ipaddr) {

    if (!_extProgram) {
        _extProgram = new QProcess(this);
    }

    QString cmd = "ssh -fx ";
    cmd.append(ipaddr);
    cmd.append(" ");
    cmd.append(_appInfo->executableName());
    cmd.append(" ");

    cmd.append(QString::number(_streamImageSize.width()));
    cmd.append(" ");
    cmd.append(QString::number(_streamImageSize.height()));
    cmd.append(" ");

    cmd.append("30 "); // frate

    //
    // _globalAppId is valid after the constructor returns
    //
    cmd.append(QString::number(_globalAppId + 60000));


    qDebug() << "FittsLawTest::launchExternalStreamer() : starting the streamer for user ID" << _userID << "[" << cmd << "]";

    _extProgram->start(cmd);

    if ( ! _extProgram->waitForStarted(-1)) {
        qCritical() << "FittsLawTest::launchExternalStreamer() : failed to start external streamer";
//        deleteLater();
        close();
    }
}

/*
bool FittsLawTest::setOverheadSize(int w, int h) {
    return setOverheadSize(QSize(w,h));
}

bool FittsLawTest::setOverheadSize(const QSize &size) {
    if (_extProgram && _extProgram->state() == QProcess::Running) {
        qDebug() << "FittsLawTest::setOverheadSize() : set this before running the streamer";
        return false;
    }

    _streamImageSize = size;
    _appInfo->setFrameSize(size.width(), size.height(), 24); // 3 Byte per pixel
    return true;
}

bool FittsLawTest::setStreamerIP(const QString &str) {
    if (_extProgram && _extProgram->state() == QProcess::Running) {
        qDebug() << "FittsLawTest::setStreamerIP() : set this before running the streamer";
        return false;
    }

    _streamerIpAddr = str;
    _appInfo->setSrcAddr(str);

    return true;
}
*/

void FittsLawTest::resizeEvent(QGraphicsSceneResizeEvent *event) {
    Q_UNUSED(event);
//    qDebug() << boundingRect() << _contentWidget->size();
    _startstop->setPos( _contentWidget->size().width() / 2 - 64 , _contentWidget->size().height() / 2 - 64);
}

void FittsLawTest::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    // do nothing
}

void FittsLawTest::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {

    if (_isRunning) {
//        qDebug() << "FittsLawTest::paint()";
        // draw cursor
//        painter->drawPixmap(_cursorPoint, _cursorPixmap);

        _cursor->setPos( mapToItem(_contentWidget, _cursorPoint) );
    }

    SN_BaseWidget::paint(painter, o, w);
}

bool FittsLawTest::handlePointerClick(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
    Q_UNUSED(btn);

    if (_isRunning) {
        //
        // see if the click cocurred on the target (hit)
        // then show next target
        //
        if ( _myPointer == pointer && _target->contains( _target->mapFromItem(this, point).toPoint() )) {

//            qDebug() << "FittsLawTest::handlePointerClick() : hit";

            _targetHitTime = QDateTime::currentMSecsSinceEpoch(); // msec

//            (_data.misscount)[ _roundCount * FittsLawTest::_NUM_TARGET_PER_ROUND + _targetHitCount] = _missCountPerTarget;
//            (_data.hitlatency)[_roundCount * FittsLawTest::_NUM_TARGET_PER_ROUND + _targetHitCount] = _targetHitTime - _targetAppearTime;

            qint64 hitlatency = _targetHitTime - _targetAppearTime;

//            qDebug() << "ID" << _globalAppId << "R#" << _roundCount << "T#" << _targetHitCount << "miss" << _data.misscount.at(_roundCount *10 + _targetHitCount) << "delay" << _data.hitlatency.at(_roundCount *10 + _targetHitCount) << "msec";

            //
            // reset this upon successful hit
            //
            _missCountPerTarget = 0;

            if (!_isDryRun)
                _dataObject->writeData(_userID, "HIT", FittsLawTest::RoundID, _targetHitCount, QString::number(hitlatency));

            _lbl_hitlatency->setText(QString::number(hitlatency));

            _targetHitCount++; // increment for next target

            //
            // now display next target count
            //
            _lbl_tgtcount->setText(QString::number(_targetHitCount));


            //
            // if current round is compeleted
            //
            if (_targetHitCount == FittsLawTest::_NUM_TARGET_PER_ROUND) {
//                qDebug() << "FittsLawTest::handlePointerClick() : round #" << _roundCount << "finished";
                pointer->setOpacity(1);
                finishRound();
            }

            //
            // otherwise, show next target
            //
            else {
                determineNextTargetPosition();
            }
        }

        //
        // otherwise it's miss
        //
        else {
//            qDebug() << "FittsLawTest::handlePointerClick() : missed  !";
            if (!_isDryRun) {
                _missCountPerTarget++;

                _dataObject->writeData(_userID, "MISS", FittsLawTest::RoundID, _targetHitCount, QString());

                _missCountPerRound += _missCountPerTarget;
                _missCountTotal += _missCountPerTarget;
            }
        }
    }

    //
    // Test is not running then
    //
    else {

        //
        // User need to click the green rectangle to start
        //
        if (_isReady) {
            //
            // See if the click occurred on the initating green icon
            // if yes then start the process
            //
            if ( _startstop->contains(_startstop->mapFromItem(this, point).toPoint())) {

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
                determineNextTargetPosition();

                _target->show();
            }
        }

        // otherwise do nothing
    }


    //
    // prevent shared pointer from generating real mouse events
    //
    return true;
}

void FittsLawTest::handlePointerDrag(SN_PolygonArrowPointer *pointer, const QPointF &point, qreal dx, qreal dy, Qt::MouseButton btn, Qt::KeyboardModifier mod) {
    if (_isRunning) {
        if (pointer == _myPointer) {

			if (_priorityData) {
				qint64 currTS = QDateTime::currentMSecsSinceEpoch();
				_priorityData->setLastInteraction(SN_Priority::CONTENTS, currTS);
			}


            //
            // variable frame rate based on mouse interaction
            //
            if (_recvThread && _recvThread->isRunning()) {
                //
                // create one resource for the stream receiver
                // upon receiving a frame, the receiver thread will emit frameReceived()
                // which is connected up scheduleUpdate()
                //
                _sema.release(1);
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

void FittsLawTest::handlePointerPress(SN_PolygonArrowPointer *, const QPointF &point, Qt::MouseButton btn) {

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

/*!
  invoked by the test controller (not a user)
  */
void FittsLawTest::setReady(bool isDryrun /* false */) {
    //
    // Run the external streamer process
    //
    if (!_extProgram) {
        launchExternalStreamer(_appInfo->srcAddr());
    }

    _isDryRun = isDryrun;

    _isReady = true;
    _targetHitCount = 0;

    if (_roundCount >= _NUM_ROUND) {
        _roundCount = 0;
    }

    _startstop->setPixmap(_startPixmap);
    _startstop->show();
}

/*!
  A user clicked the green play icon.
  (Upon receiving initClicked() signal)
  */
void FittsLawTest::startRound() {

    if (!_recvThread) {
        _recvThread = new FittsLawTestStreamReceiverThread(_streamImageSize, 30, _streamerIpAddr, _globalAppId + 60000, _perfMon, &_sema);

        QObject::connect(_recvThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()));

		// connect to streamer
		QMetaObject::invokeMethod(_recvThread, "connectToStreamer", Qt::QueuedConnection);
		QApplication::sendPostedEvents();
		QCoreApplication::processEvents();

		/*
        if ( ! _recvThread->connectToStreamer() ) {
            qDebug() << "FittsLawTest::startRound() : failed to connect to the streamer" << _streamerIpAddr << _globalAppId + 60000;
        }
*/
    }

    _targetHitCount = 0;
    _isRunning = true;
    _startstop->hide();
    _target->show();


    if (!_isDryRun) {
        _dataObject->writeData(_userID, QString("START_RND"), FittsLawTest::RoundID);

        //
        // streaming resumes
        // will call QThread::start()
		//
		///_recvThread->resumeThreadLoop();
		QMetaObject::invokeMethod(_recvThread, "resumeThreadLoop", Qt::AutoConnection);
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
void FittsLawTest::finishRound() {
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

    _lbl_tgtcount->setText("tgt cnt");
    _lbl_hitlatency->setText("hit lat");


    if (_recvThread) {
        //_recvThread->endThreadLoop();
	QMetaObject::invokeMethod(_recvThread, "endThreadLoop", Qt::AutoConnection);
	QApplication::sendPostedEvents();
	QCoreApplication::processEvents();
	}

    _isRunning = false;
    _isReady = false;
    _targetHitCount = 0;

    _missCountPerTarget = 0;
    _missCountPerRound = 0;

    if (_myPointer) {
        _myPointer->setOpacity(1);
        _myPointer->setTrapWidget(0); // reset the trapped pointer
        _myPointer = 0;
    }

    _target->hide();
    _startstop->setPixmap(_stopPixmap);
    _startstop->show();

    //
    // increment round count
    //
    if (!_isDryRun) {
        _roundCount++;

        _dataObject->writeData(_userID, "FINISH_RND", FittsLawTest::RoundID);

        if (_roundCount >= _NUM_ROUND) {
            qDebug() << "FittsLawTest::finishRound() : A test has completed for user" << _userID;

            //
            // reset round count
            //
            _roundCount = 0;

            //
            // close files
            //
            _dataObject->flushCloseAll();
        }
    }
    else {
        qDebug() << "FittsLawTest::finishRound() : A DRY round finished for user" << _userID;
    }
}

void FittsLawTest::determineNextTargetPosition() {

    QPointF nextPos = _targetPosList.value(_roundCount * FittsLawTest::_NUM_TARGET_PER_ROUND + _targetHitCount);

    //
    // there's saved value
    //
    if (! nextPos.isNull() ) {
        qDebug() << _roundCount << _targetHitCount << "has point data" << nextPos;
    }

    //
    // get value and save it
    //
    else {
        nextPos = m_getRandomPos();

        //
        // Assumes the list is already filled with null data (call clearTargetPosList() !!)
        //
        if (!_isDryRun)
            _targetPosList[ _roundCount * FittsLawTest::_NUM_TARGET_PER_ROUND + _targetHitCount ] = nextPos;

//        qDebug() << _roundCount << _targetHitCount << "choose random pos" << nextPos;
    }

    Q_ASSERT(_target);

    qreal dx = nextPos.x() - _target->pos().x();
    qreal dy = nextPos.y() - _target->pos().y();
    _tgtDistance = sqrt(dx*dx + dy*dy);

    _target->setPos(nextPos);

    _targetAppearTime = QDateTime::currentMSecsSinceEpoch(); // msec
}


void FittsLawTest::resetTest() {
    finishRound();
    _roundCount = 0;
}

void FittsLawTest::clearTargetPosList() {
    _roundCount = 0;
    _targetPosList.clear();

    for (int i=0; i< _NUM_ROUND * _NUM_TARGET_PER_ROUND; i++) {
        _targetPosList.append( QPointF() ); // init with null point
    }
}

void FittsLawTest::clearData() {
    for (int i=0; i< _NUM_ROUND * _NUM_TARGET_PER_ROUND; i++) {

        _data.misscount.append(0);
        _data.hitlatency.append(0.0);
    }
}

QPointF FittsLawTest::m_getRandomPos() {
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

void FittsLawTest::updateInfoTextItem() {
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

void FittsLawTest::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setBrush(Qt::lightGray);
    painter->drawRect(windowFrameRect());
}






























FittsLawTestData::FittsLawTestData(QObject *parent)
    : QObject(parent)
    , _globalDataFile(0)
    , _globalOut(0)
    , _currentID(QChar())
    , _frame(0)
    , _isDryRun(true)
{
//    qDebug() << "FittsLawTestData::FittsLawTestData()";
    _filenameBase = QDir::homePath().append("/.sagenext/");
}

FittsLawTestData::~FittsLawTestData() {
    qDebug() << "FittsLawTestData::~FittsLawTestData()";

    if (_frame) {
        delete _frame;
    }
    _deleteFileAndStreamObjects();

    _widgetMap.clear();
    _perAppDataFiles.clear();
    _perAppOuts.clear();
}

void FittsLawTestData::m_createGUI() {

    if (!_frame) {
        _frame = new QFrame;

        _frame->setWindowTitle("FittsLawTest Controller");

        QPushButton *nextRnd = new QPushButton("NextRound");
        QObject::connect(nextRnd, SIGNAL(clicked()), this, SLOT(advanceRound()));

        QPushButton *recreateFiles = new QPushButton("RecreateDataFiles");
        QObject::connect(recreateFiles, SIGNAL(clicked()), this, SLOT(recreateAllDataFiles()));

        QPushButton *close = new QPushButton("Finish");
        QObject::connect(close, SIGNAL(clicked()), this, SLOT(closeAll()));

        QVBoxLayout *hl = new QVBoxLayout;
        hl->addWidget(nextRnd);
        hl->addWidget(recreateFiles);
        hl->addWidget(close);
        _frame->setLayout(hl);
    }

    _frame->show();
}

void FittsLawTestData::closeAll() {
    QMap<QChar, FittsLawTest *>::iterator it;
    for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
        FittsLawTest *widget = it.value();
        if (widget) {
            widget->close();
            widget->deleteLater();
        }
    }
    deleteLater();
}

void FittsLawTestData::closeApp(QObject *obj) {
    FittsLawTest *widget = dynamic_cast<FittsLawTest *>(obj);
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

void FittsLawTestData::advanceRound() {
    //
    // increment the RoundID
    //
    FittsLawTest::RoundID += 1;
    if ( FittsLawTest::RoundID >= pow(2, FittsLawTestData::_NUM_SUBJECTS) ) {
        FittsLawTest::RoundID = 0;
    }

    qDebug() << "\nFittsLawTestData::advanceRound() : Round ID" << FittsLawTest::RoundID;

    Q_ASSERT(_widgetMap.size() == FittsLawTestData::_NUM_SUBJECTS);

    if (_globalOut) _globalOut->flush();

    switch (FittsLawTest::RoundID) {
    case 0: {
        if (_isDryRun) {

            //
            // setReady all
            //
            QMap<QChar, FittsLawTest *>::iterator it;
            for (it=_widgetMap.begin(); it!=_widgetMap.end(); it++) {
                FittsLawTest *widget = it.value();
                widget->setReady(true); // DRY run
            }

            //
            // now reset the dryrun flag
            //
            _isDryRun = false;

        }
        break;
    }
    case 1: {
        Q_ASSERT(_widgetMap.value('A', 0));
        // 0
        // 0
        (_widgetMap['A'])->setReady();
        break;
    }
    case 2: {
        Q_ASSERT(_widgetMap.value('B', 0));

        // 0
        (_widgetMap['B'])->setReady();
        // 0
        break;
    }
    case 3: {
        // 0
        (_widgetMap['B'])->setReady();
        (_widgetMap['A'])->setReady();
        break;
    }
    case 4: {
        Q_ASSERT(_widgetMap.value('C', 0));
        (_widgetMap['C'])->setReady();
        // 0
        // 0
        break;
    }
    case 5: {
        (_widgetMap['C'])->setReady();
        // 0
        (_widgetMap['A'])->setReady();
        break;
    }
    case 6: {
        (_widgetMap['C'])->setReady();
        (_widgetMap['B'])->setReady();
        // 0
        break;
    }
    case 7: {
        (_widgetMap['C'])->setReady();
        (_widgetMap['B'])->setReady();
        (_widgetMap['A'])->setReady();
        break;
    }
    }


}

bool FittsLawTestData::_openGlobalDataFile() {
    QString fname = _filenameBase;
    _globalDataFile = new QFile(fname.append("global.csv"));
    if ( ! _globalDataFile->open(QIODevice::ReadWrite) ) {
        qDebug() << "FittsLawTestData::openFile() failed to open" << _globalDataFile->fileName();
        return false;
    }
    _globalOut = new QTextStream(_globalDataFile);

//    (*_globalOut) << "# users" << FittsLawTestData::_NUM_SUBJECTS << "StreamerIP" << FittsLawTest::_streamerIpAddr << "Overhead" << FittsLawTest::_streamImageSize;
    (*_globalOut) << "TimeStamp, ActionType, UserID, RoundID, TargetCount, HitLatency\n";

    return true;
}

bool FittsLawTestData::_openPerAppDataFile(const QChar id) {
    QString fname = _filenameBase;

    QFile *f = new QFile(fname.append(QString(id)));
    QTextStream *out = new QTextStream(f);

    if ( ! f->open(QIODevice::ReadWrite) ) {
        qDebug() << "FittsLawTestData::openPerAppDataFile() : failed to open a file" << f->fileName();
        return false;
    }

    _perAppDataFiles.insert(id, f);
    _perAppOuts.insert(id, out);

    return true;
}

void FittsLawTestData::addWidget(FittsLawTest *widget, const QChar id) {
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

    if (_widgetMap.size() == FittsLawTestData::_NUM_SUBJECTS) {
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

void FittsLawTestData::writeData(const QString &id, const QString &actionType, int roundid, int targetcount, const QString &data) {

    qint64 ts = QDateTime::currentMSecsSinceEpoch();

    // START_RND
    // FINISH_RND
    // HIT , targetcount, latency
    // MISS, targetcount

    if (_globalOut) {

        _rwlock.lockForWrite();

//        (*_globalOut) << "TimeStamp, ActionType, UserID, RoundID [,TargetCount, HitLatency]\n";
        (*_globalOut) << ts << "," << actionType << "," << id << "," << roundid;
        if (targetcount >= 0) {
            (*_globalOut) << "," << targetcount;
        }
        if (!data.isNull()) {
            (*_globalOut) << "," << data;
        }
        (*_globalOut) << "\n";

        _rwlock.unlock();
    }

    QTextStream *appOut= 0;
    QMap<QChar, QTextStream *>::iterator it = _perAppOuts.find(id[0]);
    if (it != _perAppOuts.end()) {
        appOut = it.value();
    }

    if (appOut) {
        (*appOut) << ts << "," << actionType << "," << id << "," << roundid;
        if (targetcount >= 0) {
            (*appOut) << "," << targetcount;
        }
        if (!data.isNull()) {
            (*appOut) << "," << data;
        }
        (*appOut) << "\n";
    }
}

void FittsLawTestData::recreateAllDataFiles() {

    _deleteFileAndStreamObjects();

    _currentID = QChar(); // reset

    _filenameBase.append("second_");

    QMap<QChar, FittsLawTest *>::const_iterator it;
    for (it=_widgetMap.constBegin(); it!=_widgetMap.constEnd(); it++) {
        addWidget(it.value(), it.key());
    }
}

void FittsLawTestData::_deleteFileAndStreamObjects() {
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
  this is called whenever a round finished
  */
void FittsLawTestData::flushCloseAll() {
    static int count = 0;

    count += 1;

    if (count == FittsLawTestData::_NUM_SUBJECTS) {
        count = 0; // reset

        qDebug() << "FittsLawTestData::flushCloseAll() : The tests finished for all subjects ! \n";

        if (_globalOut) _globalOut->flush();
        if (_globalDataFile) _globalDataFile->close();

        QMap<QChar, QTextStream *>::iterator itt;
        for (itt=_perAppOuts.begin(); itt!=_perAppOuts.end(); itt++) {
            QTextStream *out = itt.value();
            out->flush();
        }

        QMap<QChar, QFile *>::iterator itf;
        for (itf = _perAppDataFiles.begin(); itf!=_perAppDataFiles.end(); itf++) {
            QFile *f = itf.value();
            f->close();
        }
    }
}










// TARGET name, Class name
Q_EXPORT_PLUGIN2(FittsLawTestPlugin, FittsLawTest)



