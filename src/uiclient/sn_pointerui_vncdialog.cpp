#include "sn_pointerui_vncdialog.h"
#include "ui_sn_pointerui_vncdialog.h"

SN_PointerUI_VncDialog::SN_PointerUI_VncDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SN_PointerUI_VncDialog)
{
	ui->setupUi(this);

	ui->lbl_username->hide();
	ui->le_username->hide();
	ui->lbl_password->setText("VNC password");
	ui->le_password->setEchoMode(QLineEdit::Password);

	ui->lbl_info->hide();

	//
    // Mac OS X 10.7 (Lion) users need to use their real account name and password
    // Mac OS X 10.6 (Snow Leopard) users need to put empty string on username field and can use VNC password (not the real account password)
    //
#ifdef Q_OS_MAC
    //
    // If it's Lion (10.7) and higher then account's username/password should be used.
    //
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
		ui->lbl_info->show();
        ui->le_username->setPlaceholderText("Your account name here");
        ui->le_password->setPlaceholderText("Your account passwd here");
        ui->lbl_username->show();
		ui->lbl_password->setText("Password");
        ui->le_username->show();
    }
#endif
}

SN_PointerUI_VncDialog::~SN_PointerUI_VncDialog()
{
	delete ui;
}

void SN_PointerUI_VncDialog::on_buttonBox_accepted()
{
	_username = ui->le_username->text();
	_password = ui->le_password->text();
}
