#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>

class QSettings;

class SN_PixmapButton;
//class SN_DrawingWidget;
class SN_WallPartitionBar;
//class PartitionTreeNode;

class SN_UiServer;
class SN_ResourceMonitor;
class SN_SchedulerControl;

class SN_LayoutWidget;
class SN_BaseWidget;

class SN_Launcher;

class SN_MinimizeBar;

class SN_TheScene : public QGraphicsScene
{
	Q_OBJECT
public:
	explicit SN_TheScene(const QRectF &sceneRect, const QSettings *s, QObject *parent = 0);
	~SN_TheScene();

	inline void setUiServer(SN_UiServer *u) {_uiserver = u;}
//	inline void setRMonitor(SN_ResourceMonitor *r) {_rmonitor = r;}
//	inline void setSchedControl(SN_SchedulerControl *s) {_schedcontrol = s;}
	inline SN_LayoutWidget * rootLayoutWidget() {return _rootLayoutWidget;}

	bool isOnAppRemoveButton(const QPointF &scenepos);

    bool isOnMinimizeBar(const QPointF &scenepos);

    void minimizeWidgetOnTheBar(SN_BaseWidget *widget, const QPointF &scenepos);

    void restoreWidgetFromTheBar(SN_BaseWidget *widget);

	/**
	  Add basewidget to the SAGENextLayoutWidget
	  */
	void addItemOnTheLayout(SN_BaseWidget *bw, const QPointF &scenepos = QPointF(30,30));


	/**
	  BaseWidget can register for mousehover by setting _registerMouseHover to true. The registration is done by SAGENextLauncher
	  SAGENext pointers will iterate over this list in pointerMove() function
	  ,and call BaseWidget::toggleHover()
	  */
	QList<SN_BaseWidget *> hoverAcceptingApps;


	/*!
	  retrieve widget with globalAppId
	  */
	SN_BaseWidget * getUserWidget(quint64 gaid);

    QSet<SN_BaseWidget *> getCollidingUserWidgets(QGraphicsItem *item);

	/*!
	  returns the number of user application (UserType + BASEWIDGET_USER)
	  Note that the number of application the resourceMonitor reports doesn't include non-schedulable widget
	  */
	int getNumUserWidget() const;

	/*!
	  return the average window size of user applications. (taking the scale() into an account)
	  */
	QSizeF getAvgWinSize() const;


	/*!
	  Average EVR size
	  */
	qreal getAvgEVRSize() const;

	/*!
	  returns the ratio of the empty space to the scene size.
	  */
	qreal getRatioEmptySpace() const;

	/*!
	  returns the ratio of the size of the overlapped window regions to the scene size.
	  how to properly calculate this ?
	  */
	qreal getRatioOverlapped() const;

//	inline SN_DrawingWidget * drawingCanvas() {return _drawingCanvas;}

private:
	const QSettings *_settings;
	SN_UiServer *_uiserver;
//	SN_ResourceMonitor *_rmonitor;
//	SN_SchedulerControl *_schedcontrol;

//	PartitionTreeNode *_rootPartition;
	SN_LayoutWidget *_rootLayoutWidget;

	/**
	  first click on _closeButton will just set the flag
	  second click on _closeButton will trigger shutdown
	  */
	bool _closeFlag;

	/**
	  shutdown the wall
	  */
	SN_PixmapButton *_closeButton;

	/**
	  close an application
	  */
	SN_PixmapButton *_appRemoveButton;


	/**
	  Free drawing using shared pointer will draw onto this widget
	  */
//	SN_DrawingWidget *_drawingCanvas;

    SN_MinimizeBar *_minimizeBar;

public slots:
	void closeAllUserApp();

	/**
	  kills UiServer, fsManager, ResourceMonitor
	  */
	void prepareClosing();

	void closeNow();

	/**
	  save current app layout when SN_LayoutWidget is not present
	  */
	void saveSession();

	void loadSession(QDataStream &in, SN_Launcher *launcher);
};


#include <QGraphicsWidget>

class SN_MinimizeBar : public QGraphicsWidget
{
    Q_OBJECT
public:
    SN_MinimizeBar(const QSizeF size, SN_TheScene *scene, SN_LayoutWidget *rootlayout, QGraphicsItem *parent=0, Qt::WindowFlags wf=0);

private:
    SN_LayoutWidget *_rootLayout;

    SN_TheScene *_theScene;

public slots:
    /*!
      minimize and place the widget on the position which is in this widget's local coordinate
      */
    void minimizeAndPlaceWidget(SN_BaseWidget *widget, const QPointF &position);

    /*!
      Find the widget under the position
      then restore it
      */
    void restoreWidget(SN_BaseWidget *widget);
};



#endif // SAGENEXTSCENE_H
