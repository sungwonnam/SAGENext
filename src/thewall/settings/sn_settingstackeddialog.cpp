#include "settings/sn_settingstackeddialog.h"
#include "ui_sn_settingstackeddialog.h"

#include "ui_sn_generalsettingdialog.h"
#include "ui_sn_systemsettingdialog.h"
#include "ui_sn_graphicssettingdialog.h"
#include "ui_sn_guisettingdialog.h"
#include "ui_sn_screenlayoutdialog.h"

#include <QtCore>


SN_GeneralSettingDialog::SN_GeneralSettingDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SN_GeneralSettingDialog)
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

void SN_GeneralSettingDialog::accept() {
	Q_ASSERT(_settings);
	/* save(overwrite) settings using QSetting */
//	_settings->setValue("general/wallname", ui->wallNameLineEdit->text());
	_settings->setValue("general/wallip", ui->wallIPLineEdit->text() );
	_settings->setValue("general/wallport", ui->wallPortLineEdit->text().toInt() );

	/**
	  This is static
	  */
	_settings->setValue("general/fileserverport", 46000);

	_settings->setValue("general/fsmip", ui->fsManagerIPLineEdit->text() );
	_settings->setValue("general/fsmport", ui->fsManagerPortLineEdit->text().toInt() );

	/**
	  This is static
	  */
	_settings->setValue("general/fsmstreambaseport",  ui->fsManagerPortLineEdit->text().toInt() + 3);

	_settings->setValue("general/width", ui->widthLineEdit->text().toInt());
	_settings->setValue("general/height", ui->heightLineEdit->text().toInt());
	_settings->setValue("general/offsetx", ui->offsetx->text().toInt());
	_settings->setValue("general/offsety", ui->offsety->text().toInt());
//	_settings->setValue("general/fontpointsize", ui->fontpointsize->text().toInt());

    //
    // set the mplayer arguments here
    //
    _settings->setValue("general/mplayercmdargs", QString("-vo sage -ao pulse -loop 0 -sws 4 -framedrop -quiet"));
}

SN_GeneralSettingDialog::~SN_GeneralSettingDialog() { delete ui;}






/* System */
SN_SystemSettingDialog::SN_SystemSettingDialog(QSettings *s, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SN_SystemSettingDialog)
	, _settings(s)
{
	ui->setupUi(this);

	if ( _settings->value("system/numa").toBool())
		ui->isNumaAvailLabel->setText("Yes");
	else
		ui->isNumaAvailLabel->setText("No");
	ui->numNumaNodesLabel->setText(_settings->value("system/numnumanodes", -1).toString());

	ui->numCpuPerNumaLineEdit->setText(_settings->value("system/cpupernumanode", -1).toString());
	ui->numHWThreadLineEdit->setText(_settings->value("system/threadpercpu", -1).toString());
	ui->numSWThreadLineEdit->setText(_settings->value("system/swthreadpercpu", -1).toString());

	if (_settings->value("system/resourcemonitor", false).toBool()) {
		ui->rmonitorCheckBox->setCheckState(Qt::Checked);

		if (_settings->value("system/resourcemonitorwidget", false).toBool()) {
			ui->rmonitorWidgetCheckBox->setCheckState(Qt::Checked);
		}
		else {
			ui->rmonitorWidgetCheckBox->setCheckState(Qt::Unchecked);
		}

		if (_settings->value("system/prioritygrid", false).toBool()) {
			ui->pGridCheckBox->setCheckState(Qt::Checked);
		}
		else {
			ui->pGridCheckBox->setCheckState(Qt::Unchecked);
		}

        ui->pgrid_row_spinBox->setValue( _settings->value("system/prioritygridrow", 3).toInt());
        ui->pgrid_col_spinBox->setValue( _settings->value("system/prioritygridcol", 4).toInt());


		if (_settings->value("system/scheduler", false).toBool()) {
			ui->schedulerCheckBox->setCheckState(Qt::Checked);
			ui->schedFreqLineEdit->setEnabled(true);
		}
		else {
			ui->schedulerCheckBox->setCheckState(Qt::Unchecked);
			ui->schedFreqLineEdit->setDisabled(true);
		}
	}
	else {
		ui->rmonitorCheckBox->setCheckState(Qt::Unchecked);
		ui->pGridCheckBox->setCheckState(Qt::Unchecked);
		ui->schedulerCheckBox->setCheckState(Qt::Unchecked);
	}

	ui->schedFreqLineEdit->setText(_settings->value("system/scheduler_freq", 100).toString());
}

void SN_SystemSettingDialog::on_schedulerCheckBox_stateChanged(int state) {
	if (state == Qt::Checked) {
		ui->rmonitorCheckBox->setChecked(true);
//		ui->schedFreqLineEdit->setEnabled(true);
	}
	else {
//		ui->schedFreqLineEdit->setDisabled(true);
	}
}

void SN_SystemSettingDialog::on_rmonitorCheckBox_stateChanged(int state) {
	if (state == Qt::Unchecked) {
		ui->pGridCheckBox->setCheckState(Qt::Unchecked);
		ui->schedulerCheckBox->setCheckState(Qt::Unchecked);
		ui->rmonitorWidgetCheckBox->setCheckState(Qt::Unchecked);
        ui->schedFreqLineEdit->setEnabled(false);
	}
    else if (state == Qt::Checked) {
        ui->schedFreqLineEdit->setEnabled(true);
    }
}


void SN_SystemSettingDialog::on_pGridCheckBox_stateChanged(int state)
{
    if (state == Qt::Checked) {
        ui->pgrid_col_spinBox->setEnabled(true);
        ui->pgrid_row_spinBox->setEnabled(true);
    }
    else {
        ui->pgrid_col_spinBox->setDisabled(true);
        ui->pgrid_row_spinBox->setDisabled(true);
    }
}

void SN_SystemSettingDialog::accept() {
	_settings->setValue("system/cpupernumanode", ui->numCpuPerNumaLineEdit->text().toInt());
	_settings->setValue("system/threadpercpu", ui->numHWThreadLineEdit->text().toInt());
	_settings->setValue("system/swthreadpercpu", ui->numSWThreadLineEdit->text().toInt());

	int totalCpus = _settings->value("system/numnumanodes").toInt()
	        * _settings->value("system/cpupernumanode").toInt()
	        * _settings->value("system/threadpercpu").toInt()
	        * _settings->value("system/swthreadpercpu").toInt();
	_settings->setValue("system/numcpus", totalCpus);


	// this is used in sagePixelReceiver
	_settings->setValue("system/sailaffinity", false);


    //
    // enable/disable resource monitor
    //
	_settings->setValue("system/resourcemonitor", ui->rmonitorCheckBox->isChecked());

    //
    // rmonitor and scheduling frequency in msec
    //
    _settings->setValue("system/scheduler_freq", ui->schedFreqLineEdit->text().toInt());

    //
    // enable/disable resource monitor widget for real-time monitoring
    //
	_settings->setValue("system/resourcemonitorwidget", ui->rmonitorWidgetCheckBox->isChecked());

    //
    // enable/disable priority grid for setting priority based on wall usage
    //
	_settings->setValue("system/prioritygrid", ui->pGridCheckBox->isChecked());

    _settings->setValue("system/prioritygridrow", ui->pgrid_row_spinBox->value());
    _settings->setValue("system/prioritygridcol", ui->pgrid_col_spinBox->value());

    //
    // enable/disable scheduler
    //
	_settings->setValue("system/scheduler", ui->schedulerCheckBox->isChecked());


    if (ui->schedulerCheckBox->isChecked()) {
        //
        // scheduler type
        //
        //	_settings->setValue("system/scheduler_type", "SelfAdjusting");
        _settings->setValue("system/scheduler_type", "ProportionalShare");
    }
    else {
        _settings->setValue("system/scheduler_type", "");
    }


/*
	if ( ui->schedulerCheckBox->isChecked() ) {
		_settings->setValue("system/scheduler", true);
		_settings->setValue("system/resourcemonitor", true);
//		_settings->setValue("system/scheduler_type", ui->schedulerComboBox->currentText() );

	}
	else if (ui->rmonitorCheckBox->isChecked()) {
		_settings->setValue("system/scheduler", false);
		_settings->setValue("system/resourcemonitor", true);
	}


	if ( ! _settings->value("system/resourcemonitor").toBool() ) {
		_settings->setValue("system/scheduler", false);
		_settings->setValue("system/prioritygrid", false);
		_settings->setValue("system/resourcemonitorwidget", false);
	}
    */
}

SN_SystemSettingDialog::~SN_SystemSettingDialog() {delete ui;}



/*
  Graphics
*/
SN_GraphicsSettingDialog::SN_GraphicsSettingDialog(QSettings *s, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SN_GraphicsSettingDialog)
	, _settings(s)
    , _layoutMap(screenLayout)
    , _numScreens(0)
{
	ui->setupUi(this);

	connect(ui->setLayoutButton, SIGNAL(clicked()), this, SLOT(createLayoutGrid()));

	//
	// multiple screens
	//
	_numScreens = _settings->value("graphics/screencount", 1).toInt();
	ui->numScreenLabel->setText(QString::number(_numScreens));

	if (_settings->value("graphics/isxinerama", false).toBool()) {
		ui->isXinerama->setChecked(true);
		ui->dimx->setText("1");
		ui->dimy->setText("1");
	}
	else {
		ui->isXinerama->setChecked(false);
		ui->dimx->setText(_settings->value("graphics/screencountx", 1).toString());
		ui->dimy->setText(_settings->value("graphics/screencounty", 1).toString());
	}

	ui->bezelSize->setText(_settings->value("graphics/bezelsize", 0).toString());


	if ( _settings->value("graphics/openglviewport", true).toBool() )
		ui->openglviewportCheckBox->setCheckState(Qt::Checked);
	else
		ui->openglviewportCheckBox->setCheckState(Qt::Unchecked);

	int idx = ui->viewportUpdateComboBox->findText(_settings->value("graphics/viewportupdatemode", "").toString());
	if (idx != -1) {
		ui->viewportUpdateComboBox->setCurrentIndex(idx);
	}
}

void SN_GraphicsSettingDialog::createLayoutGrid() {
	bool ok = false;
	int x = ui->dimx->text().toInt(&ok);
	if (!ok) return;
	int y = ui->dimy->text().toInt(&ok);
	if (!ok) return;

//	if ((x*y) <= 1) return;

//	if ( (x * y) !=  _numScreens) {
//		QMessageBox::critical(this, "Error", "Wrong Screen Dimension"); // application modal
//		return;
//	}

	//
	// create Application Modal dialog with grid layout
	//
	SN_ScreenLayoutDialog slDialog(_settings, x, y, _layoutMap, this);
	slDialog.exec(); // shows the dialog as a modal dialog
}

void SN_GraphicsSettingDialog::accept() {
	/* graphics system */
//	settings->setValue("system/graphicssystem", ui->graphicssystemComboBox->currentText());

	if (ui->isXinerama->isChecked())
		_settings->setValue("graphics/isxinerama", true);
	else
		_settings->setValue("graphics/isxinerama", false);

	_settings->setValue("graphics/screencountx", ui->dimx->text().toInt());
	_settings->setValue("graphics/screencounty", ui->dimy->text().toInt());

	_settings->setValue("graphics/bezelsize", ui->bezelSize->text().toInt());

	if ( ui->openglviewportCheckBox->checkState() == Qt::Checked ) {
		_settings->setValue("graphics/openglviewport", true);
	}
	else {
		_settings->setValue("graphics/openglviewport", false);
	}
	_settings->setValue("graphics/viewportupdatemode", ui->viewportUpdateComboBox->currentText());
}


void SN_GraphicsSettingDialog::on_openglviewportCheckBox_stateChanged(int)
{
	if ( ui->openglviewportCheckBox->isChecked() ) {
		_settings->setValue("graphics/openglviewport", true);
		ui->viewportUpdateComboBox->setEnabled(false);
		ui->viewportUpdateComboBox->setCurrentIndex( ui->viewportUpdateComboBox->findText("full") );
	}
	else {
		ui->viewportUpdateComboBox->setEnabled(true);
		_settings->setValue("graphics/openglviewport", false);
	}
}

SN_GraphicsSettingDialog::~SN_GraphicsSettingDialog() {delete ui;}





SN_ScreenLayoutDialog::SN_ScreenLayoutDialog(QSettings *s, int dimx, int dimy, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SN_ScreenLayoutDialog)
    , _settings(s)
    , _dimx(dimx)
    , _dimy(dimy)
    , _layoutMap(screenLayout)
{
	ui->setupUi(this);

	int row = 0;
	int col = 0;
	for (int i=0; i<dimx*dimy; ++i) {

		if (col == dimx) {
			col = 0;
			row++;
		}

		if (row!=0 || col!=0)
			ui->gridLayout->addWidget(createScreen(row, col), row, col);
		else {
			int screenid = _layoutMap->value( QPair<int,int>(0, 0) );
			ui->firstLineEdit->setText(QString::number(screenid));
		}

		col++;
	}

	QDialogButtonBox *buttonbox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	connect(buttonbox, SIGNAL(accepted()), this, SLOT(saveToSettings()));
	connect(buttonbox, SIGNAL(rejected()), this, SLOT(close()));
	ui->gridLayout->addWidget(buttonbox, row+1, 0, 1, dimx, Qt::AlignRight);

	adjustSize();
}

QFrame * SN_ScreenLayoutDialog::createScreen(int row, int col) {
	QFrame *frame = new QFrame(this);
	frame->setLineWidth(3);
	frame->setFrameStyle(QFrame::Box);

	QLabel *label = new QLabel("Screen " , frame);
	QLineEdit *le = new QLineEdit(frame);
	le->setAlignment(Qt::AlignCenter);

	int screenid = _layoutMap->value( QPair<int,int>(col, row) ); // col = x, row = y
	le->setText(QString::number(screenid));

	QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	QHBoxLayout *hlayout = new QHBoxLayout(frame);
	hlayout->addWidget(label, 2, Qt::AlignRight);
	hlayout->addWidget(le, 1, Qt::AlignLeft);
	hlayout->addSpacerItem(spacer);
    hlayout->setStretch(2, 1);

	return frame;
}
void SN_ScreenLayoutDialog::saveToSettings() {
	// overwrite
	QFile f(QDir::homePath() + "/.sagenext/screenlayout");
	f.open(QIODevice::WriteOnly);
	QDataStream out(&f);

	_layoutMap->clear();

	int row = 0;
	int col = 0;
	for (int i=0; i<_dimx*_dimy; ++i) {
		if (col == _dimx) {
			col = 0;
			row++;
		}
		QFrame *f = static_cast<QFrame *>(ui->gridLayout->itemAtPosition(row, col)->widget());
		Q_ASSERT(f);
		QHBoxLayout *hl = static_cast<QHBoxLayout *>(f->layout());
		Q_ASSERT(hl);
		QLineEdit *l = static_cast<QLineEdit *>(hl->itemAt(1)->widget());
		Q_ASSERT(l);

//		qDebug() << col << row << l->text();
		_layoutMap->insert(QPair<int,int>(col, row) , l->text().toInt());
		out << col << row << l->text().toInt();

		col++;
	}
	f.close();
	accept();
}

SN_ScreenLayoutDialog::~SN_ScreenLayoutDialog() {
	delete ui;
}




/* GUI */
SN_GuiSettingDialog::SN_GuiSettingDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SN_GuiSettingDialog)
    , _settings(s)
{
	ui->setupUi(this);

    /*!
     * The pixel width of icons on the scene (buttons int the SN_LayoutWidget)
     */
	ui->iconWidth->setText(_settings->value("gui/iconwidth", 32).toString());

	ui->fontSizeLineEdit->setInputMask("900"); // 3 ascii digit. first digit is required
	ui->fontSizeLineEdit->setText(_settings->value("gui/fontpointsize", 20).toString());

	ui->pointerFontSizeLineEdit->setInputMask("900");
    /*!
     * The size of the font used for shared pointers (SN_PolygonArrowPointer)
     */
	ui->pointerFontSizeLineEdit->setText(_settings->value("gui/pointerfontsize", 20).toString());

	ui->framemargin->setText(_settings->value("gui/framemargin", 8).toString());

    /*!
     * Pixel width of a media thumbnail
     */
	ui->thumbnailsize->setText(_settings->value("gui/mediathumbnailwidth", 128).toString());

    ui->imgScalingAmount->setText(_settings->value("gui/imgscalingamount", 32).toString());
}
void SN_GuiSettingDialog::accept() {
	_settings->setValue("gui/iconwidth", ui->iconWidth->text().toInt());
	_settings->setValue("gui/fontpointsize", ui->fontSizeLineEdit->text().toInt());
	_settings->setValue("gui/pointerfontsize", ui->pointerFontSizeLineEdit->text().toInt());
	/* window frame margins */
	_settings->setValue("gui/framemargin", ui->framemargin->text().toInt());
	_settings->setValue("gui/mediathumbnailwidth", ui->thumbnailsize->text().toInt());
    _settings->setValue("gui/imgscalingamount", ui->imgScalingAmount->text().toInt());

    /*!
     * put this manually here temporarily
     * It would be better to increase # thumanails when user resize the media browser
     */
//    _settings->setValue("gui/numthumbnailx", 8);
//    _settings->setValue("gui/numthumbnaily", 4);

}
SN_GuiSettingDialog::~SN_GuiSettingDialog() {delete ui;}




SN_SettingStackedDialog::SN_SettingStackedDialog(QSettings *s, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SN_SettingStackedDialog)
    , _settings(s)
{
    ui->setupUi(this);
    
    setWindowTitle(tr("SAGENext Configuration [ ~/.sagenext/sagenext.ini ]"));
	
	connect(ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changeStackedWidget(QListWidgetItem*,QListWidgetItem*)));
	
	/*
      stacked widget for each setting page
	  Ownership of widget is passed on to the QStackedWidget
      */
    ui->stackedWidget->addWidget(new SN_GeneralSettingDialog(_settings));
	ui->stackedWidget->addWidget(new SN_GraphicsSettingDialog(_settings, screenLayout));
	ui->stackedWidget->addWidget(new SN_GuiSettingDialog(_settings));
	ui->stackedWidget->addWidget(new SN_SystemSettingDialog(_settings));
    
    /*
      Fill listWidget
      */
	ui->listWidget->setMovement(QListView::Static);
	ui->listWidget->setViewMode(QListView::ListMode);
	
    QListWidgetItem *item = new QListWidgetItem("general");
	item->setText("General");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	ui->listWidget->addItem(item);

	item = new QListWidgetItem("graphics");
	item->setText("Graphics");
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	ui->listWidget->addItem(item);

	item = new QListWidgetItem("gui");
	item->setText("GUI related");
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	ui->listWidget->addItem(item);

	item = new QListWidgetItem("system");
	item->setText("System");
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	ui->listWidget->addItem(item);
	
	/**
	  This must be called AFTER ui->stackedWidget->addWidget
	  */
	ui->listWidget->setCurrentRow(0); // will emit currentItemChanged which is connected to changeStackedWidget
}

SN_SettingStackedDialog::~SN_SettingStackedDialog()
{
	ui->listWidget->clear(); // will delete all the items
    delete ui;
}

void SN_SettingStackedDialog::changeStackedWidget(QListWidgetItem *current, QListWidgetItem *previous) {
//	QDialog *dialog = static_cast<QDialog *>(ui->stackedWidget->widget(ui->listWidget->row(previous)));
//	dialog->accept();
	
    if (!current) current = previous;
    ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
}

void SN_SettingStackedDialog::on_buttonBox_accepted()
{
	QDialog *dialog = 0;
	for (int i=0; i<ui->stackedWidget->count(); ++i) {
		dialog  = static_cast<QDialog *>(ui->stackedWidget->widget(i));
		dialog->accept();
	}

	/* network parameters used by fsManagerMsgThread
	  let's put these here for now
	  */
	_settings->setValue("network/recvwindow", 4 * 1048576);
	_settings->setValue("network/sendwindow", 1048576);
	_settings->setValue("network/mtu", 8800);

//	_settings->setValue("misc/printperfdataattheend", false);

    accept();
}

void SN_SettingStackedDialog::on_buttonBox_rejected()
{
	::exit(1);
}




