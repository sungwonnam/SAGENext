#ifndef SN_POINTERUI_CONNDIALOG_H
#define SN_POINTERUI_CONNDIALOG_H

#include <QDialog>
#include <QHostAddress>
#include <QSettings>

namespace Ui {
	class SN_PointerUI_ConnDialog;
}

class SN_PointerUI_ConnDialog : public QDialog {
	Q_OBJECT
public:
	SN_PointerUI_ConnDialog(QSettings *s, QWidget *parent=0);
	~SN_PointerUI_ConnDialog();

	inline QHostAddress ipaddress() const {return _ipaddress;}
    inline QString hostname() const {return _hostname;}
	inline int port() const {return portnum;}
	//inline QString myAddress() const {return myaddr;}
	inline QString pointerName() const {return pName;}
	inline QString pointerColor() const {return pColor;}
//	inline QString vncUsername() const {return vncusername;}
//	inline QString vncPasswd() const {return vncpass;}
	inline QString sharingEdge() const {return psharingEdge;}

private:
	Ui::SN_PointerUI_ConnDialog *ui;
	QSettings *_settings;

        /**
          wall address
          */
	QHostAddress _ipaddress;

    /*!
      wall's hostname
      */
    QString _hostname;

        /**
          wall port
          */
	quint16 portnum;

//	QString vncusername;

//	QString vncpass;

        /**
          pointer name
          */
	QString pName;

	QString pColor;

	QString psharingEdge;

private slots:
	void on_buttonBox_rejected();
	void on_buttonBox_accepted();
	void on_pointerColorButton_clicked();
};


#endif // SN_POINTERUI_CONNDIALOG_H
