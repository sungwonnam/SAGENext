#include "resourcemonitor.h"
#include "resourcemonitorwidget.h"
#include "sagenextscheduler.h"

//#include "../graphicsviewmainwindow.h"

#include "../common/commonitem.h"

#include "../applications/base/affinityinfo.h"
#include "../applications/base/perfmonitor.h"
#include "../applications/base/railawarewidget.h"

#include <QSettings>

ProcessorNode::ProcessorNode(int type, int i) :
                nodeType(type),
                id(i),
                cpuUsage(0.0), bandwidth(0.0)
{
        _appList = new QList<RailawareWidget *>();
        _appList->clear();
}

ProcessorNode::~ProcessorNode() {
        if (_appList) {
                delete _appList;
        }
//	qDebug("ProcessorNode::%s() : proc node %d", __FUNCTION__, id);
}

int ProcessorNode::getNumWidgets() const {
        return _appList->size();
}

void ProcessorNode::addApp(RailawareWidget *app) {
        if ( _appList && !_appList->contains(app) ) {
//		appList->push_back(app);
//		qDebug("ProcessorNode::%s() : app %llu has added to ProcessorNode %d", __FUNCTION__, app->getWidgetPtr()->getGlobalAppId(), id);

                _appList->push_back(app);
                cpuUsage += app->perfMon()->getCpuUsage();
                bandwidth += app->perfMon()->getCurrBandwidthMbps();

                /***
                if ( _appList->empty() ) {
                        _appList->push_back(app);
                        return;
                }

                QList<RailawareWidget *>::iterator it = _appList->begin();
                RailawareWidget *rw = 0;
                for (; it!= _appList->end(); it++ ) {
                        rw = *it;
                        if ( app->priority() >= rw->priority() )  {
                                _appList->insert(it, app);
//				qDebug() << "ProcessorNode::addApp() : app " << app->globalAppId() << " inserted";
                                break;
                        }
                }
                ***/
        }
}

bool ProcessorNode::removeApp(quint64 appid) {
    if(!_appList) return false;

    RailawareWidget *rw = 0;
    for ( int i=0; i<_appList->size(); ++i) {
        rw = _appList->at(i);

        Q_ASSERT(rw && rw->affInfo());
        if (rw->affInfo() && rw->globalAppId() == appid) {
            _appList->removeAt(i);
            return true;
        }
    }
    return false;
}

bool ProcessorNode::removeApp(RailawareWidget *rw) {
    if ( !_appList || _appList->isEmpty())
        return false;
    else {
        if (_appList->removeAll(rw)) {
            //qDebug("[%s] ProcessorNode::%s() : app %llu has been removed from proc node %d", qPrintable(QTime::currentTime().toString("hh:mm:ss.zzz")), __FUNCTION__, rw->globalAppId(), id);
            return true;
        }
    }
    return false;
}

void ProcessorNode::refresh() {
    /* iterator over appList and add cpuUsage, bandwidth */
    //	const AffinityInfo *aff = 0;
    const PerfMonitor *perf = 0;
    RailawareWidget *rw = 0;

    // reset
    cpuUsage = 0.0;
    bandwidth = 0.0;

    for ( int i=0; i<_appList->size(); ++i ) {
        rw = _appList->at(i);
        if (!rw || !rw->affInfo() || !rw->perfMon()) {
            // remove this app from the List
            _appList->removeAt(i);
            --i;
            continue;
        }
        else {
            perf = rw->perfMon();
            Q_ASSERT(perf);
            cpuUsage += perf->getCpuUsage(); // Ratio
            bandwidth += perf->getCurrBandwidthMbps(); // Mbps
        }
    }

//	QByteArray text(512, '\0');
//	sprintf(text.data(), "CPU%d, %d,   %.2f,   %.2f",
//			id,
//			appList->size(),
//			bandwidth,
//			cpuUsage * 100.0
//				);
//	setText(QString(text));
}

int ProcessorNode::prioritySum() const {
        int sum = 0;

        for (int i=0; i<_appList->size(); ++i) {
                RailawareWidget *rw = _appList->at(i);
                if (!rw) continue;

                /*!
                  It assumes _priorityQuantized had been set before calling this function
                  */
                sum += rw->priorityQuantized();
        }
        return sum;
}

void ProcessorNode::printOverhead() const {
//	const AffinityInfo *aff = 0;
        const PerfMonitor *perf = 0;
        RailawareWidget *widget = 0;

        if (_appList->size() == 0) {
                qDebug("ProcessorNode::%s() : There's no app on this processor %d", __FUNCTION__, id);
                return;
        }

        for ( int i=0; i<_appList->size(); ++i ) {

                widget = _appList->at(i);

                if (!widget || !widget->affInfo() || !widget->perfMon()) {
                        // remove this app from the List
                        _appList->removeAt(i);
                        --i;
                        continue;
                }
                else {
//			widget = aff->widgetPtr();
                        perf = widget->perfMon();

                        // currRecvFps, currBandwidth,  currConvDelay, currDrawLatency,currDispFps
                        qDebug("ProcessorNode::%s() : Processor %d, app %llu [%.2f fps, %.2f Mbps, %.2f ms, %.2f ms, %.2f fps]", __FUNCTION__, id, widget->globalAppId(),
                                   perf->getCurrRecvFps(),
                                   perf->getCurrBandwidthMbps(),
                                   perf->getCurrConvDelay() * 1000.0,
                                   perf->getCurrDrawLatency() * 1000.0,
                                   perf->getCurrDispFps());
                }
        }
}



ResourceMonitor::ResourceMonitor(const QSettings *s) :
        textItem(new SwSimpleTextItem),
        procVec(0),
        settings(s),
        schedcontrol(0),
        numaInfo(0),
        _rMonWidget(0),
        _printDataFlag(false)
{
        textItem->setText("");

        buildProcVector();
        Q_ASSERT(procVec);

        if (settings->value("system/numnumanodes").toInt() > 1) {
                numaInfo = (Numa_Info *)malloc(sizeof(Numa_Info) * settings->value("system/numnumanodes").toInt());
                for (int i=0; i<settings->value("system/numnumanodes").toInt(); ++i) {
                        numaInfo[i].local_node = 0;
                        numaInfo[i].numa_hit = 0;
                        numaInfo[i].numa_miss = 0;
                        numaInfo[i].numa_foreign = 0;
                        numaInfo[i].other_node = 0;
                        numaInfo[i].hit = 0;
                        numaInfo[i].miss = 0;
                        numaInfo[i].foreign = 0;
                        numaInfo[i].local = 0;
                        numaInfo[i].other = 0;
                }
        }

//	widgetMultiMap.clear();
        widgetList.clear();
        widgetListRWlock.unlock();
}

void ResourceMonitor::timerEvent(QTimerEvent *) {
//	qDebug() << "timerEvent at resourceMonitor";
        refresh(); // update all data
        if (_rMonWidget && _rMonWidget->isVisible()) _rMonWidget->refresh();

        if (_printDataFlag) {
                printPrelimData();
        }
}

void ResourceMonitor::addSchedulableWidget(RailawareWidget *rw) {
        widgetListRWlock.lockForWrite();
        /* shouldn't allow duplicate item */

//	ProcessorNode *pn = getMostUnderloadedProcessor();

        if (rw) {
//		if (pn) {
//			rw->affInfo()->setReadyBit(AffinityInfo::CPU_MASK, pn->getID());
//		}
//		widgetMultiMap.insert(rw->priority(), rw);
//		widgetMultiMap.insert(rw->ratioToTheWall(), rw);
                widgetList.push_front(rw);
        }

        widgetListRWlock.unlock();
}

void ResourceMonitor::removeSchedulableWidget(RailawareWidget *rw) {
        widgetListRWlock.lockForWrite();
        if(rw) {
//		if ( widgetMultiMap.remove(rw->priority(), rw) != 1 ) {
//			qDebug("ResourceMonitor::%s() : failed to remove", __FUNCTION__);
//		}
                widgetList.removeAll(rw);
        }
        widgetListRWlock.unlock();
}

//void ResourceMonitor::rearrangeWidgetMultiMap(BaseWidget *bw, int oldpriority) {
//	rwlock.lockForWrite();

//	// remove
//	RailawareWidget *rw = static_cast<RailawareWidget *>(bw);
//	widgetMultiMap.erase( widgetMultiMap.find(oldpriority, rw) );

//	// insert
//	widgetMultiMap.insert(bw->priority(), rw);

//	rwlock.unlock();
//}

RailawareWidget * ResourceMonitor::getEarliestDeadlineWidget() {
        widgetListRWlock.lockForRead();
        qreal temp = -1;
        RailawareWidget *result = 0;
//	QMultiMap<int, RailawareWidget *>::iterator it;
        QList<RailawareWidget *>::iterator it;
//	for(it=widgetMultiMap.begin(); it!=widgetMultiMap.end(); it++) {
        for(it=widgetList.begin(); it!=widgetList.end(); it++) {
//		RailawareWidget *r = it.value();
                RailawareWidget *r = *it;
                if ( temp == -1 || temp > r->perfMon()->ts_nextframe() ) {
                        temp = r->perfMon()->ts_nextframe();
                        result = r;
                }
        }

        widgetListRWlock.unlock();
        return result;
}


ResourceMonitor::~ResourceMonitor() {
	//	if (procTree) delete procTree;

	if (schedcontrol) {
		schedcontrol->killScheduler();
		delete schedcontrol;
	}

	if (procVec) {
		ProcessorNode *pn =0;
		for (int i=0; i<procVec->size(); ++i) {
			pn = procVec->at(i);
			if(pn) {
				//				qDebug() << "3";
				delete pn;
			}
		}
		delete procVec;
	}

	/* I shouldn't delete numaInfoTextItem here */

	if (numaInfo) free(numaInfo);

	if (_dataFile.isOpen()) {
		_dataFile.flush();
		_dataFile.close();
	}

	qDebug("ResourceMonitor::%s()", __FUNCTION__);
}

//void ResourceMonitor::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
//	Q_UNUSED(option);
//	Q_UNUSED(widget);
//
//	painter->setBrush(Qt::darkGray);
//	painter->drawRect(windowFrameRect());
//
//	painter->setBrush(Qt::white);
//	painter->drawText(boundingRect(), Qt::AlignTop | Qt::AlignLeft, infoText);
//
//}

void ResourceMonitor::refresh() {
//	QString infoText = "Per CPU stat\n(# threads, B/W, CPU usage)\n";

        /* per processor info */
        if (procVec) {
//		QByteArray str(128, '\0');
                ProcessorNode *pn = 0;
                for (int i=0; i<procVec->size(); ++i) {
                        pn = procVec->at(i);
                        pn->refresh();

//			sprintf(str.data(), "CPU%d: %d\t%.2f\t%.2f\n",pn->getID(), pn->appList()->size(), pn->getBW(), pn->getCpuUsage());
//			infoText.append( QString(str) );

                        // dont do below here !!
//			qApp->sendPostedEvents(); // looks like this makes user interaction slow
                }
        }

        /* numa info */
        /**
        infoText.append("\nNUMA stat\n(hit, miss, foreign, local, other)");
        if (settings->value("system/numnumanodes").toInt() > 1 ) {
                QProcess p;
                for (int i=0; i<settings->value("system/numnumanodes").toInt(); ++i) {
                        QString arg = "cat /sys/devices/system/node/node";
                        arg.append(QString::number(i));
                        arg.append("/numastat");
                        p.start(arg);
                        p.waitForFinished(-1); // block permanently
                        QStringList sl = QString(p.readAllStandardOutput()).split(QRegExp("\\s+"), QString::SkipEmptyParts);

//			qDebug() << sl;

                        if(numaInfo) {
                                infoText.append("\nnode " + QString::number(i) + ": ");

                                numaInfo[i].hit = ((QString)sl[1]).toULongLong() - numaInfo[i].numa_hit;
                                numaInfo[i].miss = ((QString)sl[3]).toULongLong() - numaInfo[i].numa_miss;
                                numaInfo[i].foreign = ((QString)sl[5]).toULongLong() - numaInfo[i].numa_foreign;
                                numaInfo[i].local = ((QString)sl[9]).toULongLong() - numaInfo[i].local_node;
                                numaInfo[i].other = ((QString)sl[11]).toULongLong() - numaInfo[i].other_node;

                                numaInfo[i].numa_hit = ((QString)sl[1]).toULongLong();
                                numaInfo[i].numa_miss = ((QString)sl[3]).toULongLong();
                                numaInfo[i].numa_foreign = ((QString)sl[5]).toULongLong();
                                numaInfo[i].local_node = ((QString)sl[9]).toULongLong();
                                numaInfo[i].other_node = ((QString)sl[11]).toULongLong();

//				qDebug() << i << ":" << numaInfo[i].hit << numaInfo[i].miss << numaInfo[i].foreign << numaInfo[i].local << numaInfo[i].other;
                                // rate of change where delta x is 1 sec
                                infoText.append( QString::number(numaInfo[i].hit) + ", " );
                                infoText.append( QString::number(numaInfo[i].miss)+ ", " );
                                infoText.append( QString::number(numaInfo[i].foreign)+ ", " );
                                infoText.append( QString::number(numaInfo[i].local)+ ", " );
                                infoText.append( QString::number(numaInfo[i].other) );
                        }

                }
        }
        **/
        textItem->setText(infoText);

        /* update ROI */
        /**
        foreach (ROIRectF *r, roiRectFs) {
                if (r) r->update();
        }
        **/
}

//ProcessorNode * ResourceMonitor::findNode(quint64 appid) {
//	/* traverse tree and return the node pointer */
//	return 0;
//}


void ResourceMonitor::printPrelimDataHeader() {
        QDir::setCurrent(QDir::homePath() + "/.sagenext");
#if QT_VERSION >= 0x040700
        _dataFile.setFileName( QString::number(QDateTime::currentMSecsSinceEpoch()) + ".data" );
#else
        _dataFile.setFileName( QDateTime::currentDateTime().toString("MM.dd.yyyy_hh.mm.ss.zzz") + ".data");
#endif
        if ( ! _dataFile.open(QIODevice::ReadWrite) ) {
                qDebug() << "can't open prelim data file" << _dataFile.fileName();
        }
        QString dataHeader = "";
        if ( schedcontrol && schedcontrol->isRunning() ) {
                SelfAdjustingScheduler *sas = static_cast<SelfAdjustingScheduler *>(schedcontrol->scheduler());
                dataHeader.append("Scheduler (");
                // freq, sensitivity, aggression, greediness
                dataHeader.append("Frequency: " + QString::number(schedcontrol->scheduler()->frequency()) );
                dataHeader.append(" Sensitivity: " + QString::number(sas->getQTH()));
                dataHeader.append(" Aggression: " + QString::number(sas->getDecF()));
                dataHeader.append(" Greediness: " + QString::number(sas->getIncF()));
                dataHeader.append(")");
        }
        else {
                dataHeader.append("No Scheduler");
        }

        if (settings->value("system/sailaffinity").toBool()) {
                dataHeader.append(", SAIL Affinity");
        }
        if (settings->value("system/threadaffinityonirq").toBool()) {
                dataHeader.append(", Thread Affinity on IRQ");
        }
        dataHeader.append("\n");
        _dataFile.write( qPrintable(dataHeader) );
}

/*!
  This function should be called at every time slice
  */
void ResourceMonitor::printPrelimData() {

//	QString filename_date(filename);
//	filename_date.append( QString::number(QDateTime::currentMSecsSinceEpoch()));
//	QFile file(filename_date);
//	if ( ! file.open(QIODevice::ReadWrite) ) {
//		qCritical() << file << " couldn't be opened for writing";
//		return;
//	}

        if (widgetList.size() == 0 ) return;


        int count = 0;
        widgetListRWlock.lockForRead();

        // # of apps
        _dataFile.write( qPrintable(QString::number(widgetList.size())) );
        _dataFile.write(",");

        // cpu usage aggregate
        qreal cpuUsageSum = 0.0;
        foreach (RailawareWidget *rw, widgetList) {
                cpuUsageSum += rw->perfMon()->getCpuUsage();
        }
        cpuUsageSum *= 100; // convert to percentage
        _dataFile.write( qPrintable(QString::number(cpuUsageSum)) );
        _dataFile.write(",");

        foreach (RailawareWidget *rw, widgetList) {
                ++count;
                // time slice is implicit (each row = single time slice)

                // Observed quality of each app
                if ( rw->observedQuality() >= 1 )
                        _dataFile.write( "1.0" );
                else
                        _dataFile.write( qPrintable(QString::number(rw->observedQuality())) );

                if (count < widgetList.size())
                        _dataFile.write(",");

        }
        _dataFile.write("\n");
        widgetListRWlock.unlock();

        // file is closed at the calling function
}


void ResourceMonitor::removeApp(RailawareWidget *rw) {
    //	AffinityInfo *aff = static_cast<AffinityInfo *>(obj);
    ProcessorNode *pn = 0;
    Q_ASSERT(procVec);
    for ( int i=0; i<procVec->size(); ++i) {
        pn = procVec->at(i);
        if(pn) {
            if ( pn->removeApp(rw)  ) {
                emit appRemoved(i);
                qDebug("ResourceMonitor::%s() : app %llu has been removed from proc node %d", __FUNCTION__, rw->globalAppId(), i);
            }
        }
    }
}

/*!
  this slot connected to AffinityInfo::cpuOfMineChanged signal
  */
void ResourceMonitor::updateAffInfo(RailawareWidget *rw, int oldproc, int newproc) {
//	affinf->getGlobalAppId();
//	affinf->getCpuOfMine();

        if ( !procVec ) return;

        ProcessorNode *pn = 0;

        // O(1)
        if ( oldproc >= 0 && newproc >= 0 ) {
                Q_ASSERT( 0 <= oldproc && oldproc < procVec->size() );
                pn = procVec->at(oldproc);
                Q_ASSERT(pn && oldproc == pn->getID());
                pn->removeApp(rw);
                pn = 0;

                Q_ASSERT( 0 <= newproc && newproc < procVec->size() );
                pn = procVec->at(newproc);
                Q_ASSERT(pn && newproc == pn->getID());
//		qDebug() << "rMonitor::updateAffInfo() : ----------- adding app id " << rw->globalAppId() << " to proc " << pn->getID();
                pn->addApp(rw);
        }
        // for each processor, O(n)
        else {
                for (int i=0; i<procVec->size(); ++i) {
                        pn = procVec->at(i);
                        Q_ASSERT(pn);

                        // if affinityInfo of this widget tells it is affine to processor i then
                        if ( i == rw->affInfo()->cpuOfMine() ) {
                                Q_ASSERT( i == pn->getID() );
//				qDebug() << "rMonitor::updateAffInfo() : ----------- adding app id " << rw->globalAppId() << " to proc " << pn->getID();
                                pn->addApp(rw);
                        }
                        else {
                                pn->removeApp(rw);
                        }
                }
        }
}


/**
void ResourceMonitor::buildProcTree() {

        //procTree = new QTreeWidget;
        Q_ASSERT(procTree);
        procTree->setColumnCount(4);
        QStringList headerLabels;
        headerLabels << "Type" << "ID" << "# recv threads" << "memory";
        procTree->setHeaderLabels(headerLabels);

//	QStringList initText;
//	initText << "Root" << "-1" << "0" << "0";
        ProcessorNode *root = new ProcessorNode(ProcessorNode::ROOT_NODE, -1);
        root->setText(1, "-1");
        root->setText(2, "0");
        root->setText(3, "0");
        procTree->addTopLevelItem(root);

        for ( int i=0; i<settings->value("system/numnumanodes").toInt(); ++i ) {
                ProcessorNode *numanode = new ProcessorNode(ProcessorNode::NUMA_NODE, i, root);
                root->addChild(numanode);
                numanode->setData(0, Qt::DisplayRole, QVariant("NUNA node"));
                numanode->setData(1, Qt::DisplayRole, QVariant(i));

                for ( int j=0; j<settings->value("system/cpupernumanode").toInt(); ++j ) {
                        ProcessorNode *p_core = new ProcessorNode(ProcessorNode::PHY_CORE, j, numanode);
                        numanode->addChild(p_core);
                        p_core->setData(0, Qt::DisplayRole, QVariant("Phy Core"));


                        // Is this SMT ?
                        if ( settings->value("system/threadpercpu").toInt() > 1 ) {
                                for ( int k=0; k<settings->value("system/threadpercpu").toInt(); ++k ) {
                                        // How to find out logical core id in a physical core??

                                        // cat /sys/devives/system/node/node(i)/cpulist
                                        // 0-3,8-11
                                        ProcessorNode *cpu = new ProcessorNode(ProcessorNode::SMT_CORE, k, p_core);
                                        p_core->addChild(cpu);
                                        cpu->setData(0, Qt::DisplayRole, QVariant("cpu"));
                                }
                        }
                }
        }
}

**/


void ResourceMonitor::buildProcVector() {
        int totalCpu = settings->value("system/numcpus").toInt();

        Q_ASSERT(!procVec);
        procVec = new QVector<ProcessorNode *>();

        ProcessorNode *pn = 0;
        for (int i=0; i<totalCpu; ++i) {
                pn = new ProcessorNode(ProcessorNode::PHY_CORE, i);
                                if ( settings->value("system/threadpercpu").toInt() > 1 ) {
                                        pn->setNodeType(ProcessorNode::SMT_CORE);

                                        /******
                                          only works in venom (two intel quad core smt cpus)
                                          *****/
                                        if ( i < 8 ) {
                                                pn->setSMTSiblingID( i + 8 );
                                        }
                                        else {
                                                pn->setSMTSiblingID(i - 8);
                                        }

                                }
                procVec->push_back(pn);
        }
//	qDebug() << "rMonitor::buildProcVector() : " << procVec->size() << "processors";
}


ProcessorNode * ResourceMonitor::getMostUnderloadedProcessor() {
        ProcessorNode *pn = 0;
        ProcessorNode *result = 0;
        qreal temp = INT_MAX;
        for (int i=0; i<procVec->size(); ++i) {
                pn = procVec->at(i);
                pn->refresh();

                if ( pn->getBW() < temp ) {
                        temp = pn->getBW();
                        result = pn;
                }
        }
        qDebug("%s::%s() : returning pid %d", metaObject()->className(), __FUNCTION__, result->getID());
        return result;
}


int ResourceMonitor::assignProcessor(RailawareWidget *rw, ProcessorNode *pn) {
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

int ResourceMonitor::assignProcessor(RailawareWidget *rw) {
        if (!rw || !rw->affInfo()) return -1;

        ProcessorNode *pn = getMostUnderloadedProcessor();
        if (!pn) return -1;

        return assignProcessor(rw, pn);
}

/*!
  Re assign rail for all processes
  */
int ResourceMonitor::assignProcessor() {

        /* pause scheduler */
        if(schedcontrol) {
                schedcontrol->killScheduler();
        }

        foreach (RailawareWidget *w, widgetList) {
                /* disconnect signal/slot connection for now */
                if (!disconnect(w->affInfo(), SIGNAL(cpuOfMineChanged(RailawareWidget*,int,int)), this, SLOT(updateAffInfo(RailawareWidget*,int,int))) ) {
                        qDebug() << "disconnect() failed";
                }

                /* remove all widget from pn _appList */
                removeApp(w); // what about loadBalance() connection?
        }

        //	qApp->sendPostedEvents();

        for (int i=0; i<procVec->size(); ++i) {
                ProcessorNode *pn = procVec->at(i);
                pn->setBW(0);
                pn->setCpuUsage(0);
        }

//	sleep(1);

        QList<RailawareWidget *>::iterator it;
        RailawareWidget *rw = 0;
        int count = 0;

        for(it=widgetList.begin(); it!=widgetList.end();  it++) {
                rw = *it;
                if ( assignProcessor(rw) >= 0 ) {
                        ++count;
                        //			qApp->flush();

                        //			qApp->sendPostedEvents();
                        //			QThread::yieldCurrentThread();

                        //			sleep(1);
                        //			refresh();
                }
                // perf data shouldn't be reset here
                //                rw->perfMon()->reset();

                // reconnect
                connect(rw->affInfo(), SIGNAL(cpuOfMineChanged(RailawareWidget*,int,int)), this, SLOT(updateAffInfo(RailawareWidget*,int,int)));
        }


        /* resume scheduler */
        if(schedcontrol) {
                schedcontrol->launchScheduler(settings->value("system/scheduler_type").toString());
        }

        if (count != widgetList.size()) {
                qDebug("%s::%s() : Error: %d widgets are assigned. widgetList has %d items",metaObject()->className(), __FUNCTION__, count, widgetList.size());
                return -1;
        }
        return count;
}

void ResourceMonitor::resetProcessorAllocation(RailawareWidget *rw) {
        if (rw && rw->affInfo())
                rw->affInfo()->setAllReadyBit();
}

void ResourceMonitor::resetProcessorAllocation() {
        QList<RailawareWidget *>::iterator it;
        RailawareWidget *rw = 0;

        widgetListRWlock.lockForRead();
        for(it=widgetList.begin(); it!=widgetList.end();  it++) {
                rw = *it;
                rw->perfMon()->reset(); // reset perf data
                resetProcessorAllocation(rw);
        }
        widgetListRWlock.unlock();
}


ProcessorNode * ResourceMonitor::processor(int pid) {
        if (!procVec) {
                qCritical("%s::%s() : procVec is null", metaObject()->className(), __FUNCTION__);
                return 0;
        }

        return procVec->at(pid);

}


void ResourceMonitor::loadBalance() {
        // find the most overloaded processor
        ProcessorNode *pn_high = 0, *pn_low=0;
        qreal loadhigh = 0.0, loadlow = 100.0;

        if (!procVec || procVec->isEmpty()) return;


        // for each processor
        for (int i=0; i<procVec->size(); ++i) {
                ProcessorNode *tmp = procVec->at(i);

                if (tmp->getCpuUsage() > loadhigh) {
                        loadhigh = tmp->getCpuUsage();
                        pn_high = tmp;
                }
                else if (tmp->getCpuUsage() < loadlow) {
                        loadlow = tmp->getCpuUsage();
                        pn_low = tmp;
                }
        }
        qDebug("ResourceMonitor::%s() : H: (%d, %.3f),  L: (%d, %.3f)", __FUNCTION__, pn_high->getID(), loadhigh, pn_low->getID(), loadlow);


        // Is load factor greater than 0.9 ? otherwise no migratino needed.
        if (loadhigh <= 0.9) {
                // do nothing

                // What if pn_low has no widget ?? Isn't it better to migrate to pn_low ?

                return;
        }

        if (pn_high == pn_low) return;


        /*************************************
          Which processor to migrate ?????????
          *************************************/

        // for now, find the lowest priority widget from heavily loaded processor
        int priority = INT_MAX;
        RailawareWidget *rw = 0;
        QList<RailawareWidget *> *wlist = pn_high->appList();
        for (int i=0; i<wlist->size(); ++i) {
                RailawareWidget *w = wlist->at(i);
                if (!w) continue;
                if ( w->priority() < priority ) {
                        priority = w->priority();
                        rw = w;
                }
        }

        if (rw) assignProcessor(rw, pn_low);
}
