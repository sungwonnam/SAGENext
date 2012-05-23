#include "sn_sagestreammplayer.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"
#include "applications/base/sagepixelreceiver.h"

SN_SageStreamMplayer::SN_SageStreamMplayer(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_SageStreamWidget(globalappid, s, rm, parent, wFlags)
    , _isMplayerPaused(false)
{

    QObject::connect(this, SIGNAL(streamerInitialized()), this, SLOT(setButtonPosition()));

	_rewindButton = new SN_PixmapButton(":/resources/media-forward-rtl_128x128.png", 0, "", this);
	_pauseButton = new SN_PixmapButton(":/resources/media-pause-128x128.png", 0, "", this);
	_playButton = new SN_PixmapButton(":/resources/media-play-ltr_128x128.png", 0, "", this);
	_playButton->hide();
	_fforwardButton = new SN_PixmapButton(":/resources/media-forward-ltr_128x128.png", 0, "", this);

	_rewindButton->setPriorityOverride(1);
	_playButton->setPriorityOverride(1);
	_pauseButton->setPriorityOverride(-1);
	_fforwardButton->setPriorityOverride(1);

	connect(_rewindButton, SIGNAL(clicked(int)), this, SLOT(rewindMplayer()));
	connect(_pauseButton, SIGNAL(clicked(int)), this, SLOT(pauseMplayer()));
	connect(_playButton, SIGNAL(clicked(int)), this, SLOT(playMplayer()));
	connect(_fforwardButton, SIGNAL(clicked(int)), this, SLOT(fforwardMplayer()));
}

int SN_SageStreamMplayer::setQuality(qreal newQuality) {
    if (!_perfMon) return -1;

    if (_quality == newQuality) {
        return 0;
    }

    unsigned long delayneeded = 0;

    if ( newQuality >= 1.0 ) {
		_quality = 1.0;
        delayneeded = 0;
	}

    //
    // this can happen
    // when this app's requiredBW is set to 0.
    // or its priority is 0 (completely obscured by other widget)
    //
    // And it means the streamer (SAGE app) isn't sending any pixel
    //
	else if ( newQuality <= 0.0 ) {
		_quality = 0.0;
        if (!_isMplayerPaused)
            pauseMplayer();
        return 0;
	}

	else {
		_quality = newQuality;

        qreal newfps = _quality * _perfMon->getExpetctedFps(); // based on the priori
        delayneeded = 1000 * ((1.0/newfps) - (1.0/_perfMon->getExpetctedFps())); // in msec
	}


    if (_isMplayerPaused) {
        playMplayer();
    }


    if (_receiverThread) {
        if ( ! QMetaObject::invokeMethod(_receiverThread, "setDelay_msec", Qt::QueuedConnection, Q_ARG(unsigned long, delayneeded)) ) {
            qDebug() << "SN_SageStreamMplayer::setQuality() : failed to invoke setDelay_msec()";
            return -1;
        }
    }
    else {
        qDebug() << "SN_SageStreamMplayer::setQuality() : _receiverThread is null";
        return -1;
    }

    return 0;
}

void SN_SageStreamMplayer::setButtonPosition() {
    _rewindButton->setPos(0, boundingRect().bottom() - _rewindButton->size().height());
    _pauseButton->setPos(_rewindButton->geometry().topRight());
    _playButton->setPos(_rewindButton->geometry().topRight());
    _fforwardButton->setPos(_playButton->geometry().topRight());
}

void SN_SageStreamMplayer::rewindMplayer() {
	Q_ASSERT(_fsmMsgThread);
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("rewind")));
	_playButton->hide();
	_pauseButton->show();
}

void SN_SageStreamMplayer::pauseMplayer() {
	Q_ASSERT(_fsmMsgThread);
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("pause")));
	_pauseButton->hide();
	_playButton->show();

    _isMplayerPaused = true;

    qDebug() << "SN_SageStreamMplayer::pauseMplayer() : Paused";
}

void SN_SageStreamMplayer::playMplayer() {
	Q_ASSERT(_fsmMsgThread);
//	_fsmMsgThread->sendSailMsg(OldSage::EVT_KEY, "play");
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("play")));
	_playButton->hide();
	_pauseButton->show();

    _isMplayerPaused = false;

    qDebug() << "SN_SageStreamMplayer::pauseMplayer() : Resume";
}

void SN_SageStreamMplayer::fforwardMplayer() {
	Q_ASSERT(_fsmMsgThread);
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("forward")));
	_playButton->hide();
	_pauseButton->show();
}
