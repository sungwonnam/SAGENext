#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>
#include <QGraphicsWidget>

#include <QGraphicsLineItem>
//#include <QGraphicsLinearLayout>

#include <QSettings>

class SN_PixmapButton;
class SN_DrawingWidget;
class SN_WallPartitionBar;
//class PartitionTreeNode;

class SN_UiServer;
class SN_ResourceMonitor;
class SN_SchedulerControl;

class SN_LayoutWidget;
class SN_BaseWidget;

class SN_Launcher;



class SN_TheScene : public QGraphicsScene
{
	Q_OBJECT
public:
	explicit SN_TheScene(const QRectF &sceneRect, const QSettings *s, QObject *parent = 0);
	~SN_TheScene();

	inline void setUiServer(SN_UiServer *u) {_uiserver = u;}
//	inline void setRMonitor(ResourceMonitor *r) {_rmonitor = r;}
	inline void setSchedControl(SN_SchedulerControl *s) {_schedcontrol = s;}
	inline SN_LayoutWidget * rootLayoutWidget() {return _rootLayoutWidget;}

	bool isOnAppRemoveButton(const QPointF &scenepos);


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


	inline SN_DrawingWidget * drawingCanvas() {return _drawingCanvas;}


private:
	const QSettings *_settings;
	SN_UiServer *_uiserver;
//	ResourceMonitor *_rmonitor;
	SN_SchedulerControl *_schedcontrol;

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
	SN_DrawingWidget *_drawingCanvas;

public slots:
	void closeAllUserApp();

	/**
	  kills UiServer, fsManager, ResourceMonitor
	  */
	void prepareClosing();

	/**
	  save current app layout
	  */
	void saveSession();

	void loadSession(QDataStream &in, SN_Launcher *launcher);
};




//class PartitionTreeNode : public QObject {
//	Q_OBJECT
//public:
//	explicit PartitionTreeNode(QGraphicsScene *s, PartitionTreeNode *p, const QRectF &rect, QObject *parent=0);
//	~PartitionTreeNode();

//	inline PartitionTreeNode * leftOrTop() {return _leftOrTop;}
//	inline PartitionTreeNode * rightOrBottom() {return _rightOrBottom;}

//	inline int orientation() const {return _orientation;}

//	/**
//	  adjust children's _rectf
//	  */
//	void lineHasMoved(const QPointF &newpos);

//private:
//	QGraphicsScene *_scene;

//	PartitionTreeNode *_parentNode;
//	/**
//	  left or top
//	  */
//	PartitionTreeNode * _leftOrTop;
//	PartitionTreeNode * _rightOrBottom;

//	/**
//	  1 : horizontal
//	  2 : vertical
//	  */
//	int _orientation;

//	QGraphicsLineItem *_lineItem;

////	int _partitionId;

//	PixmapButton *_tileButton;

//	PixmapButton *_hButton;
//	PixmapButton *_vButton;

//	PixmapButton *_closeButton;

//	QRectF _rectf;

//	/**
//	  create childs and I become partition holder which has _lineItem and no button showing
//	  */
//	void createNewPartition(int o);

//public slots:
//	inline void createNewHPartition() {createNewPartition(1);}
//	inline void createNewVPartition() {createNewPartition(2);}

//	/**
//	  I'm actual partition and my rect changed
//	  */
//	void setNewRectf(const QRectF &r);

//	/**
//	  delete childs and I become an actual partition which has no _lineItem and all buttons showing
//	  */
//	void deleteChildPartitions();
//};



#endif // SAGENEXTSCENE_H
