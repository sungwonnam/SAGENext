#include "sagenextscheduler.h"
#include "resourcemonitor.h"

#include "../applications/base/railawarewidget.h"
#include "../applications/base/appinfo.h"
#include "../applications/base/perfmonitor.h"
#include "../applications/base/affinityinfo.h"

#include <sys/time.h>
#include <sched.h>
#include <pthread.h>

SN_SchedulerControl::SN_SchedulerControl(SN_ResourceMonitor *rm, QObject *parent) :
	QObject(parent),
	gview(0),
	resourceMonitor(rm),
	_scheduleEnd(false),
	schedType(SN_SchedulerControl::SelfAdjusting),
	granularity(1000),
	_scheduler(0),
	_isRunning(false),
	controlPanel(0)
{
//	qDebug("%s::%s() : %d scheduler have started", metaObject()->className(), __FUNCTION__, schedList.size());
//	connect(resourceMonitor, SIGNAL(appRemoved(int)), this, SLOT(loadBalance()));
}

void SN_SchedulerControl::killScheduler() {
	if(!_scheduler) return;

	if (controlPanel) {
		QLayout *l = controlPanel->layout();
		if (l) {
			delete l;
		}
		controlPanel->close();
//		controlPanel->deleteLater();
		delete controlPanel;
	}
//	_scheduler->setEnd(true);


	_scheduler->quit(); // equivalent to calling QThread::exit(0), does nothing if there's no event loop for the thread



	if (_scheduler->isRunning())
		_scheduler->wait();
	delete _scheduler;
	_scheduler = 0;

	_isRunning = false;
}

int SN_SchedulerControl::launchScheduler(SN_SchedulerControl::Scheduler_Type st, int msec) {
	Q_ASSERT(resourceMonitor);

	granularity = msec;
	schedType = st;

	if (_scheduler && _scheduler->isRunning()) {
		qCritical() << "launchScheduler() : Scheduler is already running";
		return -1;
	}

	switch(st) {
	case SN_SchedulerControl::SMART : {
		_scheduler = new SMART_EventScheduler(resourceMonitor, msec);
		break;
	}
	case SN_SchedulerControl::SelfAdjusting : {

		SN_SelfAdjustingScheduler *sas  = new SN_SelfAdjustingScheduler(resourceMonitor, msec);
		_scheduler = sas;

		QLabel *interval = new QLabel("Frequency");
		QSlider *slider_freq = new QSlider(Qt::Vertical);
		slider_freq->setRange(0,1000); // 0 - 1000 msec
		slider_freq->setValue(_scheduler->frequency());
		connect(slider_freq, SIGNAL(valueChanged(int)), _scheduler, SLOT(applyNewInterval(int)));

		QVBoxLayout *vlf = new QVBoxLayout;
		vlf->addWidget(interval);
		vlf->addWidget(slider_freq);


		QLabel *lq = new QLabel("Sensitivity"); // quality threshold

		QSlider *slider_qth = new QSlider(Qt::Vertical);
		slider_qth->setRange(0, 100);
		slider_qth->setValue( 100 * sas->getQTH() );
		connect(slider_qth, SIGNAL(valueChanged(int)), sas, SLOT(applyNewThreshold(int)));

		QVBoxLayout *vlq = new QVBoxLayout;
		vlq->addWidget(lq);
		vlq->addWidget(slider_qth);



		QLabel *ld = new QLabel("Aggresion"); // decreasing factor

		QSlider *slider_dec = new QSlider(Qt::Vertical);
		slider_dec->setRange(0, 100);
		slider_dec->setValue(-100 * sas->getDecF());
		connect(slider_dec, SIGNAL(valueChanged(int)), sas, SLOT(applyNewDecF(int)));

		QVBoxLayout *vld = new QVBoxLayout;
		vld->addWidget(ld);
		vld->addWidget(slider_dec);



		QLabel *li = new QLabel("Greediness"); // increasing factor

		QSlider *slider_inc = new QSlider(Qt::Vertical);
		slider_inc->setRange(0, 100);
		slider_inc->setValue(100 * sas->getIncF());
		connect(slider_inc, SIGNAL(valueChanged(int)), sas, SLOT(applyNewIncF(int)));

		QVBoxLayout *vli = new QVBoxLayout;
		vli->addWidget(li);
		vli->addWidget(slider_inc);


		// root layout of controlPanel
		QHBoxLayout *hl = new QHBoxLayout;
		hl->addLayout(vlf);
		hl->addLayout(vlq);
		hl->addLayout(vld);
		hl->addLayout(vli);

		if (!controlPanel) controlPanel = new QFrame;
		controlPanel->setLayout(hl);
		controlPanel->adjustSize();
//		controlPanel->show();

		break;
	}

	case SN_SchedulerControl::DividerWidget : {
		_scheduler = new DividerWidgetScheduler(resourceMonitor, msec);
		break;
	}
	case SN_SchedulerControl::DelayDistribution : {
		_scheduler = new DelayDistributionScheduler(resourceMonitor, msec);
		break;
	}
	default :
		break;
	}

	_scheduler->start();
	_isRunning = true;

	return 0;
}

int SN_SchedulerControl::launchScheduler(const QString &str, int msec) {
	Scheduler_Type st = SN_SchedulerControl::SelfAdjusting;
	if ( QString::compare(str, "SMART", Qt::CaseInsensitive) == 0 ) {
		st = SN_SchedulerControl::SMART;
	}
	else if (QString::compare(str, "DividerWidget", Qt::CaseInsensitive) == 0) {
		st = SN_SchedulerControl::DividerWidget;
	}
	else if (QString::compare(str, "SelfAdjusting", Qt::CaseInsensitive) == 0) {
		st = SN_SchedulerControl::SelfAdjusting;
	}
	else if (QString::compare(str, "DelayDistribution", Qt::CaseInsensitive) == 0) {
		st = SN_SchedulerControl::DelayDistribution;
	}

	return launchScheduler(st, msec);
}

int SN_SchedulerControl::launchScheduler() {
	return launchScheduler(schedType, granularity);
}

SN_SchedulerControl::~SN_SchedulerControl() {

	setScheduleEnd(true);
//	if ( futureWatcher.isRunning() ) {
//		futureWatcher.waitForFinished();
//	}

	/** always destroy scheduler before resource monitor */
//	for (int i=0; i<schedList.size(); ++i) {
//		schedList.at(i)->setEnd(true);
//		schedList.at(i)->wait();
//	}
	killScheduler();
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

bool SN_SchedulerControl::eventFilter(QObject *, QEvent *) {

//	if ( e->type() == QEvent::Timer ) {

//	}
//	else {
//		qDebug("%s::%s() : object %s(%x), event type %d", this->metaObject()->className(), __FUNCTION__, obj->metaObject()->className(), obj, e->type());
//	}

	// QGraphicsItem::update() is actually QEvent::MetaCall type which invokes QGraphicsScene::update()
	// Also, note that QGraphicsScene::update() will invoke updateRequest event (QGraphicsView's viewport widget as a receiver)
//	if ( strcmp(obj->metaObject()->className(), "SageStreamWidget") == 0  &&  e->type() == QEvent::MetaCall) {

//		RailawareWidget *item = static_cast<RailawareWidget *>(obj);
////		qDebug() << "SIBAL " << item->data(1976);

//		PerfMonitor *pm = item->perfMon();
//		Q_ASSERT(pm);
////		pm->updateDispFps();
////		pm->updateEQDelay();
////		qDebug() << pm->getCurrEqDelay() << pm->getAvgEqDelay();
////		qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "dispatching update()";

////		qDebug() << QTime::currentTime().toString("mm:ss.zzz") << "event dispatched";
//	}

//	if ( e->type() == QEvent::User ) {
//		UpdateEvent *ue = static_cast<UpdateEvent *>(e);
//	 QWidget::repaint(QRect);  // QGraphicsView's viewport is QGLWidget
//	QMetaObject::invokeMethod(ue->widgetPtr()->scene(), "")

//	}

	return false; // event will NOT be handled here
}














/**************
  Abstract
  *************/
SN_AbstractScheduler::~SN_AbstractScheduler() {
	qDebug() << "Scheduler has been deleted";
}

void SN_AbstractScheduler::applyNewInterval(int i) {

	timer.setInterval(i);
}

void SN_AbstractScheduler::run() {

//	QTimer timer;
	timer.setInterval(_granularity);

	connect(&timer, SIGNAL(timeout()), this, SLOT(doSchedule()));

	timer.start();
	qDebug("%s::%s() : Staring scheduler (QEventLoop) !", metaObject()->className(), __FUNCTION__);


	// Starts independent event loop of this thread. Enter eventloop and waits until exit() is called.
	// any function in the scheduler must be called by signal-slot queued connection
	// scheduler calls any function live in GUI eventloop (qApp::exec()) by slots
	exec();
}

int SN_AbstractScheduler::configureRail(SN_RailawareWidget *rw, SN_ProcessorNode *pn) {
	if (!rw || !rw->affInfo()) return -1;
	if (!pn) return -1;

	// this will clear ready bit first then set one, followed by set flag
	// because of the flag, SagePixelReceiver::run() will end up calling AffinityInfo::applyNewParameters()

	//        updateAffInfo(rw, -1, pn->getID());

	/* Add widget to the processor */
	pn->addApp(rw);

	/* this will trigger AffinityInfo::applyNewParameters() from SagePixelReceiver */
	/* which will trigger ResourceMonitor::updateAffInfo() from AffinityInfo */
	rw->affInfo()->setReadyBit(AffinityInfo::CPU_MASK, pn->getID());

	return pn->getID();

	/* set thread affinity for SageBlockStreamer */
	/*
	QByteArray ba(AffinityInfo::Num_Cpus, '0');
	ba[pn->getID()] = '1';
	rw->affInfo()->setSageStreamerAffinity(ba.constData());
	*/
}

int SN_AbstractScheduler::configureRail(SN_RailawareWidget *rw) {
	return 0;
}

int SN_AbstractScheduler::configureRail() {

	int GlobalMax = 0; // max aggregated priority value among processors
	int MaxOnProc[AffinityInfo::Num_Cpus];

	// set initial values
	for (int i=0; i<AffinityInfo::Num_Cpus; i++) {
		SN_ProcessorNode *p = rMonitor->getProcVec()->at(i);
		MaxOnProc[ p->getID() ] = p->prioritySum();

		if ( MaxOnProc[p->getID()] > GlobalMax ) {
			GlobalMax = MaxOnProc[p->getID()];
		}
	}

	// run main on node 0
	// move high priority apps to cpus on node 0 (be careful about SMT cores)
	// move the highest app to the core right next to the core where main() runs ??

	// rail is static and it's determined by hardware architecture.
	// thread affinity means running a network thread on where interrupt processing is occurring. (/proc/interrupts)

	/* find out which core is processing interrupts for eth4 */
	QProcess proc;
	proc.start("cat /proc/interrupts | grep eth4");
	// cat /proc/irq/####/smp_affinity // requires sudo permission at least

	return 0;
}













/**********************
  SMART Event scheduler
  ******************/
SMART_EventScheduler::SMART_EventScheduler(SN_ResourceMonitor *r, int granularity, QObject *parent) :
	SN_AbstractScheduler(r, granularity, parent)
{
//	proc = p;
	workingSet.clear();
	QThread::setTerminationEnabled(true);
}


int SMART_EventScheduler::insertIntoWorkingSet(SN_RailawareWidget *newtask, qreal now) {
	if (!newtask) return -1;

	qreal new_task_finish_estimate = now + newtask->perfMon()->getCurrRecvLatency() + newtask->perfMon()->getAvgConvDelay();
	qreal new_task_deadline = newtask->perfMon()->ts_nextframe();

	if (workingSet.empty()  ||  new_task_finish_estimate == now) {
		workingSet.push_back(newtask);
		return workingSet.size();
	}

	QList<SN_RailawareWidget *>::iterator it = workingSet.begin();
	SN_RailawareWidget *higher_task = 0;


	/* see if adding w into working set hurts higher priority jobs in the set  */
	for (; it!=workingSet.end(); it++) {
		higher_task = *it;
		Q_ASSERT(higher_task);

		if ( higher_task->priority() > newtask->priority() ) {

			qreal higher_task_deadline = higher_task->perfMon()->ts_nextframe();
			qreal higher_task_avglatency = higher_task->perfMon()->getCurrRecvLatency() + higher_task->perfMon()->getAvgConvDelay();
//			qreal new_task_finish_estimate = _t + newtask->perfMon()->getAvgRecvLatency();

			if ( 0 < higher_task_deadline && higher_task_deadline <= now ) {
				newtask->failToSchedule++;
				return -1;
//				continue;
			}

			if ( new_task_deadline < higher_task_deadline ) {
				if ( new_task_finish_estimate + higher_task_avglatency > higher_task_deadline ) {
					// can't schedule newtask

//					qDebug("newtask %llu can't be scheduled", newtask->globalAppId());

					newtask->failToSchedule++;

					return -1;
				}
			}
		}
		/* when tasks have all same priority, then it's EDF scheduling */
	}

	/* add newtask in workingset earliest deadline order */

	newtask->failToSchedule = 0; // reset

	SN_RailawareWidget *w = 0;
	for (it=workingSet.begin(); it!=workingSet.end(); it++) {
		w = *it;
		if (w->perfMon()->ts_nextframe() > newtask->perfMon()->ts_nextframe() ) {
			workingSet.insert(it, newtask);
			return workingSet.size();
		}
	}
	workingSet.push_back(newtask);
	return workingSet.size();
}

void SMART_EventScheduler::doSchedule() {

	if(!rMonitor || rMonitor->getWidgetList().isEmpty()) return;

	struct timeval now;
	gettimeofday(&now, 0);
	qreal nowsec = (qreal)(now.tv_sec) + 0.000001 * (qreal)(now.tv_usec);

	/*!
	  populate workingSet
	  */
	foreach (SN_RailawareWidget *rw, rMonitor->getWidgetList()) {
		//		RailawareWidget *rw = 0;
		//		for (; it != itend; it++ ) {
		//			rw = *it;
		if (!rw) continue;

		//qDebug("%s::%s() : At processor %d, app %llu", metaObject()->className(), __FUNCTION__);

		/* applying best effort algorithm on the candidate set */
		insertIntoWorkingSet(rw, nowsec);
	}
	if (workingSet.empty()) return;

//	struct timeval n;
//	gettimeofday(&n, 0);
//	qreal nn = (qreal)(n.tv_sec) + 0.000001 * (qreal)(n.tv_usec);
//	qDebug("workingset overhead %.4f msec", 1000 * (nn-nowsec));



//	foreach(RailawareWidget *r, workingSet) {
//		qDebug("%llu, %.9f", r->globalAppId(), r->perfMon()->ts_nextframe());
//	}

	// remember, workingSet is orderd by deadline
#if QT_VERSION >= 0x040700
	currMsecSinceEpoch = QDateTime::currentMSecsSinceEpoch();
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    currMsecSinceEpoch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#endif

	SN_RailawareWidget *rw = 0;

//	QList<RailawareWidget *>::iterator it;
//	for (it=workingSet.begin(); it!=workingSet.end(); it++) {
//	for (int i=0; i<workingSet.size(); ++i) {
	foreach (rw, workingSet) {
//		rw = *it;
//	while(!workingSet.isEmpty()) {
//		rw = workingSet.at(i);
//		rw = workingSet.front();

		if (!rw) continue;

		if (rw->isScheduled()) continue;

		// frame must be displayed at this time
		qreal frame_deadline = rw->perfMon()->ts_nextframe();

		// recv must be scheduled at this time
		qreal sched_deadline = frame_deadline - rw->perfMon()->getCurrRecvLatency() + rw->perfMon()->getAvgConvDelay();

		if ( frame_deadline <= 0 /*not initialized yet*/ ||  sched_deadline <= nowsec /*already too late*/ )
		{
//			printf("sched invoke recv");
			/*
			if ( ! QMetaObject::invokeMethod(rw, "scheduleReceive", Qt::QueuedConnection) ) {
				qCritical("%s::%s() : Failed to invoke scheduleUpdate on widget %llu", metaObject()->className(), __FUNCTION__, rw->globalAppId());
			}
			else {
				rw->setScheduled(true);
			}
			*/
			rw->setScheduled(true);
			rw->scheduleReceive();
		}
		else {
//			QTimer::singleShot( (frame_deadline - nowsec) * 1000, rw, SLOT(scheduleUpdate()) );
//			rw->setScheduled(true);
//			QTimer::singleShot( (sched_deadline - nowsec) * 1000, rw, SLOT(scheduleReceive()) );
		}


//		if ( /*rw->perfMon()->ts_nextframe() > 0  && */ rw->perfMon()->ts_nextframe() <= nowsec ) {
//			//				rw = workingSet.takeFirst();
//			//				qDebug("%s::%s() : Scheduling widget %llu on processor %d. (%.3f <= %.3f)", metaObject()->className(), __FUNCTION__, rw->globalAppId(), pid, rw->perfMon()->ts_nextframe(), nowsec);


//			// a QEvent will be sent and the member is invoked as soon as the application enters the main event loop with QueuedConnection
//			if ( ! QMetaObject::invokeMethod(rw, "scheduleUpdate", Qt::QueuedConnection) ) {
//				qCritical("%s::%s() : Failed to invoke scheduleUpdate on widget %llu", metaObject()->className(), __FUNCTION__, rw->globalAppId());
//			}
//			//				qApp->processEvents(); // process all pending event for the CALLING thread. Calling this function processes events only for the calling thread

//			else {
//				// reset ts_nextframe to prevent duplicate scheduleupdate()
////				rw->perfMon()->set_ts_nextframe(-1);
//				rw->perfMon()->set_ts_currframe(nowsec);
//				workingSet.pop_front();
////				workingSet.removeOne(rw);
////				workingSet.removeAt(i);
////				--i;
////				if (workingSet.isEmpty()) return;
//			}
//		}
	}
	workingSet.clear();


	//		QThread::usleep(_granularity);
//	QThread::msleep(2);

	/* below is needed when this function isn't a separate thread */
	//		qApp->flush(); // platform specific event (X events)
	//		qApp->sendPostedEvents(); // empties the event queue
	//		qApp->processEvents(); // for calling thread
}







DelayDistributionScheduler::DelayDistributionScheduler(SN_ResourceMonitor *r, int granularity, QObject *parent) :
	SN_AbstractScheduler(r, granularity, parent)
{
}

void DelayDistributionScheduler::doSchedule() {
	if (rMonitor->getWidgetList().isEmpty()) return;

#if QT_VERSION >= 0x040700
	currMsecSinceEpoch = QDateTime::currentMSecsSinceEpoch();
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    currMsecSinceEpoch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#endif
	QList<SN_RailawareWidget *> wlist = rMonitor->getWidgetList(); // snapshot of current list

	qreal SumPriority = 0; qreal SumAdjDevi = 0;
	qreal SumResNeeded = 0;
	qreal system_wide_curr_bandwidth = 0;
	foreach (SN_RailawareWidget *r, wlist) {
		SumPriority += r->priority(currMsecSinceEpoch);

		// framerate deviation from adjusted FPS
		SumAdjDevi += r->perfMon()->getCurrAdjDeviation();

		SumResNeeded += (r->perfMon()->getCurrAdjDeviation() * r->appInfo()->frameSizeInByte() * 8); // bps
//		system_wide_curr_bandwidth += r->perfMon()->getCurrBandwidthMbps();

//		r->setQuality( r->observedQuality() );
	}
	qreal AvgAdjDevi = SumAdjDevi / wlist.size();

//	if (SumResNeeded > 100 * 1000000) {
//		// needed more than 100 Mbps
//	}

	qDebug() << SumResNeeded / 1000000 << "Mbps more needed";
	foreach (SN_RailawareWidget *r, wlist) {
		qreal p = r->priority(currMsecSinceEpoch) / SumPriority;
		int intp = p * 100;
		qDebug() << r->globalAppId() << p << intp << "%. I need " << ((r->perfMon()->getExpetctedFps() * 8 * r->appInfo()->frameSizeInByte()) / 1000000) - r->perfMon()->getCurrBandwidthMbps() << "Mbps more";

		qreal needed = ((r->perfMon()->getExpetctedFps() * 8 * r->appInfo()->frameSizeInByte()) / 1000000) - r->perfMon()->getCurrBandwidthMbps(); // Mbps more needed to fullfill expected FPS
		qreal allowed = p * needed;
	}
	qDebug() << "\n\n";

	/*
	if (AvgAdjDevi > 5) {
		// let's do something

		foreach (RailawareWidget *r, wlist) {
			// calculate current priority distribution
			qreal weight = r->priority(currMsecSinceEpoch) / SumPriority;
			qreal inverse_weight = 1 / weight;

			// I should give this much to others
			0.01 * inverse_weight * SumResNeeded; // bps

			// I can have this much bps
			qreal allowedBPS = weight * system_wide_curr_bandwidth; // bps

//			qreal expectedBPS = r->perfMon()->getExpetctedFps() * r->appInfo()->getFrameBytecount() * 8;

			qreal adjustedFPS = allowedBPS / (8 * r->appInfo()->getFrameBytecount());

			r->perfMon()->setAdjustedFps(adjustedFPS);
		}
	}
	*/
}







SN_SelfAdjustingScheduler::SN_SelfAdjustingScheduler(SN_ResourceMonitor *r, int granularity, QObject *parent) :
	SN_AbstractScheduler(r, granularity, parent),

	qualityThreshold(0.96),
	decreasingFactor(-0.1),
	increasingFactor(0.2),

	phaseToken(-1)
{
//	qDebug() << "Self Adjusting Scheduler ";
}

void SN_SelfAdjustingScheduler::applyNewThreshold(int i) {
	rwlock.lockForWrite();
	if ( i <= 0 ) {
		qualityThreshold = 0.001;
	}
	else {
				qualityThreshold = (qreal)i / 100.0; // note that slider range 0 - 100
	}
	rwlock.unlock();
}

void SN_SelfAdjustingScheduler::applyNewDecF(int i) {
	rwlock.lockForWrite();
	if ( i <= 0 ) {
		decreasingFactor = -0.001;
	}
	else {
		decreasingFactor = -1 * (qreal)i / 100.0;
	}
	rwlock.unlock();
}

void SN_SelfAdjustingScheduler::applyNewIncF(int i) {
	rwlock.lockForWrite();
	if ( i <= 0 ) {
		increasingFactor = 0.001;
	}
	else {
		increasingFactor = (qreal)i / 100.0;
	}
	rwlock.unlock();
}

void SN_SelfAdjustingScheduler::doSchedule() {
#if QT_VERSION >= 0x040700
	currMsecSinceEpoch = QDateTime::currentMSecsSinceEpoch();
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    currMsecSinceEpoch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#endif
	QList<SN_RailawareWidget *> wlist = rMonitor->getWidgetList();


	// items are always sorted by key when iterating over QMap
	QMap<qreal, SN_RailawareWidget *> wmap;
	foreach (SN_RailawareWidget *rw, wlist) {
		if (!rw || !rw->perfMon()) continue;
		wmap.insertMulti(rw->priority(currMsecSinceEpoch), rw);
	}
	QMapIterator<qreal, SN_RailawareWidget *> it_secondary(wmap);
	QMapIterator<qreal, SN_RailawareWidget *> it_primary(wmap); // iterate high priority first
        it_primary.toBack();


	int priorityBias = 100; // to quantize
	qreal priorityRank = 1.0; // greediness and aggression will bigger for higher priority widgets

	rwlock.lockForRead();


	while ( it_primary.hasPrevious() ) {
		it_primary.previous();
		SN_RailawareWidget *rw = it_primary.value();

//		qDebug() << rw->priority(currMsecSinceEpoch);

		// sorted by priority descendant (high priority first)
		if (!rw || !rw->perfMon()) continue;

		int rwPriority = priorityBias * rw->priority(currMsecSinceEpoch);
		rw->setPriorityQuantized(rwPriority);

		//
		  //priority offset based on ROI can be applied here
		  //

		qreal rwQuality = rw->observedQuality(); // current observed quality based on EXPECTED quality.
//		PerfMonitor *rwPm = rw->perfMon();
//		qreal myUnitValue = rwPriority / (rw->appInfo()->getFrameBytecount() * rw->perfMon()->getExpetctedFps()); // priority per unit byte/sec

		qreal rwQualityAdjusted = rw->observedQualityAdjusted(); // current observed quality based on ADJUSTED quality

		if (priorityRank == 1.0) {
			// the highest priority widget
			rw->setQuality(1.0);
		}

//		bool problem = false;

//		if ( rwPriority <= 0 ) {
//			rw->setQuality(0.01);
//			continue;
//		}

//		qDebug() << rw->globalAppId() << rwPriority << rwQuality << rwQualityAdjusted;

		// Compare with other widget in the list
		// sorted by priority ascendant
		it_secondary.toFront(); // Always rewind for each rw
		while ( it_secondary.hasNext() ) {
			it_secondary.next();
			SN_RailawareWidget *other = it_secondary.value();

			if (!other || !other->perfMon()) continue;
			if (rw == other) continue;

			int otherPriority = priorityBias * other->priority(currMsecSinceEpoch);
			qreal otherQuality = other->observedQuality();
//			qreal otherQualityAdjusted = other->observedQualityAdjusted();

//			qDebug() << "\t" << other->globalAppId() << otherPriority << otherQuality << otherQualityAdjusted;

			// I'm doing OK as expected
			if ( rwQualityAdjusted >= qualityThreshold ) {

				if (rwPriority > otherPriority) {
					// my quality is higher
					if (rwQuality >= otherQuality) {
						// everything's ok
					}
					else {
						// I'm doing as OK when compared to lower widget
						// But my quality is lower than lower priority apps
						// So my adjusted quality should be higher than lower priority apps
//						qDebug() << "\t\t" << rw->globalAppId() << rwPriority << rwQuality << " and " << other->globalAppId() << otherPriority << otherQuality;

						// RailawareWidget::adjustQuality() will return
						// 1, if adjustedFps > expectedFps
						// -1, if adjustedFps < 1
						// 0, otherwise
						int ret = rw->adjustQuality( increasingFactor / priorityRank ); // Greedy for correctness
						if (ret == 1) {
							// rw can't increase more
							other->adjustQuality( decreasingFactor );
						}
					}
				}
				else if (rwPriority == otherPriority) {
					if (otherQuality > rwQuality) {
						// Has to achieve proportional sharing among equal priority apps
						rw->adjustQuality( increasingFactor ); // Greedy for Fairness
					}
				}
				else {
					// I'm doing ok but my quality is higher than higher priority widget
					if (rwQuality >= otherQuality) {
//						other->adjustQuality( increasingFactor );
						rw->adjustQuality( decreasingFactor ); // Yielding for correctness. I should slow down for higher priority widget
					}
					else {
						// Everything is ok between rw and other
					}
				}
			}


			// I'm not acheiving adjusted quality
			else {
				if (rwPriority > otherPriority) {
					// Attack lower priority widget
					other->adjustQuality( decreasingFactor ); // Aggression for higher priority app's QoS
				}

				// Fairness among same priority apps
				else if (rwPriority == otherPriority) {
					if (rwQuality >= otherQuality) {
						// I'm not doing ok but my quality is higher than other apps that has the same priority, so my adjusted quality should be lower.
						rw->adjustQuality(decreasingFactor);
					}
					else {
						// I'm not doing ok and other same priority app is showing better quality, so they should lower the quality for fairness
						other->adjustQuality(decreasingFactor);
					}
				}
				else {
					// can't do much in this situation
				}
			}
		}

//		if (!problem) {
//			rw->adjustQuality(0.1);
//		}
		priorityRank += 0.2; // 1.0 for the highest priority ( higher the priority lower the rank)
	}
	rwlock.unlock();





	phaseToken = 0; // Below section is not used  !!!

//	QList<RailawareWidget *> wlist = rMonitor->getWidgetList();

	if ( phaseToken < 0 ) {
		foreach (SN_RailawareWidget *rw, wlist) {
			if (!rw || !rw->perfMon()) continue;

			qreal rwPriority = rw->priority(currMsecSinceEpoch);
			qreal rwQuality = rw->observedQuality();
	//		PerfMonitor *rwPm = rw->perfMon();
	//		qreal myUnitValue = rwPriority / (rw->appInfo()->getFrameBytecount() * rw->perfMon()->getExpetctedFps()); // priority per unit byte/sec

			qreal temp = 1000;

			// compare with other widget in the list
			foreach (SN_RailawareWidget *higher, wlist) {
				if (!higher || !higher->perfMon()) continue;
				if (rw == higher) continue;

				qreal otherPriority = higher->priority(currMsecSinceEpoch);
				qreal otherQuality = higher->observedQuality();

				if ( rwPriority < otherPriority  &&  rwQuality > otherQuality) {
					if (otherQuality < temp) temp = otherQuality;
				}
			}

			if (temp < 1000) {
				// there's higher priority widget that shows lower quality than me

				if ( rw->adjustQuality(-0.1) == -1 ) {
					// I can't lower more
					--phaseToken;
				}
			}
			else {
				// everything's ok
				++phaseToken; // greedy
				rw->adjustQuality(0.05);
			}
		}
	}

	else if (phaseToken > 0) {
		foreach (SN_RailawareWidget *rw, wlist) {
			if (!rw || !rw->perfMon()) continue;

			qreal rwPriority = rw->priority(currMsecSinceEpoch);
			qreal rwQuality = rw->observedQuality();
	//		PerfMonitor *rwPm = rw->perfMon();
	//		qreal myUnitValue = rwPriority / (rw->appInfo()->getFrameBytecount() * rw->perfMon()->getExpetctedFps()); // priority per unit byte/sec


			qreal temp = -1;

			// compare with other widget in the list
			foreach (SN_RailawareWidget *lower, wlist) {
				if (!lower || !lower->perfMon()) continue;
				if (rw == lower) continue;

				qreal otherPriority = lower->priority(currMsecSinceEpoch);
				qreal otherQuality = lower->observedQuality();

				if ( rwPriority > otherPriority ) {
					if (rwQuality < otherQuality) {
						if (otherQuality > temp) temp = otherQuality;
					}
				}
			}

			if (temp>0) {
				// there's a lower priority widget that show higher quality than rw
				if ( rw->setQuality( temp + 0.05 ) == 1 ) {
					// I can't increase more !
					--phaseToken;
				}
			}
			else {
				// everything's ok
				++phaseToken;
				rw->adjustQuality(0.05);
			}
		}
	}

	else {
//		++phaseToken; // greedy
	}
}
/*!

  execute second loop differently
  */
/****
void SelfAdjustingScheduler::doSchedule() {
	currMsecSinceEpoch = QDateTime::currentMSecsSinceEpoch();
	QList<RailawareWidget *> wlist = rMonitor->getWidgetList();


	// items are always sorted by key when iterating over QMap
	QMap<qreal, RailawareWidget *> wmap;
	foreach (RailawareWidget *rw, wlist) {
		if (!rw || !rw->perfMon()) continue;
		wmap.insertMulti(rw->priority(currMsecSinceEpoch), rw);
	}
	QMapIterator<qreal, RailawareWidget *> it_secondary(wmap);
	QMapIterator<qreal, RailawareWidget *> it_primary(wmap);
	it_primary.toBack();// iterate high priority first


	int priorityBias = 100; // to quantize
	int priorityRank = 0; // greediness and aggression will bigger for higher priority widgets

	rwlock.lockForRead();


	while ( it_primary.hasPrevious() ) {
		it_primary.previous();
		RailawareWidget *rw = it_primary.value();
		priorityRank++; // 1 for the highest priority

//		qDebug() << rw->priority(currMsecSinceEpoch);

		// sorted by priority descendant (high priority first)
		if (!rw || !rw->perfMon()) continue;

		int rwPriority = priorityBias * rw->priority(currMsecSinceEpoch);
		rw->setPriorityQuantized(rwPriority);


		qreal rwQuality = rw->observedQuality(); // current observed quality based on EXPECTED quality.
//		PerfMonitor *rwPm = rw->perfMon();
//		qreal myUnitValue = rwPriority / (rw->appInfo()->getFrameBytecount() * rw->perfMon()->getExpetctedFps()); // priority per unit byte/sec

		qreal rwQualityAdjusted = rw->observedQualityAdjusted(); // current observed quality based on ADJUSTED quality


		// I'm doing OK as expected.
		// Let's look higher doing ok as well
		if ( rwQualityAdjusted >= qualityThreshold ) {
			it_secondary.toBack(); // Always rewind for each rw. And look for higher priority widgets
			while ( it_secondary.hasPrevious() ) {
				it_secondary.previous();
				RailawareWidget *other = it_secondary.value();
				if (rw == other) continue;
				if (!other || !other->perfMon()) continue;

				int otherPriority = priorityBias * other->priority(currMsecSinceEpoch);
				qreal otherQuality = other->observedQuality();

				if (rwPriority > otherPriority) {
					break; // I don't need to look lower priority widgets
				}
			}
		}

		// I'm not doing OK
		// Let's kill some lower widgets
		else {
			it_secondary.toFront(); // Always rewind for each rw. And look for lower priority widgets
			while ( it_secondary.hasNext() ) {
				it_secondary.next();
				RailawareWidget *other = it_secondary.value();
				if (rw == other) continue;
				if (!other || !other->perfMon()) continue;

				int otherPriority = priorityBias * other->priority(currMsecSinceEpoch);
				qreal otherQuality = other->observedQuality();

				if (rwPriority < otherPriority) {
					break; // I shouldn't look higher priority widgets
				}
			}
		}


//		qDebug() << rw->globalAppId() << rwPriority << rwQuality << rwQualityAdjusted;

		// Compare with other widget in the list
		// sorted by priority ascendant
		it_secondary.toFront(); // Always rewind for each rw
		while ( it_secondary.hasNext() ) {
			it_secondary.next();
			RailawareWidget *other = it_secondary.value();

			if (!other || !other->perfMon()) continue;
			if (rw == other) continue;

			int otherPriority = priorityBias * other->priority(currMsecSinceEpoch);
			qreal otherQuality = other->observedQuality();
			qreal otherQualityAdjusted = other->observedQualityAdjusted();

//			qDebug() << "\t" << other->globalAppId() << otherPriority << otherQuality << otherQualityAdjusted;

			// I'm doing OK as expected
			if ( rwQualityAdjusted >= qualityThreshold ) {

				if (rwPriority > otherPriority) {
					// my quality is higher
					if (rwQuality >= otherQuality) {
						// everything's ok
					}
					else {
						// I'm doing as OK based on adjusted quality
						// But my quality is lower than lower priority apps
						// So my adjusted quality should be higher than lower priority apps
//						qDebug() << "\t\t" << rw->globalAppId() << rwPriority << rwQuality << " and " << other->globalAppId() << otherPriority << otherQuality;
						rw->adjustQuality( increasingFactor ); // Greedy for correctness
					}
				}
				else if (rwPriority == otherPriority) {
					if (otherQuality > rwQuality) {
						// Has to achieve proportional sharing among equal priority apps
						rw->adjustQuality(increasingFactor); // Greedy for Fairness
					}
				}
				else {
					// I'm doing ok but my quality is higher than higher priority widget
					if (rwQuality >= otherQuality) {
//						other->adjustQuality( increasingFactor );
						rw->adjustQuality(decreasingFactor); // Yielding for correctness. I should slow down for higher priority widget
					}
					else {
						// Everything is ok between rw and other
					}
				}
			}


			// I'm not acheiving adjusted quality
			else {
				if (rwPriority > otherPriority) {
					// Attack lower priority widget
					other->adjustQuality( decreasingFactor ); // Aggression for higher priority app's QoS
				}

				// Fairness among same priority apps
				else if (rwPriority == otherPriority) {
					if (rwQuality >= otherQuality) {
						// I'm not doing ok but my quality is higher than other apps that has the same priority, so my adjusted quality should be lower.
						rw->adjustQuality(decreasingFactor);
					}
					else {
						// I'm not doing ok and other same priority app is showing better quality, so they should lower the quality for fairness
						other->adjustQuality(decreasingFactor);
					}
				}
				else {
					// can't do much in this situation
				}

			}


		}

//		if (!problem) {
//			rw->adjustQuality(0.1);
//		}
	}

	rwlock.unlock();
}

***/








DividerWidgetScheduler::DividerWidgetScheduler(SN_ResourceMonitor *r, int granularity, QObject *parent) :
	SN_AbstractScheduler(r, granularity, parent),
	_Q_highest(1.0),
	_P_fiducial(0.0),
	_Q_fiducial(1.0),
	totalRequiredResource(0),
	totalRedundantResource(0),
	fiducialWidget(0)

{
#if QT_VERSION >= 0x040700
	currMsecSinceEpoch = QDateTime::currentMSecsSinceEpoch();
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    currMsecSinceEpoch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#endif

	qDebug("\nStarting SelfAdjustingScheduler\n");
}

void DividerWidgetScheduler::setQHighest(qreal qh) {
	rwlock.lockForWrite();
	_Q_highest = qh;
	rwlock.unlock();
}
void DividerWidgetScheduler::setQAnchor(qreal qa) {
	rwlock.lockForWrite();
	_Q_fiducial = qa;
	rwlock.unlock();
}
void DividerWidgetScheduler::setPAnchor(qreal pa) {
	rwlock.lockForWrite();
	_P_fiducial = pa;

	if (rMonitor) {
		qreal temp = 100000.0;
		foreach (SN_RailawareWidget *rw, rMonitor->getWidgetList()) {
			if ( rw->observedQuality() <= 0) continue;

			qreal priority = rw->priority(currMsecSinceEpoch); // TODO : priority() must be light !

			// find the closest priority
			qreal diff = fabs( priority - _P_fiducial );
			if ( diff < temp ) {
				fiducialWidget = rw;
				temp = diff;
			}
		}
	}

	rwlock.unlock();
}

void DividerWidgetScheduler::doSchedule() {
	/* _P_anchor is real readout
	  _Q_highest is scaled value
	  */
	if (!rMonitor) return;

	QList<SN_RailawareWidget *> widgetList = rMonitor->getWidgetList(); // copy of current widget list
	if (widgetList.isEmpty()) return;

#if QT_VERSION >= 0x040700
	currMsecSinceEpoch = QDateTime::currentMSecsSinceEpoch();
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    currMsecSinceEpoch = tv.tv_sec * 1000  +  tv.tv_usec * 0.0001;
#endif

	// reset
	totalRequiredResource = 0.0;
	totalRedundantResource = 0.0;

	// To find current the lowest/highest priority
	qreal lowestp = 100, highestp = -100;
	foreach (SN_RailawareWidget *rw, widgetList) {
		qreal priority = rw->priority(currMsecSinceEpoch); // TODO : priority() must be light !

		if ( priority < lowestp ) lowestp = priority;
		if ( priority > highestp ) highestp = priority;
	}

	// To find the anchor widget
//	RailawareWidget *anchorWidget = 0; qreal temp = 100000.0;

	if ( ! fiducialWidget ) return;


	rwlock.lockForRead();

	QMap<qreal, SN_RailawareWidget *> redunMap;


//	qreal anchorWidgetQuality = anchorWidget->perfMon()->getCurrRecvFps() / anchorWidget->perfMon()->getExpetctedFps();
	qreal fiducialWidgetQuality = fiducialWidget->observedQuality();
	qreal fiducialWidgetPriority = fiducialWidget->priority(currMsecSinceEpoch);

	qDebug() << "\n\nThe fiducial widget id" << fiducialWidget->globalAppId() << "priority" << fiducialWidgetPriority;


	/***
	if ( _Q_fiducial > fiducialWidgetQuality ) {
		fiducialWidget->perfMon()->setQuality(_Q_fiducial);
		qDebug() << "fiducialWidget's quality is set to" << _Q_fiducial << " adjusted FPS" << fiducialWidget->perfMon()->getAdjustedFps();
	}
	else {
		// current observed fps is higher than user set fps
		qreal redundantFps = fiducialWidget->perfMon()->getCurrRecvFps() - _Q_fiducial * fiducialWidget->perfMon()->getExpetctedFps();

		// resource (in bps) is redundant
		// be aware, this calculation is inaccurate
		qreal redun = (redundantFps * 8 * fiducialWidget->appInfo()->getFrameBytecount());
		totalRedundantResource += redun;
		redunMap.insert(fiducialWidgetPriority, fiducialWidget);

		qDebug() << "fiducialWidget's quality is currently better. redundant resource is now" << 0.000001 * totalRedundantResource << "Mbps";
	}
	***/
//	fiducialWidget->perfMon()->setQuality(_Q_fiducial);
	fiducialWidget->setQuality(_Q_fiducial);




	/**
	  * Find the linear curve
	  **/

	qreal slope = -1;
	qreal y_intercept = 0;
	if ( _Q_fiducial >= _Q_highest ) {
		// there's no need to come up with equation to calculate quality scales
		// quality scales are gonna be _Q_highest
	}
	else {
		// A linear equation to calculate quality scale of widget important than anchor widget
		// slope and y-intercept of linear equation (y2-y1)/(x2-x1)  and  b = y - mx
		slope = (_Q_highest - _Q_fiducial ) / (highestp - fiducialWidgetPriority);
//		qDebug() << "slope=" << _Q_highest << "-" << anchorWidgetQuality << "/" << highestp << "-" << anchorWidget->priority();
		y_intercept = _Q_fiducial - (slope * fiducialWidgetPriority);

		qDebug() << "\nThe linear function determined. QUALITY SCALE =" << slope  << "* PRIORITY +" << y_intercept << "\n";
	}



	/**
	  * Apply new quality scale based on the curve, and find the knapsack size
	  * It is total amount of bandwidth required for these widgets to fullfill the desired quality
	  **/

	foreach (SN_RailawareWidget *higher, widgetList) {
		if ( higher->priority(currMsecSinceEpoch) <= fiducialWidgetPriority ) continue;

		/*
		  calculate desired minimum fps
		  */
		qreal newQualityScale = _Q_highest;

		// if the curve exist,
		if ( slope >= 0 ) {
			newQualityScale = slope * higher->priority(currMsecSinceEpoch) + y_intercept;  // linear
		}
//		qreal desired_minimum_fps = newQualityScale * higher->perfMon()->getExpetctedFps();


		/*
		  apply new desired quality to high priority widgets (higher than the anchor widget) if needed
		  */
//		if ( higher->perfMon()->currObservedQuality() >= newQualityScale ) {
////		if ( higher->perfMon()->getCurrRecvFps() >= desired_minimum_fps ) {
//			// this widget already showing better
//			qDebug() << "\tA high priority widget" << higher->globalAppId() << "is doing ok :" << higher->perfMon()->getCurrRecvFps();

//			/****

//			  create inverse knapsack here

//			  ***/

//			qreal redundantFps = higher->perfMon()->getCurrRecvFps() - desired_minimum_fps;
//			totalRedundantResource += (redundantFps * 8 * higher->appInfo()->getFrameBytecount()); // in bps
//			redunMap.insert(higher->priority(), higher);
//			qDebug() << "\t\tRedundant res is now" << totalRedundantResource * 0.000001 << "Mbps";

//		}
//		else {
//			// this widget needs to be showing better quality
//			higher->perfMon()->setAdjustedFps( desired_minimum_fps );
//			qDebug() << "\tA high priority widget" << higher->globalAppId() << " quality scale" << newQualityScale << "adjusted fps" << higher->perfMon()->getAdjustedFps();


//			/*
//			  Now calculate the knapsack size. -> How much resource should be taken from lower priority widgets?
//			  */
//			qreal expectedBW = 8 * higher->appInfo()->getFrameBytecount() * higher->perfMon()->getAdjustedFps(); // desired bandwidth based on the curve (in bps)
//			qreal observedBW = 1000000 * higher->perfMon()->getCurrBandwidthMbps(); // current observed bandwidth in bps
//			qreal requiredBW = expectedBW - observedBW;
//			if (requiredBW > 0)
//				totalRequiredResource += requiredBW; // fixed size knapsack (in bps)
//			else {
//				// nothing has to be done for this widget. It has already achieved desired quality
//			}
//		}


//		higher->perfMon()->setAdjustedFps( desired_minimum_fps );
		higher->setQuality(newQualityScale);

		qreal expectedBW = 8 * higher->appInfo()->frameSizeInByte() * higher->perfMon()->getAdjustedFps(); // desired bandwidth based on the curve (in bps)
		qreal observedBW = 1000000 * higher->perfMon()->getCurrBandwidthMbps(); // current observed bandwidth in bps
		qreal requiredBW = expectedBW - observedBW;
		if (requiredBW > 0)
			totalRequiredResource += requiredBW; // fixed size knapsack (in bps)
		else {
			// nothing has to be done for this widget. It has already achieved desired quality
		}

	}


	rwlock.unlock();



	/**
	  * Find out how much bandwidth to preempt from which widget
	  * It's bounded knapsack problem where weight is frame cost and value is priority
	  */

	QList<SN_RailawareWidget *> wlist; // sorted low priority widget list
	QList<SN_RailawareWidget *>::iterator iter;

	qreal scoop = 1.0; // 1 frame
//	qreal scoop = 0.5; // half frame


	if (totalRequiredResource > 0) {

		qDebug() << "The total required resource" << totalRequiredResource * 0.000001 << "Mbps";

//		if (totalRedundantResource > 0) {
//			// get it from redundant
//			qDebug() << "\tAnd there's redundant resource available";

//			QMap<qreal, RailawareWidget *>::iterator iter;
//			for (iter=redunMap.begin(); iter!=redunMap.end(); iter++) {

//			}
//		}
//		else {
			// get if from lower priority widgets

			// sort widget by unit value
			foreach (SN_RailawareWidget *lower, widgetList) {
				if ( lower->priority(currMsecSinceEpoch) >= fiducialWidgetPriority ) continue;
				if ( lower->perfMon()->getCurrBandwidthMbps() <= 0 ) continue;

				// init widget's adjusted Fps to curretn fps
				//		lower->perfMon()->setAdjustedFps(  lower->perfMon()->getAvgRecvFps() );

				qreal unitValue = lower->priority(currMsecSinceEpoch) / lower->perfMon()->getCurrBandwidthMbps();

				// sorted by unitValue - ascending order. will preempt more from smaller unitvalue
				for (iter=wlist.begin(); iter!=wlist.end(); iter++) {
					SN_RailawareWidget *r = *iter;
					if ( (r->priority(currMsecSinceEpoch) / r->perfMon()->getCurrBandwidthMbps())  >  unitValue ) {
						// lower is less important. insert before *iter
						wlist.insert(iter, lower);
						break;
					}
				}
				if (iter == wlist.end()) {
					wlist.push_back(lower);
				}
			}

			if (wlist.isEmpty()) {
				qDebug("%s::%s() : required resource is %.4f Mbps but there's no lower priority widgets (lower than %.4f)", metaObject()->className(), __FUNCTION__, totalRequiredResource * 0.000001, fiducialWidgetPriority);
				return;
			}

			/**
			  * Preempt resource from lower priority widgets
			  */

//			while (totalRequiredResource > 0) {
				foreach (SN_RailawareWidget *r, wlist) {

					// lower quality
//					if ( r->perfMon()->reAdjustFramerateBy( -1 * scoop) == -1 ) {
					if ( r->adjustQuality(-1 * 0.1) == -1 ) {
						// can't lower more !

						// if inverse knapsack is not empty
						// resource should be preempted from there
					}
					else {
						qDebug() << "\tlower priority widget id" << r->globalAppId() << " adjusted fps" << r->perfMon()->getAdjustedFps();
						// update knapsack ( is in bps )
						totalRequiredResource -= (scoop * 8 * r->appInfo()->frameSizeInByte()); // scoop frame worth bandwidth
					}

					/*********************************
					// This loop can be running forever !!!
					**********************************/
//					if ( totalRequiredResource <= 0 ) break;
				}
//			}
//		}
	}
	else {
//		if (totalRedundantResource > 0) {
//			// distribute resource to lower priority widgets
//		}
//		else {
			// try increase lower priority widgets' quality

			qDebug() << "Higher priority widgets don't need more resources";

			// try increase quality of low priority widgets
			foreach(SN_RailawareWidget *r, widgetList) {
				if ( r->priority(currMsecSinceEpoch) < fiducialWidgetPriority ) {

//					int ret = r->perfMon()->reAdjustFramerateBy(1.5 * scoop);
					int ret = r->adjustQuality(0.1);
					if ( ret == 1 ) {
						qDebug() << "\tCan't increase frame rate more than expected rate";
					}
					else if (ret == 0 ) {
						qDebug() << "\ttry increasing quality of lower priority widget" << r->globalAppId() << "by" << r->perfMon()->getAdjustedFps();
					}
					else {
						// waht the fuck ?
					}
				}
			}
//		}
	}



	/**
	  * Preempt resource proportional to unit value
	  */
//	if ( totalRequiredResource > 0 ) {
//		qreal sumUnitValue = 0.0;
//		foreach (RailawareWidget *rw, widgetList) {
//			if ( rw->priority() >= anchorWidgetPriority ) continue;
//			if ( rw->perfMon()->getCurrBandwidth() <= 0 ) continue;

//			qreal unitValue = rw->priority() / rw->perfMon()->getCurrBandwidth();
//			sumUnitValue += unitValue;

//			wlist.push_back(rw);
//		}

//		foreach (RailawareWidget *rw, wlist) {
//			if ( rw->perfMon()->getCurrBandwidth() <= 0 ) continue;

//			qreal unitValue = rw->priority() / rw->perfMon()->getCurrBandwidth();

//			Q_ASSERT(sumUnitValue > 0);
//			qreal bandwidthToPreempt = (unitValue / sumUnitValue) * totalRequiredResource; // in bps

//			qreal frameAdjustment = bandwidthToPreempt / (8 * rw->appInfo()->getFrameSize());
//			rw->perfMon()->setAdjustedFps( rw->perfMon()->getExpetctedFps() - frameAdjustment );

//			qDebug() << "\tA low widget id" << rw->globalAppId() << " frame adjustment is" << frameAdjustment << "adjusted FPS" << rw->perfMon()->getAdjustedFps();
//		}
//	}

















/****************************************************



	// re-examine widget list
	foreach (RailawareWidget *rwme, rMonitor->getWidgetList()) {
		if (!rwme) continue;

		// per-frame cost
//		rwme->appInfo()->getFrameSize() * rwme->perfMon()->getExpetctedFps(); // Byte per second

		qreal currAdjFps = rwme->perfMon()->getAdjustedFps();
		qreal expectedFps = rwme->perfMon()->getExpetctedFps();
		bool currentlyOk = true;

		foreach (RailawareWidget *rw, rMonitor->getWidgetList()) {
			if (!rw || rwme == rw) continue;

			// I'm more important than rw
			if (rw->priority() < rwme->priority()) {

				// if my quality is worse than lower priority task rw
				if ( rw->perfMon()->getCurrAbsDeviation() <= rwme->perfMon()->getCurrAbsDeviation()) {
					currentlyOk = false;

					// if I'm already set to maximum and my quality loss is noticeable
					if (currAdjFps == expectedFps   &&  rwme->perfMon()->getCurrAbsDeviation() >= 1 ) {
						// decrease quality of lower priority task rw
						rw->perfMon()->setAdjustedFps( rw->perfMon()->getAdjustedFps() - 2 );
					}

					// I'm not set to maximum
					else {
						// increase quality of myself
						rwme->perfMon()->setAdjustedFps(currAdjFps + 1);
					}
				}
			}
			else if (rw->priority() == rwme->priority()) {

			}
			// I'm less important than higher priority task rw
			else {
				// if higher priority task rw has lower quality
				if (rw->perfMon()->getCurrAbsDeviation() > rwme->perfMon()->getCurrAbsDeviation()) {

					currentlyOk = false;

					// I have lower quality of myself
					rwme->perfMon()->setAdjustedFps( currAdjFps - 1 );
//					qDebug("\twidget %llu 's adjusted fps is now %.2f", rwme->globalAppId(), rwme->perfMon()->getAdjustedFps());
					//break;
				}
			}
		}

		// if everything is fine, try increase my quality
		if (currentlyOk) {
			rwme->perfMon()->setAdjustedFps( currAdjFps + 1 );
		}
	}

	*****************************************/





	/* signal (wake) based on adjusted deadline */
	/*************
	struct timeval now;
	gettimeofday(&now, 0);
	qreal nowsec = (qreal)(now.tv_sec) + 0.000001 * (qreal)(now.tv_usec);

	qreal frame_deadline = 0, sched_deadline = 0;

	foreach (RailawareWidget *rw, rMonitor->getWidgetList()) {
		if (!rw) continue;
		if (rw->isScheduled()) continue; // already scheduled

		// frame must be displayed at this time
		frame_deadline = rw->perfMon()->ts_nextframe(); // this reflects adjusted framerate

		// recv must be scheduled at this time
		sched_deadline = frame_deadline - rw->perfMon()->getCurrRecvLatency() + rw->perfMon()->getAvgConvDelay();

		qreal delay = sched_deadline - nowsec;
		if (delay > 0) {
			// schedule recv()
			QTimer::singleShot( (sched_deadline - nowsec) * 1000, rw, SLOT(scheduleReceive()) );
		}
		else {
			if ( ! QMetaObject::invokeMethod(rw,  "scheduleReceive", Qt::QueuedConnection) )
				qCritical("%s::%s() : Failed to invoke scheduleUpdate on widget %llu", metaObject()->className(), __FUNCTION__, rw->globalAppId());
		}
		// sets the flag to prevent duplicate scheduling
		rw->setScheduled(true);
	}
		*************/
}
