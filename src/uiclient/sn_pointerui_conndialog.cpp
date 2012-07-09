#include "sn_pointerui_conndialog.h"
#include "ui_sn_pointerui_conndialog.h"

#include <QColorDialog>

SN_PointerUI_ConnDialog::SN_PointerUI_ConnDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SN_PointerUI_ConnDialog)
    , _settings(s)
    , portnum(0)
{
	ui->setupUi(this);

	_ipaddress.clear(); // 0.0.0.0  = isNull() true
    _hostname.clear(); // isEmpty() true

    ui->ipaddr->setPlaceholderText("hostname or IP address");

//	ui->ipaddr->setInputMask("000.000.000.000;_");
	ui->port->setInputMask("00000;_");

    //
    // retrieve the saved value
    //
	ui->ipaddr->setText( _settings->value("walladdr", "127.0.0.1").toString() );


	/**
		QList<QHostAddress> myiplist = QNetworkInterface::allAddresses();
		for (int i=0; i<myiplist.size(); ++i) {
			if (myiplist.at(i).toIPv4Address()) {
				ui->myAddrCB->addItem(myiplist.at(i).toString(), myiplist.at(i).toIPv4Address()); // QString, quint32
			}
		}
		int currentIdx = ui->myAddrCB->findText( _settings->value("myaddr", "127.0.0.1").toString() );
		ui->myAddrCB->setCurrentIndex(currentIdx);
		**/



	ui->port->setText( _settings->value("wallport", 30003).toString() );
	ui->pointerNameLineEdit->setText( _settings->value("pointername", "pointerName").toString());


	//		ui->pointerColorLabel->setText(_settings->value("pointercolor", "#FF0000").toString());
	QColor pc(_settings->value("pointercolor", "#ff0000").toString());
	pColor = pc.name();
	QPixmap pixmap(ui->pointerColorLabel->size());
	pixmap.fill(pc);
	ui->pointerColorLabel->setPixmap(pixmap);

	int idx = ui->sharingEdgeCB->findText(_settings->value("sharingedge", "right").toString());
	if (idx != -1)
		ui->sharingEdgeCB->setCurrentIndex(idx);
	else
		ui->sharingEdgeCB->setCurrentIndex(2); // 0 top, 1 left, 2 right, 3 bottom

	adjustSize();

//	ui->ipaddr->setText("127.0.0.1");
//	ui->port->setText("30003");
}

SN_PointerUI_ConnDialog::~SN_PointerUI_ConnDialog() {
	delete ui;
}

void SN_PointerUI_ConnDialog::on_buttonBox_accepted()
{
    QString addrtemp = ui->ipaddr->text();

    QRegExp regexp_ipaddr("^\\d+\\.\\d+\\.\\d+\\.\\d+$", Qt::CaseInsensitive);
    if (regexp_ipaddr.exactMatch(addrtemp)) {

        _ipaddress = QHostAddress(addrtemp);
        _settings->setValue("walladdr", _ipaddress.toString());

        qDebug() << "SN_PointerUI_ConnDialog::on_buttonBox_accepted() : IP address detected" << _ipaddress;
    }
    else {
        _hostname = addrtemp;
        _settings->setValue("walladdr", _hostname);

        qDebug() << "SN_PointerUI_ConnDialog::on_buttonBox_accepted() : hostname detected" << _hostname;
    }

	portnum = ui->port->text().toInt();
//	myaddr = ui->myAddrCB->currentText();
	pName = ui->pointerNameLineEdit->text();
	psharingEdge = ui->sharingEdgeCB->currentText();

	_settings->setValue("wallport", portnum);
	_settings->setValue("pointername", pName);
	_settings->setValue("pointercolor", pColor);
	_settings->setValue("sharingedge", psharingEdge);

	accept();
	//	done(0);
}

void SN_PointerUI_ConnDialog::on_buttonBox_rejected()
{
	reject();
}

void SN_PointerUI_ConnDialog::on_pointerColorButton_clicked()
{
	QColor prevColor;
	prevColor.setNamedColor(_settings->value("pointercolor", "#FF0000").toString());
	QColor newColor = QColorDialog::getColor(prevColor, this, "Pointer Color"); // pops up modal dialog

	QPixmap pixmap(ui->pointerColorLabel->size());
	pixmap.fill(newColor);
//	ui->pointerColorLabel->setText(newColor.name());
	ui->pointerColorLabel->setPixmap(pixmap);

	pColor = newColor.name();
}

