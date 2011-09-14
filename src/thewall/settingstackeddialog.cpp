#include "settingstackeddialog.h"
#include "ui_settingstackeddialog.h"

#include <QSettings>

SettingStackedDialog::SettingStackedDialog(QSettings *s, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingStackedDialog)
    , _settings(s)
{
    ui->setupUi(this);
    
    setWindowTitle(tr("Config Dialog"));
    
    /*
      Fill listWidget
      */
    QListWidgetItem item("general", ui->listWidget);
    item.setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    
    ui->listWidget->setCurrentRow(0);
    
    connect(ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(changeStackedWidget(QListWidgetItem*,QListWidgetItem*)));
    
    
    
    /*
      stacked widget for each setting page
      */
    ui->stackedWidget->addWidget(new GeneralSettingWidget(_settings));
}

SettingStackedDialog::~SettingStackedDialog()
{
    delete ui;
}

void SettingStackedDialog::changeStackedWidget(QListWidgetItem *current, QListWidgetItem *previous) {
    if (!current) current = previous;
    ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
}
