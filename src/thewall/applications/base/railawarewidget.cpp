#include "railawarewidget.h"
#include "affinityinfo.h"
#include "affinitycontroldialog.h"
#include "appinfo.h"
#include "perfmonitor.h"

#include "../../system/resourcemonitor.h"
#include "../../system/sagenextscheduler.h"


SN_RailawareWidget::SN_RailawareWidget() :
    affCtrlDialog(0),
    _affCtrlAction(0),
    _widgetClosed(false),
    _scheduled(false)
{
//	setWidgetType(SN_BaseWidget::Widget_RealTime);
	setCacheMode(QGraphicsItem::NoCache);
}

SN_RailawareWidget::SN_RailawareWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
    : SN_BaseWidget(globalappid, s, parent, wflags)
    , affCtrlDialog(0)
    , _affCtrlAction(0)
    , _widgetClosed(false)
    , _scheduled(false)
{
//	setWidgetType(SN_BaseWidget::Widget_RealTime);

	failToSchedule = 0;

	/* railaware widget is streaming widget, so turn off cache */
	setCacheMode(QGraphicsItem::NoCache);
	//	setCacheMode(QGraphicsItem::ItemCoordinateCache);
	//	setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	//	setBoundingRegionGranularity(0.25);


	//	qDebug() << "affInfo" << affInfo;
	//	qDebug() << "gaid" << globalappid;
	//	qDebug() << "rail on?" << s->value("system/rail").toBool();

	if (s && s->value("system/resourcemonitor").toBool()) {
		//		qDebug() << "RailawareWidget() : creating affinity instance";
		createAffInstances();
	}
}



int SN_RailawareWidget::setQuality(qreal newQuality) {

	//qDebug() << _globalAppId << "railwarewidget::setQuality" << newQuality;

	if ( newQuality > 1.0 ) {
		_quality = 1.0;
	}
	else if ( newQuality <= 0.0 ) {
		_quality = 0.1;
	}
	else {
		_quality = newQuality;
	}

	if (_perfMon) {
		// for now frame rate is the quality metric
		return _perfMon->setAdjustedFps(_perfMon->getExpetctedFps() * _quality);
	}
	return -1;
}

qreal SN_RailawareWidget::observedQuality() {
	if (_perfMon) {
		//qDebug() << _perfMon->getCurrRecvFps() << _perfMon->getExpetctedFps() << _perfMon->getCurrRecvFps() / _perfMon->getExpetctedFps();
		return _perfMon->getCurrRecvFps() / _perfMon->getExpetctedFps(); // frame rate for now
	}
	else return -1;
}

qreal SN_RailawareWidget::observedQualityAdjusted() {
	return _perfMon->getCurrRecvFps() / _perfMon->getAdjustedFps();
}




void SN_RailawareWidget::createAffInstances()
{
	if (!_affInfo)
		_affInfo = new AffinityInfo(this);

	if (!_affCtrlAction) {
		_affCtrlAction = new QAction("Affinity Control", this);
		_affCtrlAction->setEnabled(false);
		_contextMenu->addAction(_affCtrlAction);
		connect(_affCtrlAction, SIGNAL(triggered()), this, SLOT(showAffCtrlDialog()));
	}

	//	Q_ASSERT(_affInfo);
	//	if ( rMonitor ) {
	//		if ( connect(_affInfo, SIGNAL(cpuOfMineChanged(RailawareWidget *,int,int)), rMonitor, SLOT(updateAffInfo(RailawareWidget *,int,int))) ) {
	//		}
	//		else {
	//			qCritical("RailawareWidget::%s() : connecting AffinityInfo::affInfoChanged() to ResourceMonitor::updateAffInfo() failed", __FUNCTION__);
	//		}
	//	}
}


void SN_RailawareWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if ( _affInfo && _affCtrlAction ) {
		_affCtrlAction->setEnabled(true);
	}
	//	BaseWidget::contextMenuEvent(event);
	scene()->clearSelection();
	setSelected(true);

	//	_contextMenu->exec(event->screenPos());
	_contextMenu->popup(event->screenPos());
}


void SN_RailawareWidget::showAffCtrlDialog() {
	if ( affCtrlDialog ) {
		affCtrlDialog->updateInfo();
		affCtrlDialog->show();
		return;
	}

	Q_ASSERT(_affInfo);
	Q_ASSERT(_settings);
	Q_ASSERT(globalAppId() > 0);
	/* will modify mask through affInfo pointer and sets the flag */
	affCtrlDialog = new AffinityControlDialog(_globalAppId, _affInfo, _settings);
	affCtrlDialog->show();
}



SN_RailawareWidget::~SN_RailawareWidget()
{
        if (_affInfo) {
			//
			// todo :  explain why
			//
			_affInfo->disconnect();
			delete _affInfo;
		}
        if (affCtrlDialog) delete affCtrlDialog;

        qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}
