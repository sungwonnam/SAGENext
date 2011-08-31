#include "settingdialog.h"
#include <QSettings>

SettingDialog::SettingDialog(QSettings *settings, QWidget *parent /*0*/)
	: QDialog(parent), ui(new Ui::SettingDialogClass), settings(settings)
{
	ui->setupUi(this);

	setWindowFlags(Qt::Dialog);

	ui->filenameLabel->setText( settings->fileName() );
	ui->wallNameLineEdit->setText( settings->value("general/wallname", "").toString() );



	ui->wallIPLineEdit->setInputMask("000.000.000.000;_");
	ui->wallPortLineEdit->setInputMask("00000;_");
	ui->fsManagerIPLineEdit->setInputMask("000.000.000.000;_");
	ui->fsManagerPortLineEdit->setInputMask("00000;_");

	ui->wallIPLineEdit->setText( settings->value("general/wallip", "127.0.0.1").toString() );
	ui->wallPortLineEdit->setText( settings->value("general/wallport", 30003).toString());

	ui->fsManagerIPLineEdit->setText( settings->value("general/fsmip", "127.0.0.1").toString() );
	ui->fsManagerPortLineEdit->setText( settings->value("general/fsmport", 20002).toString());
	// fsManager stream base port ??


	ui->widthLineEdit->setText(settings->value("general/width", 0).toString());
	ui->heightLineEdit->setText(settings->value("general/height", 0).toString());
	ui->offsetx->setText(settings->value("general/offsetx", 0).toString());
	ui->offsety->setText(settings->value("general/offsety",0).toString());
	ui->fontpointsize->setText(settings->value("general/fontpointsize", 20).toString());


	if ( settings->value("system/openglviewport", false).toBool() )
		ui->openglviewportCheckBox->setCheckState(Qt::Checked);
	else
		ui->openglviewportCheckBox->setCheckState(Qt::Unchecked);


	ui->numNumaNodeLabel->setText(settings->value("system/numnumanodes", -1).toString());
	ui->cpuPerNumaLineEdit->setText(settings->value("system/cpupernumanode", -1).toString());
	ui->threadPerCpuLineEdit->setText(settings->value("system/threadpercpu", -1).toString());
	ui->swthreadPerCpuLineEdit->setText(settings->value("system/swthreadpercpu", -1).toString());

	if ( settings->value("system/numa").toBool() ) {
		ui->numaLable->setText("NUMA API available");
	}
	else {
		ui->numaLable->setText("NUMA API is not available");
	}

	if ( settings->value("system/resourcemonitor", false).toBool() ) {
		ui->resMonitorCheckBox->setCheckState(Qt::Checked);

		if (settings->value("system/scheduler", false).toBool()) {
			ui->schedulerCheckBox->setCheckState(Qt::Checked);
		}
		else {
			ui->schedulerCheckBox->setCheckState(Qt::Unchecked);
			ui->schedComboBox->setCurrentIndex(0);
		}
	}
	else {
		ui->resMonitorCheckBox->setCheckState(Qt::Unchecked);
		ui->schedulerCheckBox->setCheckState(Qt::Unchecked);
	}

	ui->schedFreqLineEdit->setText( settings->value("system/scheduler_freq", 1000).toString() );


	ui->mainonnodeLineEdit->setText( settings->value("system/mainonnode", 0).toString() );

	ui->recvWindowLineEdit->setText(settings->value("network/recvwindow", 16777216).toString());
	ui->sendWindowLineEdit->setText(settings->value("network/sendwindow", 65536).toString());
	ui->mtuLineEdit->setText(settings->value("network/mtu", 1450).toString());

/**
  system/numnumanodes
  system/cpupernumanode
  system/threadpercpu
  system/issmt
  system/ishyp
  **/

//	setAttribute(Qt::WA_DeleteOnClose, true);
}

SettingDialog::~SettingDialog()
{
	delete ui;
}

void SettingDialog::on_saveButton_clicked()
{
	Q_ASSERT(settings);
	/* save(overwrite) settings using QSetting */
	settings->setValue("general/wallname", ui->wallNameLineEdit->text());
	settings->setValue("general/wallip", ui->wallIPLineEdit->text() );
	settings->setValue("general/wallport", ui->wallPortLineEdit->text().toInt() );
	settings->setValue("general/fsmip", ui->fsManagerIPLineEdit->text() );
	settings->setValue("general/fsmport", ui->fsManagerPortLineEdit->text().toInt() );
	settings->setValue("general/fsmstreambaseport",  ui->fsManagerPortLineEdit->text().toInt() + 3);

	settings->setValue("general/width", ui->widthLineEdit->text().toInt());
	settings->setValue("general/height", ui->heightLineEdit->text().toInt());
	settings->setValue("general/offsetx", ui->offsetx->text().toInt());
	settings->setValue("general/offsety", ui->offsety->text().toInt());
	settings->setValue("general/fontpointsize", ui->fontpointsize->text().toInt());

	/* window frame margins */
	settings->setValue("general/framemarginleft", 3);
	settings->setValue("general/framemargintop", 3);
	settings->setValue("general/framemarginright", 3);
	settings->setValue("general/framemarginbottom", 3);

	/* graphics system */
//	settings->setValue("system/graphicssystem", ui->graphicssystemComboBox->currentText());
	if ( ui->openglviewportCheckBox->checkState() == Qt::Checked ) {
		settings->setValue("system/openglviewport", true);
	}
	else {
		settings->setValue("system/openglviewport", false);
	}
	settings->setValue("system/viewportupdatemode", ui->viewportUpdateComboBox->currentText());


	/* resource monitor & scheduling */
	settings->setValue("system/resourcemonitor", false);
	settings->setValue("system/scheduler", false);
	if ( ui->schedulerCheckBox->isChecked() ) {
		settings->setValue("system/scheduler", true);
		settings->setValue("system/resourcemonitor", true);
		settings->setValue("system/scheduler_type", ui->schedComboBox->currentText() );
		settings->setValue("system/scheduler_freq", ui->schedFreqLineEdit->text().toInt());
	}
	else if (ui->resMonitorCheckBox->isChecked()) {
		settings->setValue("system/scheduler", false);
		settings->setValue("system/resourcemonitor", true);
	}
	if ( ! settings->value("system/resourcemonitor").toBool() ) {
		settings->setValue("system/scheduler", false);
	}

	// will receiving thread affinity affect sail thread affinity ?
        settings->setValue("system/sailaffinity", false);

	// thread affinity based on NIC irq processing core
	settings->setValue("system/threadaffinityonirq", false);

	settings->setValue("system/mainonnode", ui->mainonnodeLineEdit->text());

	// value from numa_num_configured_node()
//	settings->setValue("system/numnumanodes", ui->numNumaNodeLineEdit->text().toInt());
	settings->setValue("system/cpupernumanode", ui->cpuPerNumaLineEdit->text().toInt());
	settings->setValue("system/threadpercpu", ui->threadPerCpuLineEdit->text().toInt());
	settings->setValue("system/swthreadpercpu", ui->swthreadPerCpuLineEdit->text().toInt());

	int numCpus = settings->value("system/numnumanodes").toInt() *
				   settings->value("system/cpupernumanode").toInt() *
				   settings->value("system/threadpercpu").toInt() *
				   settings->value("system/swthreadpercpu").toInt();

	settings->setValue("system/numcpus", numCpus);
        settings->setValue("system/refreshintervalmsec", 1000); // info update interval (QTimer interval)



	/* network parameters */
	settings->setValue("network/recvwindow", ui->recvWindowLineEdit->text().toInt());
	settings->setValue("network/sendwindow", ui->sendWindowLineEdit->text().toInt());
	settings->setValue("network/mtu", ui->mtuLineEdit->text().toInt());

	emit settingSaved();
	close();
}

void SettingDialog::on_loadButton_clicked()
{
	/* MainWindow tries to open ini file first */

	/* load settings from QSetting */
//	ui->orgNameLineEdit->setText( settings->organizationName() );
//	ui->wallNameLineEdit->setText( settings->applicationName() );
}


void SettingDialog::on_schedComboBox_currentIndexChanged(QString itemname)
{
	if ( QString::compare(itemname, "None", Qt::CaseInsensitive) ) {

	}
	else {
		settings->setValue("system/scheduler", true);
		ui->resMonitorCheckBox->setChecked(true);
	}
}

void SettingDialog::on_schedulerCheckBox_stateChanged(int state)
{
	if ( state == Qt::Checked ) {
		ui->resMonitorCheckBox->setChecked(true);
		ui->schedComboBox->setEnabled(true);
	}
	else {
		ui->schedComboBox->setCurrentIndex(0);
		ui->schedComboBox->setDisabled(true);
	}
}

void SettingDialog::on_resMonitorCheckBox_stateChanged(int state)
{
	if ( state == Qt::Unchecked ) {
		ui->schedulerCheckBox->setCheckState(Qt::Unchecked);
	}
}
