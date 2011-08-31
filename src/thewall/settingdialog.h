#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QtGui/QDialog>
#include "ui_settingdialog.h"
#include <QSettings>

namespace Ui {
	class SettingDialogClass;
}

class SettingDialog : public QDialog
{
	Q_OBJECT

public:
	SettingDialog(QSettings *settings, QWidget *parent = 0);
	~SettingDialog();

private:
	Ui::SettingDialogClass *ui;
	QSettings *settings;

signals:
	void settingSaved();
	void settingClosed();

private slots:
	void on_loadButton_clicked();
	void on_saveButton_clicked();
	void on_schedComboBox_currentIndexChanged(QString );
	void on_schedulerCheckBox_stateChanged(int );
	void on_resMonitorCheckBox_stateChanged(int );
};

#endif // SETTINGDIALOG_H
