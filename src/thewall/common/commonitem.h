#ifndef COMMONITEM_H
#define COMMONITEM_H

#include <QtGui>

class BaseWidget;

class PolygonArrow : public QGraphicsPolygonItem {
public:
	PolygonArrow(const quint64 uicid, const QSettings *, const QColor c, QGraphicsItem *parent=0);
	~PolygonArrow();

	void setPointerName(const QString &text);

	bool setAppUnderPointer(const QPointF scenePosOfPointer);
	inline BaseWidget * appUnderPointer() {return app;}

//	enum { Type = UserType + 1 };
//	int type() const { return Type; } // reimplementing this


	/**
	  * Moves pointer itself
	  * If dragging and there's app under the pointer then move the app widget too
	  */
	void pointerMove(const QPointF &scenePos, Qt::MouseButtons buttonFlags);

	/**
	  * There's no change in pointer. setAppUnderPointer() is called
	  * If app is present, change its zValue so that it can be top most item among app widgets
	  */
	void pointerPress(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);

//	void pointerRelease(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);
	void pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);

	/**
	  * generates real mouse event
	  */
	void pointerWheel(const QPointF &scenePos, int delta = 120);

	/**
	  * This is not standard mouse event. simulate mouse click
	  */
	void pointerClick(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);

private:
	/**
	  * The unique ID of UI client to which this pointer belongs
	  */
	const quint64 uiclientid;
	const QSettings *settings;

	/**
	  * The current viewport
	  */
//	QGraphicsView *gview;

	/**
	  * The pointer name
	  */
	QGraphicsSimpleTextItem *textItem;

	/**
	  * The app widget under this pointer
	  */
	BaseWidget *app;
};


/**
  * The close button icon on the wall
  */
class PixmapCloseButton : public QGraphicsPixmapItem
{
public:
	explicit PixmapCloseButton(const QString resource, QGraphicsItem *parent = 0);
	explicit PixmapCloseButton(const QPixmap pixmap, QGraphicsItem *parent=0) : QGraphicsPixmapItem(pixmap, parent) {}

protected:
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
	bool flag;
};


class PixmapCloseButtonOnScene : public QGraphicsPixmapItem
{
public:
	explicit PixmapCloseButtonOnScene(const QString resource, QGraphicsItem *parent = 0);
	explicit PixmapCloseButtonOnScene(const QPixmap pixmap, QGraphicsItem *parent=0) : QGraphicsPixmapItem(pixmap, parent) {}

protected:
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
	bool flag;
};





class SwSimpleTextItem : public QGraphicsSimpleTextItem
{
public:
	SwSimpleTextItem(int pointSize=0, QGraphicsItem *parent=0);
	~SwSimpleTextItem();

protected:
	void wheelEvent(QGraphicsSceneWheelEvent * event);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

};




class PixmapArrow : public QGraphicsPixmapItem
{
public:
	PixmapArrow(const quint64 uicid, QGraphicsItem *parent=0);
	void setPointerName(const QString &text);

private:
	QGraphicsSimpleTextItem *textItem;
	const quint64 uiclientid;

	/**
	  * the app under this pointer
	  */
	BaseWidget *app;
};



#endif // COMMONITEM_H
