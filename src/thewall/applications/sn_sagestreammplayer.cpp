#include "sn_sagestreammplayer.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"

SN_SageStreamMplayer::SN_SageStreamMplayer(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_SageStreamWidget(globalappid, s, rm, parent, wFlags)
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

	connect(_rewindButton, SIGNAL(clicked(int)), this, SLOT(rewindMplayer(int)));
	connect(_pauseButton, SIGNAL(clicked(int)), this, SLOT(pauseMplayer(int)));
	connect(_playButton, SIGNAL(clicked(int)), this, SLOT(playMplayer(int)));
	connect(_fforwardButton, SIGNAL(clicked(int)), this, SLOT(fforwardMplayer(int)));
}

void SN_SageStreamMplayer::setButtonPosition() {
    _rewindButton->setPos(0, boundingRect().bottom() - _rewindButton->size().height());
    _pauseButton->setPos(_rewindButton->geometry().topRight());
    _playButton->setPos(_rewindButton->geometry().topRight());
    _fforwardButton->setPos(_playButton->geometry().topRight());
}

void SN_SageStreamMplayer::rewindMplayer(int p) {
	Q_ASSERT(_fsmMsgThread);
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("rewind")));
	_playButton->hide();
	_pauseButton->show();
}

void SN_SageStreamMplayer::pauseMplayer(int p) {
	Q_ASSERT(_fsmMsgThread);
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("pause")));
	_pauseButton->hide();
	_playButton->show();

//	_perfMon->setAdjustedFps(0);
    _perfMon->setRequiredBW_Mbps(0);
}

void SN_SageStreamMplayer::playMplayer(int p) {
	Q_ASSERT(_fsmMsgThread);
//	_fsmMsgThread->sendSailMsg(OldSage::EVT_KEY, "play");
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("play")));
	_playButton->hide();
	_pauseButton->show();

    _perfMon->setRequiredBW_Mbps( _appInfo->frameSizeInByte() * 8.0f * _perfMon->getExpetctedFps() * 1e-6 );
}

void SN_SageStreamMplayer::fforwardMplayer(int p) {
	Q_ASSERT(_fsmMsgThread);
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("forward")));
	_playButton->hide();
	_pauseButton->show();
}
