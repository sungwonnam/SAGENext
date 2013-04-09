#ifndef SETTINGSTACKEDDIALOG_H
#define SETTINGSTACKEDDIALOG_H

#include <QDialog>

class QSettings;
class QFrame;

namespace Ui {
class GeneralSettingDialog;
class SystemSettingDialog;
class GraphicsSettingDialog;
class ScreenLayoutDialog;
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
    void on_pGridCheckBox_stateChanged(int state);
};

/**
  Graphics backend (e.g. -graphicssystem raster)
  Whether OpenGL widget as the viewport or not
  The view update modes
  */
class GraphicsSettingDialog : public QDialog {
    Q_OBJECT
public:
    explicit GraphicsSettingDialog(QSettings *s, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent=0);
    ~GraphicsSettingDialog();
	
private:
    Ui::GraphicsSettingDialog *ui;
    QSettings *_settings;

	QMap<QPair<int,int>,int> *_layoutMap;

	int _numScreens;
	
public slots:
	void accept();

	void createLayoutGrid();
private slots:
	void on_openglviewportCheckBox_stateChanged(int arg1);
};

class ScreenLayoutDialog : public QDialog {
	Q_OBJECT
public:
	explicit ScreenLayoutDialog(QSettings *s, int dimx, int dimy, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent=0);
	~ScreenLayoutDialog();
private:
	Ui::ScreenLayoutDialog *ui;
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
    explicit SettingStackedDialog(QSettings *s, QMap<QPair<int,int>,int> *screenLayout, QWidget *parent = 0);
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
