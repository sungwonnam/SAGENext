#include "fittslawtestplugin.h"
#include "common/sn_sharedpointer.h"
#include "system/resourcemonitor.h"
#include "applications/base/appinfo.h"
#include "applications/base/perfmonitor.h"

#include <QtGui>

FittsLawTest::FittsLawTest()
    : SN_BaseWidget(Qt::Window)
    , _isRunning(false)
    , _isReady(false)
    , _testCount(0)
    , _contentWidget(new QGraphicsWidget)
    , _initCircle(0)
    , _target(0)
//    , _handle(new QGraphicsPixmapItem(QPixmap(":/blackhand_48.png"), this))

    , _resetbutton(new QGraphicsProxyWidget(this))
    , _readybutton(new QGraphicsProxyWidget(this))
    , _donebutton(new QGraphicsProxyWidget(this))

    , _myPointer(0)

    , _recvThread(0)
    , _extProgram(0)

{
    //
	// register myself to the hoveraccepting app list of the scene
	//
	setRegisterForMouseHover(true);


    //
    // Schedulable widget
    //
    setSchedulable();


    _cursorPixmap = QPixmap(":/blackhand_48.png");


    setContentsMargins(16, 40, 16, 16);


    //
    // initial GUI interacted with system mouse by operator
    //
    /*
    QLineEdit *le_userID = new QLineEdit("");
    QLineEdit *le_streamerIP = new QLineEdit("127.0.0.1");
    QLineEdit *le_width = new QLineEdit("1280");
    QLineEdit *le_height = new QLineEdit("1024");
    QPushButton *init  = new QPushButton("INIT");

    QGraphicsLinearLayout *initguilayout = new QGraphicsLinearLayout(Qt::Horizontal);
    */



    //
    // buttons for operator
    // remember these buttons will only react to real mouse
    //
    QPushButton *reset = new QPushButton("Reset");
    QPushButton *ready = new QPushButton("set READY");
    QPushButton *done = new QPushButton("Write Data");

    QObject::connect(ready, SIGNAL(clicked()), this, SLOT(setReady()));
    QObject::connect(reset, SIGNAL(clicked()), this, SLOT(resetTest()));
    QObject::connect(done, SIGNAL(clicked()), this, SLOT(finishTest()));

    _resetbutton->setWidget(reset);
    _readybutton->setWidget(ready);
    _donebutton->setWidget(done);

    QGraphicsLinearLayout *buttonlayout = new QGraphicsLinearLayout(Qt::Horizontal);
    buttonlayout->addItem(_readybutton);
    buttonlayout->addItem(_resetbutton);
    buttonlayout->addItem(_donebutton);



    //
    /// the green circle
    //
    _initCircle = new QGraphicsRectItem(0, 0, 128, 128, _contentWidget);

    //
    // yellow targets that the user will click
    //
    _target = new QGraphicsRectItem(0, 0, 64, 64, _contentWidget);

    _target->setFlag(QGraphicsItem::ItemStacksBehindParent);
    _target->setAcceptedMouseButtons(0);
    _target->setBrush(Qt::yellow);
    _target->hide();
    _target->setOpacity(0.5);

    _initCircle->setAcceptedMouseButtons(0);
    _initCircle->setBrush(Qt::green);
    _initCircle->hide();




    _contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::DefaultType);
    _contentWidget->setAcceptedMouseButtons(0);


    //
    // content's area where targets will be placed
    //
    QGraphicsLinearLayout *mainlayout = new QGraphicsLinearLayout(Qt::Vertical);
    mainlayout->addItem(buttonlayout);
    mainlayout->addItem(_contentWidget);

    setLayout(mainlayout);


    resize(1280, 1024);
    _streamImageSize = QSize(1280, 1024);
    _appInfo->setFrameSize(1280, 1024, 24);


    //
    // signal slot connection
    //
    QObject::connect(this, SIGNAL(roundFinished()), this, SLOT(resetTest()));
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
        }
        delete _recvThread;
    }

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

void FittsLawTest::launchExternalStreamer(const QString &ipaddr) {

    if (!_extProgram) {
        _extProgram = new QProcess(this);
    }

    QString cmd = "ssh -fx ";
    cmd.append(ipaddr);
    cmd.append(" ");
    cmd.append("fittslawteststreamer ");

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

void FittsLawTest::setSize(int w, int h) {
    resize(w, h);
}

void FittsLawTest::resizeEvent(QGraphicsSceneResizeEvent *event) {
    Q_UNUSED(event);
//    qDebug() << boundingRect() << _contentWidget->size();

    _initCircle->setPos( _contentWidget->size().width() / 2 - 64 , _contentWidget->size().height() / 2 - 64);
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

    if (_isRunning && _myPointer) {
        // see if the click cocurred on the target (hit)
        // then show next dot
        if ( _myPointer == pointer && _target->boundingRect().contains( _target->mapFromItem(this, point).toPoint() )) {

//            qDebug() << "FittsLawTest::handlePointerClick() : hit";

            _testCount++;

            if (_testCount == 10) {
                qDebug() << "FittsLawTest::handlePointerClick() : round finished";
                emit roundFinished();
                _testCount = 0;
                pointer->setOpacity(1.0);
            }
            else {
                determineNextTargetPosition();
            }
        }

        // otherwise it's miss
        else {
            qDebug() << "FittsLawTest::handlePointerClick() : missed  !";
            emit miss();
        }
    }
    else {

        if (_isReady) {
            // see if the click occurred on the initating circle
            // if yes then start the process

            _cursorPoint = point;

            _myPointer = pointer;

            emit initClicked();

            pointer->setOpacity(0.01);

            determineNextTargetPosition();
            _target->show();
            _testCount++;
        }


        // otherwise do nothing
    }


    //
    // prevent shared pointer from generating real mouse events
    //
    return true;
}

void FittsLawTest::handlePointerHover(SN_PolygonArrowPointer *pointer, const QPointF &point, bool isHovering) {

    if (isHovering) {
        if (_isRunning) {

            if (_myPointer && _myPointer == pointer) {
                pointer->setOpacity(0.0);
                _cursorPoint = point;
            }

        }
        else {
        }
    }
    else {
        // do nothing
        pointer->setOpacity(1.0);
    }

}

void FittsLawTest::handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
    if (_isRunning) {
        // can't resize, can't moving while test is running
    }
    else {
        SN_BaseWidget::handlePointerPress(pointer, point, btn);
    }
}

/*!
  invoked by the test controller (not a user)
  */
void FittsLawTest::setReady() {
    if (!_extProgram) {
        launchExternalStreamer("127.0.0.1");
    }


    _isReady = true;
    _testCount = 0;

    _initCircle->setBrush(Qt::green);
    _initCircle->show();
}

/*!
  user clicked the green circle
  */
void FittsLawTest::startRound() {
    //
    // streaming resumes
    //
    if (!_recvThread) {
        _recvThread = new FittsLawTestStreamReceiver(_streamImageSize, 30, _streamerIpAddr, _globalAppId + 60000, _perfMon);

        QObject::connect(_recvThread, SIGNAL(frameReceived()), this, SLOT(scheduleUpdate()));

        // connect to streamer
        if ( ! _recvThread->connectToStreamer() ) {
            qDebug() << "FittsLawTest::startRound() : failed to connect to the streamer" << _streamerIpAddr << _globalAppId + 60000;
        }
    }

    // will call QThread::start()
    _recvThread->resumeThreadLoop();


     _isRunning = true;
    _initCircle->hide();
    _target->show();
}

void FittsLawTest::determineNextTargetPosition() {

    int possible_x = _contentWidget->size().width() - _target->boundingRect().width();
    int possible_y = _contentWidget->size().height() - _target->boundingRect().height();

    int randi = qrand();
    qreal rand_x = (qreal)possible_x * ( (qreal)randi / (qreal)RAND_MAX ); // 0 ~ width

    randi = qrand();
    qreal rand_y = (qreal)possible_y * ( (qreal)randi / (qreal)RAND_MAX ); // 0 ~ height

    Q_ASSERT(_target);
    _target->setPos(rand_x, rand_y);
}

void FittsLawTest::resetTest() {

    _perfMon->setRequiredBandwidthMbps(0);
    _perfMon->setCurrBandwidthMbpsManual(0);

    _recvThread->endThreadLoop();

    _isRunning = false;
    _isReady = false;
    _testCount = 0;

    _myPointer = 0;

    _target->hide();
    _initCircle->hide();
}

// TARGET name, Class name
Q_EXPORT_PLUGIN2(FittsLawTestPlugin, FittsLawTest)