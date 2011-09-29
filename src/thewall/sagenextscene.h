#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>
#include <QGraphicsWidget>

#include <QGraphicsLineItem>
#include <QGraphicsLinearLayout>

#include <QSettings>

class SAGENextPixmapButton;
class PartitionBar;
//class PartitionTreeNode;

class UiServer;
class ResourceMonitor;
class SchedulerControl;

class SAGENextLayoutWidget;
class BaseWidget;


//class PartitionBar : public QGraphicsLineItem {
//public:
//	PartitionBar(Qt::Orientation o, PartitionTreeNode *owner, QGraphicsItem *parent=0);

//	inline Qt::Orientation orientation() const {return _orientation;}

//protected:
//	QVariant itemChange(GraphicsItemChange change, const QVariant &value);

//private:
//	PartitionTreeNode *_ownerNode;

//	Qt::Orientation _orientation;
//};

class PartitionBar : public QGraphicsLineItem {
public:
	PartitionBar(Qt::Orientation o, SAGENextLayoutWidget *owner, QGraphicsItem *parent=0);

	inline Qt::Orientation orientation() const {return _orientation;}
	inline SAGENextLayoutWidget * ownerNode() {return _ownerNode;}

protected:

private:
	SAGENextLayoutWidget *_ownerNode;
	Qt::Orientation _orientation;
};







/**
  will hold BaseWdigets or another SAGENextLayoutWidget
  */
class SAGENextLayoutWidget : public QGraphicsWidget {
	Q_OBJECT
public:
	SAGENextLayoutWidget(const QString &pos, const QRectF &r, SAGENextLayoutWidget *parentWidget, const QSettings *s, QGraphicsItem *parent=0);
	~SAGENextLayoutWidget();

	inline SAGENextLayoutWidget * leftWidget() {return _leftWidget;}
	inline SAGENextLayoutWidget * rightWidget() {return _rightWidget;}

	inline SAGENextLayoutWidget * topWidget() {return _topWidget;}
	inline SAGENextLayoutWidget * bottomWidget() {return _bottomWidget;}

	/**
	  Add widget as my child. This will automatically add widget to the scene
	  */
	void addItem(BaseWidget *bw, const QPointF &scenepos = QPointF(30, 30));

	/**
	  Reparent all the basewidgets to the new layoutWidget
	  */
	void reparentWidgets(SAGENextLayoutWidget *newParent);

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	const QSettings *_settings;

	SAGENextLayoutWidget *_parentLayoutWidget;

	/**
	  If this widget has Vertical bar
	  */
	SAGENextLayoutWidget *_leftWidget;
	SAGENextLayoutWidget *_rightWidget;

	/**
	  If this widget has Horizontal bar
	  */
	SAGENextLayoutWidget *_topWidget;
	SAGENextLayoutWidget *_bottomWidget;

	PartitionBar *_bar;
	SAGENextPixmapButton *_tileButton;
	SAGENextPixmapButton *_hButton;
	SAGENextPixmapButton *_vButton;
	SAGENextPixmapButton *_xButton;

	QGraphicsItemGroup *_buttonGrp;

	bool _isTileOn;

	/**
	  left , right, top, or bottom
	  */
	QString _position;

	void createChildPartitions(Qt::Orientation barOrientation);

	void setButtonPos();

	void doTile();

signals:
	void resized();

public slots:
	/**
	  create horizontal bar that partitions this widget into top and bottom
	  */
	inline void createHBar() {createChildPartitions(Qt::Horizontal);}

	/**
	  creates vertical bar that partitions this widget into left and right
	  */
	inline void createVBar() {createChildPartitions(Qt::Vertical);}

	// I'll become actual partition
	void deleteChildPartitions();

	void adjustBar();


	void toggleTile();

	/**
	  */
	void saveCurrentSession();
};





class SAGENextScene : public QGraphicsScene
{
	Q_OBJECT
public:
	explicit SAGENextScene(const QRectF &sceneRect, const QSettings *s, QObject *parent = 0);
	~SAGENextScene();

	inline void setUiServer(UiServer *u) {_uiserver = u;}
//	inline void setRMonitor(ResourceMonitor *r) {_rmonitor = r;}
	inline void setSchedControl(SchedulerControl *s) {_schedcontrol = s;}
	inline SAGENextLayoutWidget * rootLayoutWidget() {return _rootLayoutWidget;}

	bool isOnAppRemoveButton(const QPointF &scenepos);


	/**
	  Add basewidget to the SAGENextLayoutWidget
	  */
	void addItemOnTheLayout(BaseWidget *bw, const QPointF &scenepos = QPointF(30,30));




	/**
	  BaseWidget can register for mousehover by setting _registerMouseHover to true. The registration is done by SAGENextLauncher
	  SAGENext pointers will iterate over this list in pointerMove() function
	  ,and call BaseWidget::toggleHover()
	  */
	QList<BaseWidget *> hoverAcceptingApps;



	/**
	  This must be called after scene rect has become valid
	  */
//	void setRootPartition();

//	inline QGraphicsLinearLayout * rootLinearLayout() {return _rootLinearLayout;}

private:
	const QSettings *_settings;
	UiServer *_uiserver;
//	ResourceMonitor *_rmonitor;
	SchedulerControl *_schedcontrol;

//	PartitionTreeNode *_rootPartition;
	SAGENextLayoutWidget *_rootLayoutWidget;

	/**
	  first click on _closeButton will just set the flag
	  second click on _closeButton will trigger shutdown
	  */
	bool _closeFlag;

	/**
	  shutdown the wall
	  */
	SAGENextPixmapButton *_closeButton;

	/**
	  close an application
	  */
	SAGENextPixmapButton *_appRemoveButton;

public slots:
	void closeAllUserApp();

	/**
	  kills UiServer, fsManager, ResourceMonitor
	  */
	void prepareClosing();
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
