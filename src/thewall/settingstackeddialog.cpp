#include "settingstackeddialog.h"
#include "ui_settingstackeddialog.h"

#include "ui_generalsettingdialog.h"
#include "ui_systemsettingdialog.h"
#include "ui_graphicssettingdialog.h"

#include <QSettings>
#include <QtCore>


GeneralSettingDialog::GeneralSettingDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GeneralSettingDialog)
    , _settings(s)
{
    ui->setupUi(this);
	
	// fill the GUI components with value read from _settings
	ui->wallIPLineEdit->setInputMask("000.000.000.000;_");
	ui->wallPortLineEdit->setInputMask("00000;_");
	ui->fsManagerIPLineEdit->setInputMask("000.000.000.000;_");
	ui->fsManagerPortLineEdit->setInputMask("00000;_");

	ui->wallIPLineEdit->setText( _settings->value("general/wallip", "127.0.0.1").toString() );
	ui->wallPortLineEdit->setText( _settings->value("general/wallport", 30003).toString());

	ui->fsManagerIPLineEdit->setText( _settings->value("general/fsmip", "127.0.0.1").toString() );
	ui->fsManagerPortLineEdit->setText( _settings->value("general/fsmport", 20002).toString());

	ui->widthLineEdit->setText(_settings->value("general/width", 0).toString());
	ui->heightLineEdit->setText(_settings->value("general/height", 0).toString());
	ui->offsetx->setText(_settings->value("general/offsetx", 0).toString());
	ui->offsety->setText(_settings->value("general/offsety",0).toString());
}

void GeneralSettingDialog::accept() {
	Q_ASSERT(_settings);
	/* save(overwrite) settings using QSetting */
//	_settings->setValue("general/wallname", ui->wallNameLineEdit->text());
	_settings->setValue("general/wallip", ui->wallIPLineEdit->text() );
	_settings->setValue("general/wallport", ui->wallPortLineEdit->text().toInt() );
	_settings->setValue("general/fsmip", ui->fsManagerIPLineEdit->text() );
	_settings->setValue("general/fsmport", ui->fsManagerPortLineEdit->text().toInt() );
	_settings->setValue("general/fsmstreambaseport",  ui->fsManagerPortLineEdit->text().toInt() + 3);

	_settings->setValue("general/width", ui->widthLineEdit->text().toInt());
	_settings->setValue("general/height", ui->heightLineEdit->text().toInt());
	_settings->setValue("general/offsetx", ui->offsetx->text().toInt());
	_settings->setValue("general/offsety", ui->offsety->text().toInt());
//	_settings->setValue("general/fontpointsize", ui->fontpointsize->text().toInt());
}

GeneralSettingDialog::~GeneralSettingDialog() { delete ui;}

SystemSettingDialog::SystemSettingDialog(QSettings *s, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SystemSettingDialog)
	, _settings(s)
{
	ui->setupUi(this);
}

void SystemSettingDialog::accept() {
	
}

SystemSettingDialog::~SystemSettingDialog() {delete ui;}

GraphicsSettingDialog::GraphicsSettingDialog(QSettings *s, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::GraphicsSettingDialog)
	, _settings(s)
{
	ui->setupUi(this);
}

void GraphicsSettingDialog::accept() {
	
}

GraphicsSettingDialog::~GraphicsSettingDialog() {delete ui;}






SettingStackedDialog::SettingStackedDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingStackedDialog)
    , _settings(s)
{
    ui->setupUi(this);
    
    setWindowTitle(tr("Config Dialog"));
	
	connect(ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changeStackedWidget(QListWidgetItem*,QListWidgetItem*)));
	
	/*
      stacked widget for each setting page
      */
    ui->stackedWidget->addWidget(new GeneralSettingDialog(_settings));
	ui->stackedWidget->addWidget(new GraphicsSettingDialog(_settings));
	ui->stackedWidget->addWidget(new SystemSettingDialog(_settings));
    
    /*
      Fill listWidget
      */
	ui->listWidget->setMovement(QListView::Static);
	ui->listWidget->setViewMode(QListView::ListMode);
	
    QListWidgetItem *item = new QListWidgetItem("general");
	item->setText("General");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	ui->listWidget->addItem(item);
	
	/**
	  This must be called AFTER ui->stackedWidget->addWidget
	  */
	ui->listWidget->setCurrentRow(0); // will emit currentItemChanged which is connected to changeStackedWidget
}

SettingStackedDialog::~SettingStackedDialog()
{
	ui->listWidget->clear(); // will delete all the items
    delete ui;
}

void SettingStackedDialog::changeStackedWidget(QListWidgetItem *current, QListWidgetItem *previous) {
	QDialog *dialog = static_cast<QDialog *>(ui->stackedWidget->widget(ui->listWidget->row(previous)));
	dialog->accept();
	
    if (!current) current = previous;
    ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
}

void SettingStackedDialog::on_buttonBox_accepted()
{
	QDialog *dialog = 0;
	for (int i=0; i<ui->stackedWidget->count(); ++i) {
		dialog  = static_cast<QDialog *>(ui->stackedWidget->widget(i));
		dialog->accept();
	}
}

void SettingStackedDialog::on_buttonBox_rejected()
{
	::exit(1);
}
