#include "sn_sagestreammplayer.h"
#include "applications/base/perfmonitor.h"
#include "applications/base/appinfo.h"
#include "applications/base/sagepixelreceiver.h"

SN_SageStreamMplayer::SN_SageStreamMplayer(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_SageStreamWidget(globalappid, s, rm, parent, wFlags)
    , _isMplayerPaused(false)
{

    QObject::connect(this, SIGNAL(streamerInitialized()), this, SLOT(setButtonPosition()));

	_rewindButton = new SN_PixmapButton(":/resources/media-forward-rtl_128x128.png", 64, "", this);
	_pauseButton = new SN_PixmapButton(":/resources/media-pause-128x128.png", 64, "", this);
	_playButton = new SN_PixmapButton(":/resources/media-play-ltr_128x128.png", 64, "", this);
	_playButton->hide();
	_fforwardButton = new SN_PixmapButton(":/resources/media-forward-ltr_128x128.png", 64, "", this);

	/*
	_rewindButton->setPriorityOverride(1);
	_playButton->setPriorityOverride(1);
	_pauseButton->setPriorityOverride(-1);
	_fforwardButton->setPriorityOverride(1);
	*/

	QObject::connect(_rewindButton, SIGNAL(clicked()), this, SLOT(rewindMplayer()));
	QObject::connect(_pauseButton, SIGNAL(clicked()), this, SLOT(pauseMplayer()));
	QObject::connect(_playButton, SIGNAL(clicked()), this, SLOT(playMplayer()));
	QObject::connect(_fforwardButton, SIGNAL(clicked()), this, SLOT(fforwardMplayer()));

}


void SN_SageStreamMplayer::setButtonPosition() {
    int buttonswidth = _rewindButton->size().width() + _playButton->size().width() + _fforwardButton->size().width();
    _rewindButton->setPos((boundingRect().width() - buttonswidth)/2, boundingRect().bottom() - _rewindButton->size().height() - 4);

    _pauseButton->setPos(_rewindButton->geometry().topRight());
    _playButton->setPos(_rewindButton->geometry().topRight());
    _fforwardButton->setPos(_playButton->geometry().topRight());

    /*
    QGraphicsLayoutItem* blank = new QGraphicsLayoutItem;
    blank->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QGraphicsLinearLayout* btnlayout = new QGraphicsLinearLayout(Qt::Horizontal);
	btnlayout->addItem(_rewindButton);
	btnlayout->addItem(_pauseButton);
	btnlayout->addItem(_playButton);
	btnlayout->addItem(_fforwardButton);

	QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(Qt::Vertical);
    layout->setMinimumSize(_appInfo->nativeSize());
	layout->setAlignment(btnlayout, Qt::AlignHCenter | Qt::AlignBottom);

    layout->addItem(blank);
	layout->addItem(btnlayout);

	setLayout(layout);
    */
}

void SN_SageStreamMplayer::rewindMplayer() {
	if (!_fsmMsgThread) return;
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("rewind")));
	_playButton->hide();
	_pauseButton->show();
}

void SN_SageStreamMplayer::pauseMplayer() {
    if (!_fsmMsgThread) return;

	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("pause")));
	_pauseButton->hide();
	_playButton->show();

    _isMplayerPaused = true;

    qDebug() << "SN_SageStreamMplayer::pauseMplayer() : Paused";
}

void SN_SageStreamMplayer::playMplayer() {
	if (!_fsmMsgThread) return;
//	_fsmMsgThread->sendSailMsg(OldSage::EVT_KEY, "play");
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("play")));
	_playButton->hide();
	_pauseButton->show();

    _isMplayerPaused = false;

    qDebug() << "SN_SageStreamMplayer::pauseMplayer() : Resume";
}

void SN_SageStreamMplayer::fforwardMplayer() {
	if (!_fsmMsgThread) return;
	QMetaObject::invokeMethod(_fsmMsgThread, "sendSailMsg", Qt::QueuedConnection, Q_ARG(int, OldSage::EVT_KEY), Q_ARG(QString, QString("forward")));
	_playButton->hide();
	_pauseButton->show();
}
