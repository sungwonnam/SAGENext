#ifndef RESOURCEMONITORWIDGET_H
#define RESOURCEMONITORWIDGET_H

#include <QtCore>
#include <QtGui>

/*
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_marker.h>
*/

class SN_ResourceMonitor;
class SN_SchedulerControl;
class SN_AbstractScheduler;

//class GraphicsViewMain;

/*!
  Main plot class
  */
/*
class SchedulingPlot : public QwtPlot {
	Q_OBJECT

public:
	SchedulingPlot(ResourceMonitor *, AbstractScheduler *, QWidget *parent = 0);

	void updateData(qint64 ctse=0);

	void setFiducialMarkerPos(const QPoint &);

private:
	ResourceMonitor *rm;
	AbstractScheduler *scheduler;

	QwtPlotCurve *curve;
	QwtPlotMarker fiducialMarker;


private slots:

};

class CanvasPicker : public QObject {
	Q_OBJECT
public:
	CanvasPicker(SchedulingPlot *parent=0);
	virtual bool eventFilter(QObject *, QEvent *);

private:
	SchedulingPlot *plot() { return (SchedulingPlot *)parent(); }
	const SchedulingPlot *plot() const { return (SchedulingPlot *)parent(); }
};
*/

typedef struct {
	qreal recvfps;
	qreal cpuusage;
	qreal observedquality;
	quint64 count;
} PerfItem;



namespace Ui {
class ResourceMonitorWidget;
}

class ResourceMonitorWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ResourceMonitorWidget(SN_ResourceMonitor *rm, SN_SchedulerControl *sc, QWidget *parent = 0);
	~ResourceMonitorWidget();

	inline void setNumWidgets(int i) {_numWidgets=i;}
	inline int numWidgets() const {return _numWidgets;}

	inline void setMediaFilename(QString str) {_mediaFilename=str;}
	inline QString mediaFilename() const {return _mediaFilename;}

private:
	Ui::ResourceMonitorWidget *ui;

	SN_ResourceMonitor *rMonitor;
	SN_SchedulerControl *schedcontrol;

	/*!
	  how many widgets do you want to load
	  */
	int _numWidgets;

	/*!
	  media file to open when using mplayer
	  */
	QString _mediaFilename;

	bool isAllocationEnabled;
	bool isScheduleEnabled;

//	QList<qint64> pidList;

	/*!
	  create HBoxLayouts for per CPU perf monitor
	  */
	void buildPerCpuHLayouts();


	/*!
	  This function must be called after the perAppPerfTable has sorted
	  */
	void populatePerfDataPerPriorityRank();


	QMap<int, PerfItem> PerfPerRankMap;

	quint64 refreshCount;

//	void layoutButtons();

	/*!
	  one per processor
	  */
//	QVector<QHBoxLayout *> hlvec;


	/*!
	  Per widget performance data
	  */
//	QTableWidget widgetDataTable;


	/*!
	  */
//	SchedulingPlot *plot;


signals:

public slots:
	void refresh();

private slots:
//	void on_loadTestSetButton_clicked();
//	void on_allocationButton_clicked();
//	void on_scheduleButton_clicked();

};










#endif // SUNGWONTHESIS_H
