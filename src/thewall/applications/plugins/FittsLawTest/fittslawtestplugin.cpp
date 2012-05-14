#include "fittslawtestplugin.h"
#include "common/sn_sharedpointer.h"
#include "system/resourcemonitor.h"
#include "applications/base/appinfo.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/sn_priority.h"

#include <QtGui>

FittsLawTestData * FittsLawTest::_dataObject = new FittsLawTestData;

const QString FittsLawTest::_streamerIpAddr = QString("127.0.0.1");

const QSize FittsLawTest::_streamImageSize = QSize(1920, 1080);

const QPixmap FittsLawTest::_startPixmap = QPixmap(":/greenplay128.png");

const QPixmap FittsLawTest::_stopPixmap = QPixmap(":/stopsign48.png").scaledToWidth(128);

const QPixmap FittsLawTest::_cursorPixmap = QPixmap(":/blackarrow_upleft128.png");



FittsLawTest::FittsLawTest()

    : SN_BaseWidget(Qt::Window)

    , _isRunning(false)
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

    , _myPointer(0)

//    , _recvThread(0)
//    , _recvObject(0)
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

    newwidget->_init();

    _dataObject->addWidget(newwidget);

    return newwidget;
}

void FittsLawTest::_init() {
    //
    // content area wher targets will appear
    //
    _contentWidget = new QGraphicsWidget;
    _contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::DefaultType);
    _contentWidget->setAcceptedMouseButtons(0);


    QPushButton *reset = new QPushButton("Reset roundCount");
    QPushButton *resetList = new QPushButton("Reset targetList");
    QPushButton *ready = new QPushButton("set READY");
    QPushButton *done = new QPushButton("Write Data");

    QObject::connect(ready, SIGNAL(clicked()), this, SLOT(setReady()));
    QObject::connect(reset, SIGNAL(clicked()), this, SLOT(resetTest()));
    QObject::connect(resetList, SIGNAL(clicked()), this, SLOT(clearTargetPosList()));
    QObject::connect(done, SIGNAL(clicked()), this, SLOT(finishTest()));

    _resetRoundButton = new QGraphicsProxyWidget(this);
    _resetTargetPosList = new QGraphicsProxyWidget(this);
    _readybutton = new QGraphicsProxyWidget(this);
    _donebutton = new QGraphicsProxyWidget(this);

    _resetTargetPosList->setWidget(resetList);
    _resetRoundButton->setWidget(reset);
    _readybutton->setWidget(ready);
    _donebutton->setWidget(done);

    QGraphicsLinearLayout *buttonlayout = new QGraphicsLinearLayout(Qt::Horizontal);
    buttonlayout->addItem(_readybutton);
    buttonlayout->addItem(_resetRoundButton);
    buttonlayout->addItem(_resetTargetPosList);
    buttonlayout->addItem(_donebutton);


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
    _target->setOpacity(0.5);



    _infoText = new QGraphicsSimpleTextItem(_contentWidget);
    _infoText->setAcceptedMouseButtons(0);
    _infoText->setFlag(QGraphicsItem::ItemStacksBehindParent);
    QFont f;
    f.setPointSize(32);
    _infoText->setFont(f);
    _showInfo = true;


    //
    // buttons on the top
    // content area on the bottom
    //
    QGraphicsLinearLayout *mainlayout = new QGraphicsLinearLayout(Qt::Vertical);
    mainlayout->addItem(buttonlayout);
    mainlayout->addItem(_contentWidget);

    setLayout(mainlayout);

    resize(1280, 1024);

    _appInfo->setExecutableName("fittslawteststreamer");
    _appInfo->setSrcAddr(_streamerIpAddr);

    //
    // signal slot connection for start/stop round
    //
    QObject::connect(this, SIGNAL(roundFinished()), this, SLOT(finishRound()));
    QObject::connect(this, SIGNAL(initClicked()), this, SLOT(startRound()));
}

FittsLawTest::~FittsLawTest() {
    if (isSchedulable() && _rMonitor) {
        qDebug() << "~FittsLawTest() : remove from rMonitor";
        _rMonitor->removeSchedulableWidget(this);
    }

    _myPointer = 0;

    disconnect(this);

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
//    qDebug() << "FittsLawTest::scheduleUpdate()";
    update();
}


int FittsLawTest::setQuality(qreal newQuality) {

    if (!_perfMon) return -1;

    if (newQuality <= 0) {
        //
        // this can hapeen when the requiredBW is 0
        //
        return -1;
    }

    if ( newQuality > 1.0 ) {
		_quality = 1.0;
	}
	else {
		_quality = newQuality;
	}

    if (_recvThread) {

        qreal bwallowed = _perfMon->getRequiredBW_Mbps(_quality); // demanded by the scheduler

//            qreal fpsallowed =  1e+6 * bwallowed / (_appInfo->frameSizeInByte() * 8.0f); // demanded by the scheduler

//            _recvThread->setExtraDelay_Msec( );

    }

    return -1;
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


    qDebug() << "FittsLawTest::launchExternalStreamer() : starting streamer. " << cmd;

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

//    painter->drawRect(mapRectFromItem(_contentWidget, _contentWidget->boundingRect()));

    if (_isRunning) {
//        qDebug() << "FittsLawTest::paint() drawing cursor";
        // draw cursor
        painter->drawPixmap(_cursorPoint, _cursorPixmap);
    }

    SN_BaseWidget::paint(painter, o, w);
}

bool FittsLawTest::handlePointerClick(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
    Q_UNUSED(btn);

    if (_isRunning) {
        // see if the click cocurred on the target (hit)
        // then show next dot
        if ( _myPointer == pointer && _target->contains( _target->mapFromItem(this, point).toPoint() )) {

//            qDebug() << "FittsLawTest::handlePointerClick() : hit";

            _targetHitTime = QDateTime::currentMSecsSinceEpoch(); // msec

            (_data.misscount)[ _roundCount * 10 + _targetHitCount] = _missCountPerTarget;
            (_data.hitlatency)[_roundCount * 10 + _targetHitCount] = _targetHitTime - _targetAppearTime;

            qDebug() << "ID" << _globalAppId << "R#" << _roundCount << "T#" << _targetHitCount << "miss" << _data.misscount.at(_roundCount *10 + _targetHitCount) << "delay" << _data.hitlatency.at(_roundCount *10 + _targetHitCount) << "msec";

            //
            // reset this upon successful hit
            //
            _missCountPerTarget = 0;

            emit hit();

            _targetHitCount++;

            //
            // if current round is compeleted
            //
            if (_targetHitCount == 10) {
                qDebug() << "FittsLawTest::handlePointerClick() : round #" << _roundCount << "finished";
                emit roundFinished();
                pointer->setOpacity(1);
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

            _missCountPerTarget++;

            emit miss();

            _missCountPerRound += _missCountPerTarget;

            _missCountTotal += _missCountPerTarget;

        }
    }
    else {

        //
        // User need to click the green rectangle to start
        //
        if (_isReady) {

            // see if the click occurred on the initating circle
            // if yes then start the process
            //
            if ( _startstop->contains(_startstop->mapFromItem(this, point).toPoint())) {

                _cursorPoint = point; // update cursor position

                _myPointer = pointer;

                //
                // trap the pointer so that it can only move within this widget
                //
                pointer->setTrapWidget(this);

                emit initClicked();

                pointer->setOpacity(0);

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

            qint64 currTS = QDateTime::currentMSecsSinceEpoch();
            _priorityData->setLastInteraction(SN_Priority::CONTENTS, currTS);


            //
            // variable frame rate based on mouse interaction
            //
            _sema.release(1); // create one resource


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

void FittsLawTest::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
    if (_isRunning) {
        // can't resize, can't moving while test is running
    }
    else {
        SN_BaseWidget::handlePointerPress(pointer, point, btn);
    }

    // can't resize all times
    _isResizing = false;
}

/*!
  invoked by the test controller (not a user)
  */
void FittsLawTest::setReady() {

    //
    // Run the external streamer process
    //
    if (!_extProgram) {
        launchExternalStreamer(_appInfo->srcAddr());
    }

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
    //
    // streaming resumes
    //

    if (!_recvThread) {
        _recvThread = new FittsLawTestStreamReceiverThread(_streamImageSize, 30, _streamerIpAddr, _globalAppId + 60000, _perfMon, &_sema);

        QObject::connect(_recvThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()));

        // connect to streamer
        if ( ! _recvThread->connectToStreamer() ) {
            qDebug() << "FittsLawTest::startRound() : failed to connect to the streamer" << _streamerIpAddr << _globalAppId + 60000;
        }
    }


    // will call QThread::start()
    _recvThread->resumeThreadLoop();


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

     _isRunning = true;
     _startstop->hide();
    _target->show();
}

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


    if (_recvThread)
        _recvThread->endThreadLoop();

    _isRunning = false;
    _isReady = false;
    _targetHitCount = 0;

    _missCountPerTarget = 0;
    _missCountPerRound = 0;

    if (_myPointer) {
        _myPointer->setOpacity(1);
        _myPointer->setTrapWidget(0); // reset
        _myPointer = 0;
    }

    _target->hide();
    _startstop->setPixmap(_stopPixmap);
    _startstop->show();

    //
    // increment round count
    //
    _roundCount++;

    if (_roundCount >= _NUM_ROUND) {
        qDebug() << _roundCount << "rounds finished";
        emit finishTest();
    }
}

void FittsLawTest::determineNextTargetPosition() {

    QPointF nextPos = _targetPosList.value(_roundCount * 10 + _targetHitCount);

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
        // Assumes the list is already filled with numm data
        //
        _targetPosList[ _roundCount * 10 + _targetHitCount ] = nextPos;

        qDebug() << _roundCount << _targetHitCount << "choose random pos" << nextPos;
    }

    Q_ASSERT(_target);
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

    QByteArray text(4096, '\0');

    sprintf(text.data(),
            "Round %d, Target %d \n P %.2f (Win %hu, Wal %hu, ipm %.3f) \n OQ_Rq %.1f OQ_Dq %.1f DQ %.1f \n CurBW %.3f, ReqBW %.3f \n allowedBW %.3f"
            , _roundCount, _targetHitCount

            , priority() /* qreal */
            , _priorityData->evrToWin() /* unsigned short - quint16 */
            , _priorityData->evrToWall()  /* unsigned short - quint16 */
            , _priorityData->ipm() /* qreal */

            , observedQuality_Rq(), observedQuality_Dq(), _quality
            , _perfMon->getCurrBW_Mbps(), _perfMon->getRequiredBW_Mbps()
            , _perfMon->getRequiredBW_Mbps(_quality)
            );

    _infoText->setText(QString(text));
}































FittsLawTestData::FittsLawTestData(QObject *parent)
    : QObject(parent)
    , _globalDataFile(0)
    , _globalOut(0)
{
    _globalDataFile = new QFile("/home/snam5/.sagenext/global.csv", this);
    if ( ! _globalDataFile->open(QIODevice::ReadWrite) ) {
        qDebug() << "FittsLawTestData::FittsLawTestData() failed to open" << _globalDataFile->fileName();
    }
    _globalOut = new QTextStream(_globalDataFile);
}

FittsLawTestData::~FittsLawTestData() {
    if (_globalOut)
        _globalOut->flush();

    if (_globalDataFile) {
        _globalDataFile->close();
    }


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

void FittsLawTestData::addWidget(FittsLawTest *widget) {
    Q_ASSERT(widget);

    static QChar id = QChar();

    if (id.isNull()) {
        id = 'A';
    }
    else if (id == 'A') {
        id = 'B';
    }
    else if (id == 'B') {
        id = 'C';
    }
    widget->setUserID(QString(id));
    _widgetMap.insert(id, widget);

    QString fname = "/home/snam5/.sagenext/";
    fname.append(id);

    QFile *f = new QFile(fname, this);
    QTextStream *out = new QTextStream(f);
    if ( ! f->open(QIODevice::ReadWrite) ) {
        qDebug() << "FittsLawTestData::addWidget() : failed to open a file" << f->fileName();
    }

    _perAppDataFiles.insert(id, f);
    _perAppOuts.insert(id, out);

}


void FittsLawTestData::writeData(const QString &id, const QByteArray &bytearry) {
    writeData(id, QString(bytearry));
}

void FittsLawTestData::writeData(const QString &id, const QString &str) {

    qint64 ts = QDateTime::currentMSecsSinceEpoch();

    if (_globalOut) {
        (*_globalOut) << ts << "," << str;
    }

    QTextStream *appOut= 0;
    QMap<QChar, QTextStream *>::iterator it = _perAppOuts.find(id[0]);
    if (it != _perAppOuts.end()) {
        appOut = it.value();
    }

    if (appOut) {
        (*appOut) << ts << "," << id << "," << str;
    }
}











// TARGET name, Class name
Q_EXPORT_PLUGIN2(FittsLawTestPlugin, FittsLawTest)



