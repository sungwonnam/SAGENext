#ifndef SETTINGSTACKEDDIALOG_H
#define SETTINGSTACKEDDIALOG_H

#include <QDialog>

class QSettings;

namespace Ui {
class GeneralSettingDialog;
class SystemSettingDialog;
class GraphicsSettingDialog;

class SettingStackedDialog;
}

class GeneralSettingDialog : public QDialog {
    Q_OBJECT
public:
	explicit GeneralSettingDialog(QSettings *s, QWidget *parent=0);
	~GeneralSettingDialog();
	
private:
    Ui::GeneralSettingDialog *ui;
    QSettings *_settings;
	
public slots:
	void accept();
};

class SystemSettingDialog : public QDialog {
    Q_OBJECT
public:
    explicit SystemSettingDialog(QSettings *s, QWidget *parent=0);
    ~SystemSettingDialog();
	
private:
    Ui::SystemSettingDialog *ui;
    QSettings *_settings;
	
public slots:
	void accept();
};

class GraphicsSettingDialog : public QDialog {
    Q_OBJECT
public:
    explicit GraphicsSettingDialog(QSettings *s, QWidget *parent=0);
    ~GraphicsSettingDialog();
	
private:
    Ui::GraphicsSettingDialog *ui;
    QSettings *_settings;
	
public slots:
	void accept();
};





class QListWidgetItem;

class SettingStackedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingStackedDialog(QSettings *s, QWidget *parent = 0);
    ~SettingStackedDialog();

private:
    Ui::SettingStackedDialog *ui;
    QSettings *_settings;
    
private slots:
	void changeStackedWidget(QListWidgetItem *current, QListWidgetItem *previous);
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();
};




#endif // SETTINGSTACKEDDIALOG_H
