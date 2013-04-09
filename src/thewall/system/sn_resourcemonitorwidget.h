#ifndef RESOURCEMONITORWIDGET_H
#define RESOURCEMONITORWIDGET_H

#include <QtGui>

#ifdef USE_QWT
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#endif

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
class SN_PriorityGrid;

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
	class SN_ResourceMonitorWidget;
}

class ResourceMonitorWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ResourceMonitorWidget(SN_ResourceMonitor *rm, SN_PriorityGrid *pg, QWidget *parent = 0);
	~ResourceMonitorWidget();

    /*!
      SchedulerControl has its GUI.
      When the resourcemonitorwidget is available, the sched control will add its GUI to the rmonwidget using this function
      */
    void setSchedCtrlFrame(QFrame *frame);

private:
	Ui::SN_ResourceMonitorWidget *ui;

    /*!
      keep refreshing data or not
      */
    bool _isRefreshEnabled;

	SN_ResourceMonitor *_rMonitor;

	SN_PriorityGrid *_pGrid;


	/*!
	  create HBoxLayouts for per CPU perf monitor
	  */
	void buildPerCpuHLayouts();


	/*!
	  This function must be called after the perAppPerfTable has sorted
	  */
	void populatePerfDataPerPriorityRank();


	QMap<int, PerfItem> PerfPerRankMap;

	quint64 _refreshCount;


	void refreshCPUdata();

	void refreshPerAppPerfData();

	void refreshPerAppPriorityData();

	void refreshPriorityGridData();

#ifdef USE_QWT
    QwtPlot *_priorityHistogramPlot;
    QwtPlotCurve _priorityHistogram;
    void updatePriorityHistogram();

    QwtPlot *_qualityCurvePlot;
    QwtPlotCurve _qualityCurve;
    void updateQualityCurve();
#endif


public slots:
    /*!
      This is connected to the resource monitor's dataRefreshed signal
      */
	void refresh();

private slots:

    /*!
      Invoke SN_ResourceMonitor::setPrintData() slot and pass the filename
      */
    void on_printDataBtn_clicked();

    void on_toggleRefreshDataBtn_clicked();
};










#endif // SUNGWONTHESIS_H
