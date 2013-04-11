#include "applications/sn_phononwidget.h"


PhononWidget::PhononWidget(QString filename, const quint64 aid, const QSettings *s, BaseGraphicsWidget *parent, Qt::WindowFlags wFlags)
	: BaseGraphicsWidget(aid, s, parent, wFlags)
{
//	setAutoDelete(true);

	init();

	_vplayer->load(Phonon::MediaSource(filename));
	//loadVideo(mediaSource); // is this blocking ??

	setNativeSize(_vwidget->sizeHint());
	resize(_vwidget->sizeHint());
//	setPreferredSize(_vwidget->sizeHint());

//	appInfo = new AppInfo();
	appInfo->setFileInfo(filename);
	perfMon = new PerfMonitor();

	//_vplayer->play(); // blocking?
//	infoTextItem = new QGraphicsSimpleTextItem(this);
	initInfoTextItem();
}

PhononWidget::PhononWidget(const Phonon::MediaSource &/*mediaSource*/, const quint64 aid, const QSettings *s, BaseGraphicsWidget *parent, Qt::WindowFlags wFlags)
	: BaseGraphicsWidget(aid, s, parent, wFlags)
{
}

PhononWidget::~PhononWidget() {
	qDebug("PhononWidget::%s() : ", __FUNCTION__);

	/*_vplayer which is embedded is destroyed by ProxyWidget */
	if ( proxyWidget) delete proxyWidget;

	/* the objects below will be automatically destroyed by Qt */
	if (bufferingText) delete bufferingText;
}

//void PhononWidget::run() {
//	_vplayer->play();
//}

void PhononWidget::init() {
	bufferingText = 0;

	// widget must be a top-level widget whose parent is 0.
	_vplayer = new Phonon::VideoPlayer(Phonon::VideoCategory, 0);

	proxyWidget = new QGraphicsProxyWidget(this);
	proxyWidget->setWidget(_vplayer);

	_vwidget = _vplayer->videoWidget();

	// decoder finds out aspect ratio automatically (default)
	//_vwidget->setAspectRatio(Phonon::VideoWidget::AspectRatioAuto);
	_vwidget->setScaleMode(Phonon::VideoWidget::FitInView);

	_mobject = _vplayer->mediaObject();

	connect(_mobject, SIGNAL(bufferStatus(int)), this, SLOT(bufferingInfo(int)));
	connect(_mobject, SIGNAL(stateChanged(Phonon::State,Phonon::State)), this, SLOT(stateChanged(Phonon::State,Phonon::State)));
}


void PhononWidget::stateChanged(Phonon::State newstate, Phonon::State oldstate) {
	qDebug("PhononWidget::%s() : stated changed to %d", __FUNCTION__, newstate);
	if ( _vplayer && oldstate == Phonon::LoadingState &&
		 newstate == Phonon::StoppedState ) {
		qDebug("PhononWidget::%s() : playing video", __FUNCTION__);

		/** separate thread using QtConcurrent framework **/
//		QFuture<void> future = QtConcurrent::run(_vplayer, &Phonon::VideoPlayer::play);

		_vplayer->play();

//		QThreadPool *tp = QThreadPool::globalInstance();
//		tp->start(this);

	}

	if ( newstate == Phonon::ErrorState ) {
		qWarning() << "PhononWidget::" << __FUNCTION__ << "() : Error: " << _mobject->errorString();
	}
}

void PhononWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
//	painter->drawPixmap(boundingRect(), QPixmap::grabWidget(_vwidget), _vwidget->rect());

	if ( showInfo ) {
		Q_ASSERT(infoTextItem);
		painter->setBrush(Qt::black);
		painter->drawRect(infoTextItem->boundingRect());

		infoTextItem->show();
	}
	else {
		Q_ASSERT(infoTextItem);
		infoTextItem->hide();
	}
}

void PhononWidget::bufferingInfo(int percent) {
	qDebug("PhononWidget::%s() : buffering %d", __FUNCTION__, percent);
	if ( bufferingText ) {
		// update text
		bufferingText->setText(QString::number(percent).append(" %"));
	}
	else {
		// create object
		bufferingText = new QGraphicsSimpleTextItem(QString::number(percent).append(" %"), this);
	}

	// draw text
	update();
}

void PhononWidget::minimize() {
	_vplayer->pause();
	BaseGraphicsWidget::minimize();
}

void PhononWidget::restore() {
	BaseGraphicsWidget::restore();
	_vplayer->play();
}
