/********************************************************************************
** Form generated from reading UI file 'resourcemonitorwidget.ui'
**
** Created: Wed Jun 15 19:23:30 2011
**      by: Qt User Interface Compiler version 4.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RESOURCEMONITORWIDGET_H
#define UI_RESOURCEMONITORWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTableWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ResourceMonitorWidget
{
public:
    QHBoxLayout *rootHLayout;
    QVBoxLayout *vLayoutOnTheLeft;
    QVBoxLayout *vlayout_schedparam;
    QLabel *label_schedparam_header;
    QGridLayout *gLayout_schedparam;
    QLabel *label_schedparam_freq_header;
    QLabel *label_schedparam_freq;
    QLabel *label_2;
    QLabel *label_schedparam_sens;
    QLabel *label_4;
    QLabel *label_schedparam_greed;
    QLabel *label_3;
    QLabel *label_schedparam_aggr;
    QVBoxLayout *vLayout_percpu;
    QTableWidget *perfPerPriorityRankTable;
    QVBoxLayout *tablePlotVLayoutOnTheRight;
    QTableWidget *perAppPerfTable;

    void setupUi(QWidget *ResourceMonitorWidget)
    {
        if (ResourceMonitorWidget->objectName().isEmpty())
            ResourceMonitorWidget->setObjectName(QString::fromUtf8("ResourceMonitorWidget"));
        ResourceMonitorWidget->resize(937, 565);
        rootHLayout = new QHBoxLayout(ResourceMonitorWidget);
        rootHLayout->setObjectName(QString::fromUtf8("rootHLayout"));
        vLayoutOnTheLeft = new QVBoxLayout();
        vLayoutOnTheLeft->setObjectName(QString::fromUtf8("vLayoutOnTheLeft"));
        vlayout_schedparam = new QVBoxLayout();
        vlayout_schedparam->setObjectName(QString::fromUtf8("vlayout_schedparam"));
        label_schedparam_header = new QLabel(ResourceMonitorWidget);
        label_schedparam_header->setObjectName(QString::fromUtf8("label_schedparam_header"));

        vlayout_schedparam->addWidget(label_schedparam_header);

        gLayout_schedparam = new QGridLayout();
        gLayout_schedparam->setObjectName(QString::fromUtf8("gLayout_schedparam"));
        label_schedparam_freq_header = new QLabel(ResourceMonitorWidget);
        label_schedparam_freq_header->setObjectName(QString::fromUtf8("label_schedparam_freq_header"));

        gLayout_schedparam->addWidget(label_schedparam_freq_header, 0, 0, 1, 1);

        label_schedparam_freq = new QLabel(ResourceMonitorWidget);
        label_schedparam_freq->setObjectName(QString::fromUtf8("label_schedparam_freq"));

        gLayout_schedparam->addWidget(label_schedparam_freq, 0, 1, 1, 1);

        label_2 = new QLabel(ResourceMonitorWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gLayout_schedparam->addWidget(label_2, 1, 0, 1, 1);

        label_schedparam_sens = new QLabel(ResourceMonitorWidget);
        label_schedparam_sens->setObjectName(QString::fromUtf8("label_schedparam_sens"));

        gLayout_schedparam->addWidget(label_schedparam_sens, 1, 1, 1, 1);

        label_4 = new QLabel(ResourceMonitorWidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gLayout_schedparam->addWidget(label_4, 2, 0, 1, 1);

        label_schedparam_greed = new QLabel(ResourceMonitorWidget);
        label_schedparam_greed->setObjectName(QString::fromUtf8("label_schedparam_greed"));

        gLayout_schedparam->addWidget(label_schedparam_greed, 2, 1, 1, 1);

        label_3 = new QLabel(ResourceMonitorWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gLayout_schedparam->addWidget(label_3, 3, 0, 1, 1);

        label_schedparam_aggr = new QLabel(ResourceMonitorWidget);
        label_schedparam_aggr->setObjectName(QString::fromUtf8("label_schedparam_aggr"));

        gLayout_schedparam->addWidget(label_schedparam_aggr, 3, 1, 1, 1);

        gLayout_schedparam->setColumnStretch(0, 1);
        gLayout_schedparam->setColumnStretch(1, 2);

        vlayout_schedparam->addLayout(gLayout_schedparam);

        vlayout_schedparam->setStretch(0, 1);
        vlayout_schedparam->setStretch(1, 8);

        vLayoutOnTheLeft->addLayout(vlayout_schedparam);

        vLayout_percpu = new QVBoxLayout();
        vLayout_percpu->setObjectName(QString::fromUtf8("vLayout_percpu"));

        vLayoutOnTheLeft->addLayout(vLayout_percpu);

        perfPerPriorityRankTable = new QTableWidget(ResourceMonitorWidget);
        perfPerPriorityRankTable->setObjectName(QString::fromUtf8("perfPerPriorityRankTable"));

        vLayoutOnTheLeft->addWidget(perfPerPriorityRankTable);

        vLayoutOnTheLeft->setStretch(1, 1);
        vLayoutOnTheLeft->setStretch(2, 2);

        rootHLayout->addLayout(vLayoutOnTheLeft);

        tablePlotVLayoutOnTheRight = new QVBoxLayout();
        tablePlotVLayoutOnTheRight->setObjectName(QString::fromUtf8("tablePlotVLayoutOnTheRight"));
        perAppPerfTable = new QTableWidget(ResourceMonitorWidget);
        perAppPerfTable->setObjectName(QString::fromUtf8("perAppPerfTable"));

        tablePlotVLayoutOnTheRight->addWidget(perAppPerfTable);


        rootHLayout->addLayout(tablePlotVLayoutOnTheRight);

        rootHLayout->setStretch(0, 1);
        rootHLayout->setStretch(1, 2);

        retranslateUi(ResourceMonitorWidget);

        QMetaObject::connectSlotsByName(ResourceMonitorWidget);
    } // setupUi

    void retranslateUi(QWidget *ResourceMonitorWidget)
    {
        ResourceMonitorWidget->setWindowTitle(QApplication::translate("ResourceMonitorWidget", "SAGE Next ResourceMonitor", 0, QApplication::UnicodeUTF8));
        label_schedparam_header->setText(QApplication::translate("ResourceMonitorWidget", "Scheduler Parameters", 0, QApplication::UnicodeUTF8));
        label_schedparam_freq_header->setText(QApplication::translate("ResourceMonitorWidget", "Frequency", 0, QApplication::UnicodeUTF8));
        label_schedparam_freq->setText(QApplication::translate("ResourceMonitorWidget", "TextLabel", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("ResourceMonitorWidget", "Sensitivity", 0, QApplication::UnicodeUTF8));
        label_schedparam_sens->setText(QApplication::translate("ResourceMonitorWidget", "TextLabel", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("ResourceMonitorWidget", "Greediness", 0, QApplication::UnicodeUTF8));
        label_schedparam_greed->setText(QApplication::translate("ResourceMonitorWidget", "TextLabel", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("ResourceMonitorWidget", "Aggression", 0, QApplication::UnicodeUTF8));
        label_schedparam_aggr->setText(QApplication::translate("ResourceMonitorWidget", "TextLabel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ResourceMonitorWidget: public Ui_ResourceMonitorWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RESOURCEMONITORWIDGET_H
