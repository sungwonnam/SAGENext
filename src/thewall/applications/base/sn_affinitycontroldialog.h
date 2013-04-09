#ifndef AFFINITYCONTROLDIALOG_H
#define AFFINITYCONTROLDIALOG_H

#include <QDialog>

class SN_AffinityInfo;
class QSettings;

namespace Ui {
	class SN_AffinityControlDialog;
}

class SN_AffinityControlDialog : public QDialog
{
	Q_OBJECT
public:
	explicit SN_AffinityControlDialog(const quint64 gappid, SN_AffinityInfo *ai, const QSettings *s, QWidget *parent = 0);
	~SN_AffinityControlDialog();

private:
	Ui::SN_AffinityControlDialog *ui;
	const QSettings *settings;

	/**
	  * app specific affinity info is stored at/modified from here
	  */
	SN_AffinityInfo *affInfo;

	const quint64 globalAppId;

signals:

public slots:
	/**
	  * Update most recent affinity info from affInfo object
	  */
	void updateInfo();

private slots:
	void on_closeButton_clicked();

	/**
	  * The new values are stored in temporary storage at the AffinityInfo class object.
	  * A thread should read new values from this temporary storage and apply
	  */
	void on_applyButton_clicked();
};

#endif // AFFINITYCONTROLDIALOG_H
