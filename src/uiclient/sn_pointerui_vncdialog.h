#ifndef SN_POINTERUI_VNCDIALOG_H
#define SN_POINTERUI_VNCDIALOG_H

#include <QDialog>

namespace Ui {
	class SN_PointerUI_VncDialog;
}

class SN_PointerUI_VncDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit SN_PointerUI_VncDialog(QWidget *parent = 0);
	~SN_PointerUI_VncDialog();

	inline QString username() const {return _username;}
	inline QString password() const {return _password;}
	
private slots:
	void on_buttonBox_accepted();

private:
	Ui::SN_PointerUI_VncDialog *ui;

	QString _username;
	QString _password;
};

#endif // SN_POINTERUI_VNCDIALOG_H
