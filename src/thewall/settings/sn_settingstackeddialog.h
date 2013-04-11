#ifndef SETTINGSTACKEDDIALOG_H
#define SETTINGSTACKEDDIALOG_H

#include <QDialog>

class QSettings;
class QFrame;

namespace Ui {
class SN_GeneralSettingDialog;
class SN_SystemSettingDialog;
class SN_GraphicsSettingDialog;
class SN_ScreenLayoutDialog;
class SN_GuiSettingDialog;

class SN_SettingStackedDialog;
}

/**
  wall address:port
  fsManager address:port
  wall width x height
  wall offset (x,y)
  */
class SN_GeneralSettingDialog : public QDialog {
    Q_OBJECT
public:
	explicit SN_GeneralSettingDialog(QSettings *s, QWidget *parent=0);
	~SN_GeneralSettingDialog();
	
private:
    Ui::SN_GeneralSettingDialog *ui;
    QSettings *_settings;
	
public slots:
	void accept();
};

/**
  NUMA related stuff
  ResourceMonitor and Scheduler related stuff
  */
class SN_SystemSettingDialog : public QDialog {
    Q_OBJECT
public:
    explicit SN_SystemSettingDialog(QSettings *s, QWidget *parent=0);
    ~SN_SystemSettingDialog();
	
private:
    Ui::SN_SystemSettingDialog *ui;
    QSettings *_settings;
	
public slots:
	void accept();
private slots:
	void on_schedulerCheckBox_stateChanged(int arg1);
	void on_rmonitorCheckBox_stateChanged(int arg1);
    void on_pGridCheckBox_stateChanged(int state);
};

/**
  Graphics backend (e.g. -graphicssystem raster)
  Whether OpenGL widget as the viewport or not
  The view update modes
  */
class SN_GraphicsSettingDialog : public QDialog {
    Q_OBJECT
public:
    explicit SN_GraphicsSettingDialog(QSettings *s, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent=0);
    ~SN_GraphicsSettingDialog();
	
private:
    Ui::SN_GraphicsSettingDialog *ui;
    QSettings *_settings;

	QMap<QPair<int,int>,int> *_layoutMap;

	int _numScreens;
	
public slots:
	void accept();

	void createLayoutGrid();
private slots:
	void on_openglviewportCheckBox_stateChanged(int arg1);
};

class SN_ScreenLayoutDialog : public QDialog {
	Q_OBJECT
public:
	explicit SN_ScreenLayoutDialog(QSettings *s, int dimx, int dimy, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent=0);
	~SN_ScreenLayoutDialog();
private:
	Ui::SN_ScreenLayoutDialog *ui;
	QSettings *_settings;
	int _dimx;
	int _dimy;
	QMap<QPair<int,int>,int> *_layoutMap;
	QFrame * createScreen(int row, int col);
private slots:
	void saveToSettings();
};

/**
  GUI related stuff
  */
class SN_GuiSettingDialog : public QDialog {
	Q_OBJECT
public:
	explicit SN_GuiSettingDialog(QSettings *s, QWidget *parent=0);
	~SN_GuiSettingDialog();

private:
	Ui::SN_GuiSettingDialog *ui;
	QSettings *_settings;



public slots:
	void accept();
};





class QListWidgetItem;

class SN_SettingStackedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SN_SettingStackedDialog(QSettings *s, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent = 0);
    ~SN_SettingStackedDialog();

private:
    Ui::SN_SettingStackedDialog *ui;
    QSettings *_settings;
    
private slots:
	void changeStackedWidget(QListWidgetItem *current, QListWidgetItem *previous);
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();
};




#endif // SETTINGSTACKEDDIALOG_H
