#ifndef SETTINGSTACKEDDIALOG_H
#define SETTINGSTACKEDDIALOG_H

#include <QDialog>

class QSettings;

namespace Ui {
class GeneralSettingDialog;
class SystemSettingDialog;
class GraphicsSettingDialog;
class GuiSettingDialog;

class SettingStackedDialog;
}

/**
  wall address:port
  fsManager address:port
  wall width x height
  wall offset (x,y)
  */
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

/**
  NUMA related stuff
  ResourceMonitor and Scheduler related stuff
  */
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
private slots:
	void on_schedulerCheckBox_stateChanged(int arg1);
	void on_rmonitorCheckBox_stateChanged(int arg1);
};

/**
  Graphics backend (e.g. -graphicssystem raster)
  Whether OpenGL widget as the viewport or not
  The view update modes
  */
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

/**
  GUI related stuff
  */
class GuiSettingDialog : public QDialog {
	Q_OBJECT
public:
	explicit GuiSettingDialog(QSettings *s, QWidget *parent=0);
	~GuiSettingDialog();

private:
	Ui::GuiSettingDialog *ui;
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
