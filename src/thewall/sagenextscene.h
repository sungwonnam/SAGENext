#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>
#include <QGraphicsWidget>

#include <QGraphicsLineItem>
#include <QGraphicsLinearLayout>

class PixmapButton;
class PartitionBar;
class PartitionTreeNode;

class UiServer;
class ResourceMonitor;
class SchedulerControl;

class SAGENextLayoutWidget;


class PartitionTreeNode : public QObject {
	Q_OBJECT
public:
	explicit PartitionTreeNode(QGraphicsScene *s, PartitionTreeNode *p, const QRectF &rect, QObject *parent=0);
	~PartitionTreeNode();

	inline PartitionTreeNode * leftOrTop() {return _leftOrTop;}
	inline PartitionTreeNode * rightOrBottom() {return _rightOrBottom;}

	inline int orientation() const {return _orientation;}

	/**
	  adjust children's _rectf
	  */
	void lineHasMoved(const QPointF &newpos);

private:
	QGraphicsScene *_scene;

	PartitionTreeNode *_parentNode;
	/**
	  left or top
	  */
	PartitionTreeNode * _leftOrTop;
	PartitionTreeNode * _rightOrBottom;

	/**
	  1 : horizontal
	  2 : vertical
	  */
	int _orientation;

	QGraphicsLineItem *_lineItem;

//	int _partitionId;

	PixmapButton *_tileButton;

	PixmapButton *_hButton;
	PixmapButton *_vButton;

	PixmapButton *_closeButton;

	QRectF _rectf;

	/**
	  create childs and I become partition holder which has _lineItem and no button showing
	  */
	void createNewPartition(int o);

public slots:
	inline void createNewHPartition() {createNewPartition(1);}
	inline void createNewVPartition() {createNewPartition(2);}

	/**
	  I'm actual partition and my rect changed
	  */
	void setNewRectf(const QRectF &r);

	/**
	  delete childs and I become an actual partition which has no _lineItem and all buttons showing
	  */
	void deleteChildPartitions();
};


















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
	QVariant itemChange(GraphicsItemChange change, const QVariant &value);

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
	SAGENextLayoutWidget(const QRectF &r, SAGENextLayoutWidget *parentWidget=0, QGraphicsItem *parent=0);
	~SAGENextLayoutWidget();

	inline SAGENextLayoutWidget * leftTopWidget() {return _left_top_w;}
	inline SAGENextLayoutWidget * rightBottomWidget() {return _right_bottom_w;}

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
	void resizeEvent(QGraphicsSceneResizeEvent *event);
	QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
	SAGENextLayoutWidget *_parentWidget;
	SAGENextLayoutWidget *_left_top_w;
	SAGENextLayoutWidget *_right_bottom_w;

	PartitionBar *_bar;
	PixmapButton *_tileButton;
	PixmapButton *_hButton;
	PixmapButton *_vButton;
	PixmapButton *_xButton;

	QGraphicsItemGroup *_buttonGrp;

	bool _isTileOn;

	void createChildPartitions(Qt::Orientation);

	void setButtonPos();

signals:
	void resized();

public slots:
	inline void createHChildPartitions() {createChildPartitions(Qt::Horizontal);}
	inline void createVChildPartitions() {createChildPartitions(Qt::Vertical);}

	// I'll become actual partition
	void deleteChildPartitions();

	void adjustBar();

	void doTile();
};





class SAGENextScene : public QGraphicsScene
{
	Q_OBJECT
public:
	explicit SAGENextScene(const QRectF &sceneRect, QObject *parent = 0);
	~SAGENextScene();

	inline void setUiServer(UiServer *u) {_uiserver = u;}
	inline void setRMonitor(ResourceMonitor *r) {_rmonitor = r;}
	inline void setSchedControl(SchedulerControl *s) {_schedcontrol = s;}

	/**
	  This must be called after scene rect has become valid
	  */
//	void setRootPartition();

//	inline QGraphicsLinearLayout * rootLinearLayout() {return _rootLinearLayout;}

private:
	UiServer *_uiserver;
	ResourceMonitor *_rmonitor;
	SchedulerControl *_schedcontrol;

//	PartitionTreeNode *_rootPartition;
	SAGENextLayoutWidget *_rootLayoutWidget;
signals:

public slots:
	void closeAllUserApp();

	/**
	  kills UiServer, fsManager, ResourceMonitor
	  */
	void prepareClosing();

};

#endif // SAGENEXTSCENE_H
