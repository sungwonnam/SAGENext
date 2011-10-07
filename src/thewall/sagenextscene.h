#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>
#include <QGraphicsWidget>

#include <QGraphicsLineItem>
#include <QGraphicsLinearLayout>

#include <QSettings>

class SN_PixmapButton;
class SN_WallPartitionBar;
//class PartitionTreeNode;

class SN_UiServer;
class SN_ResourceMonitor;
class SN_SchedulerControl;

class SN_LayoutWidget;
class SN_BaseWidget;

class SN_Launcher;

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

class SN_WallPartitionBar : public QGraphicsLineItem {
public:
	SN_WallPartitionBar(Qt::Orientation o, SN_LayoutWidget *owner, QGraphicsItem *parent=0);

	inline Qt::Orientation orientation() const {return _orientation;}
	inline SN_LayoutWidget * ownerNode() {return _ownerNode;}

protected:

private:
	SN_LayoutWidget *_ownerNode;
	Qt::Orientation _orientation;
};







/**
  will hold BaseWdigets or another SAGENextLayoutWidget
  */
class SN_LayoutWidget : public QGraphicsWidget {
	Q_OBJECT
public:
	SN_LayoutWidget(const QString &posStr, SN_LayoutWidget *parentWidget, const QSettings *s, QGraphicsItem *parent=0);
	~SN_LayoutWidget();

//	inline SN_LayoutWidget * leftWidget() {return _leftWidget;}
//	inline SN_LayoutWidget * rightWidget() {return _rightWidget;}

//	inline SN_LayoutWidget * topWidget() {return _topWidget;}
//	inline SN_LayoutWidget * bottomWidget() {return _bottomWidget;}

	inline SN_LayoutWidget * firstChildLayout() {return _firstChildLayout;}
	inline SN_LayoutWidget * secondChildLayout() {return _secondChildLayout;}

	/**
	  Add widget as my child. This will automatically add widget to the scene
	  */
	void addItem(SN_BaseWidget *bw, const QPointF &pos = QPointF(30, 30));

	/**
	  Reparent all the basewidgets to the new layoutWidget
	  */
	void reparentWidgets(SN_LayoutWidget *newParent);

	/**
	  This will call resize()
	  */
	void setRectangle(const QRectF &r);


	/**
	  widget's center is (0,0)
	  */
//	QRectF boundingRect() const;

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	const QSettings *_settings;

	SN_LayoutWidget *_parentLayoutWidget;

	/**
	  If this widget has Vertical bar
	  */
//	SN_LayoutWidget *_leftWidget;
//	SN_LayoutWidget *_rightWidget;

	/**
	  If this widget has Horizontal bar
	  */
//	SN_LayoutWidget *_topWidget;
//	SN_LayoutWidget *_bottomWidget;

	SN_LayoutWidget *_firstChildLayout;
	SN_LayoutWidget *_secondChildLayout;

	SN_WallPartitionBar *_bar;
	SN_PixmapButton *_tileButton;
	SN_PixmapButton *_hButton;
	SN_PixmapButton *_vButton;
	SN_PixmapButton *_xButton;

	QGraphicsItemGroup *_buttonGrp;

	bool _isTileOn;

	/**
	  left , right, top, or bottom
	  */
	QString _position;

	void createChildPartitions(Qt::Orientation barOrientation, const QRectF &first, const QRectF &second);

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
	void saveSession(QDataStream &out);

	void loadSession(QDataStream &in, SN_Launcher *launcher);
};





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
