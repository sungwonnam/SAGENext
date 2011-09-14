#ifndef SETTINGSTACKEDDIALOG_H
#define SETTINGSTACKEDDIALOG_H

#include <QDialog>


namespace Ui {
    class SettingStackedDialog;
}

class QSettings;
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
    
public slots:
    void changeStackedWidget(QListWidgetItem *current, QListWidgetItem *previous);
};



namespace Ui {
class GeneralSettingWidget;
}

class GeneralSettingWidget : public QWidget {
    Q_OBJECT
public:
    explicit GeneralSettingWidget(QWidget *parent=0);
    ~GeneralSettingWidget();
    
private:
    Ui::GeneralSettingWidget *ui;
};




#endif // SETTINGSTACKEDDIALOG_H
