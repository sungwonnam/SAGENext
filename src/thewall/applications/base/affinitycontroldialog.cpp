#include "affinitycontroldialog.h"
#include "ui_affinitycontroldialog.h"
#include "affinityinfo.h"

#include <QSettings>

AffinityControlDialog::AffinityControlDialog(const quint64 gid, AffinityInfo *ai, const QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AffinityControlDialog)
    , settings(s)
    , globalAppId(gid)
{
	ui->setupUi(this);
	affInfo = ai;

	Q_ASSERT(affInfo);

	/* init */
	ui->appidLabel->setText(QString::number(gid)); // global app id
	ui->numNodesLabel->setText(settings->value("system/numnumanodes").toString()); // total number of numa nodes in the system
	ui->numCpuLabel->setText(settings->value("system/numcpus").toString()); // total number of cpus in the system
//	ui->maincpuLabel->setText("-1");
	ui->cpuOfMineLabel->setText(QString::number(affInfo->cpuOfMine()));

	/* define valid format for values */
	/*
	const QString nodemask( settings->value("system/numnumanodes").toInt(), 'B');
	const QString cpumask( settings->value("system/numcpus").toInt(), 'B');
	ui->newCpuAffinity->setInputMask( cpumask );
	ui->newMembind->setInputMask(nodemask);
	ui->newNodeAffinity->setInputMask(nodemask);

	ui->streamerCpuMaskLineEdit->setInputMask(cpumask);
	*/

	/* update info from affInfo */
	updateInfo();
}

AffinityControlDialog::~AffinityControlDialog() {
	if (ui) delete ui;
}

void AffinityControlDialog::updateInfo() {
//	ui->maincpuLabel->setText( QString::number(affInfo->getCpuOfMain()) );
	ui->cpuOfMineLabel->setText(QString::number(affInfo->cpuOfMine()));
	ui->nodeAffinityLabel->setText( QString(affInfo->getNodeMask()) );
	ui->cpuAffinityLabel->setText(QString(affInfo->getCpuMask()));
	ui->membindLabel->setText(QString(affInfo->getMemMask()));

	ui->streamerCpuMaskLabel->setText(QString(affInfo->getStreamerCpuMask()));
}

void AffinityControlDialog::on_applyButton_clicked()
{
	// by default, line edit contains empty string
	QString nodemask(ui->newNodeAffinity->text());
	QString memmask(ui->newMembind->text());
	QString cpumask(ui->newCpuAffinity->text());
	QString streamerCpuMask(ui->streamerCpuMaskLineEdit->text());

//	qDebug("AffinityControlDialog::%s() : Applying, nodemaks %s, memmask %s, cpumask %s", __FUNCTION__, qPrintable(nodemask), qPrintable(memmask), qPrintable(cpumask));
	Q_ASSERT(affInfo);

	/*
	bool isChanged = false;
	if ( ! nodemask.isEmpty() && nodemask.contains('1')) {
		affInfo->clearReadyBit(AffinityInfo::NODE_MASK);
		affInfo->setReadyBit(AffinityInfo::NODE_MASK, qPrintable(nodemask), nodemask.size()); // QString::size() returns # of character excluding '\0'
		isChanged = true;
	}
	if ( ! memmask.isEmpty() && memmask.contains('1')) {
		affInfo->clearReadyBit(AffinityInfo::MEM_MASK);
		affInfo->setReadyBit(AffinityInfo::MEM_MASK, qPrintable(memmask), memmask.size());
		isChanged = true;
	}
	if ( ! cpumask.isEmpty()  &&  cpumask.contains('1')) {
		affInfo->clearReadyBit(AffinityInfo::CPU_MASK);
		affInfo->setReadyBit(AffinityInfo::CPU_MASK, qPrintable(cpumask), cpumask.size());
		isChanged = true;
	}
	if ( !streamerCpuMask.isEmpty()  &&  streamerCpuMask.contains('1') ) {
		affInfo->setSageStreamerAffinity(qPrintable(streamerCpuMask));
	}
	affInfo->setFlag(); // will trigger a thread to apply new settings
	*/

//	bool isChanged = false;
	affInfo->setReadyString(nodemask, memmask, cpumask); // will set flag true
	/*
	if ( ! nodemask.isEmpty() ) {
		affInfo->setReadyString(AffinityInfo::NODE_MASK, nodemask);
		isChanged = true;
	}
	if ( ! memmask.isEmpty() ) {
		affInfo->setReadyString(AffinityInfo::MEM_MASK, memmask);
		isChanged = true;
	}
	if ( ! cpumask.isEmpty() ) {
		affInfo->setReadyString(AffinityInfo::CPU_MASK, cpumask);
		isChanged = true;
	}
	*/
//	if (isChanged)
//		affInfo->setFlag();
//	accept();

}

void AffinityControlDialog::on_closeButton_clicked()
{
	accept();
}
