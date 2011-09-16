#ifndef COMMONITEM_H
#define COMMONITEM_H

#include <QtGui>

class BaseWidget;


/**
  An widget will be moved to this button to be removed on the scene
  */
class WidgetRemoveButton : public QGraphicsPixmapItem {
public:
        explicit WidgetRemoveButton(QGraphicsItem *parent = 0);
        ~WidgetRemoveButton() {}

protected:
        void dragEnterEvent(QGraphicsSceneDragDropEvent *event);

//	void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);

        void dropEvent(QGraphicsSceneDragDropEvent *event);
};









/**
  When a user shares his pointer through ui client,
  This class is instantiated and added to the scene
  */
class PolygonArrow : public QGraphicsPolygonItem {
public:
        PolygonArrow(const quint64 uicid, const QSettings *, const QColor c, QGraphicsItem *parent=0);
        ~PolygonArrow();

        void setPointerName(const QString &text);

        /**
          This is called by pointerPress()
          It sets a user widget (QGraphicsItem::UserType + 2) under the pointer
          */
        bool setAppUnderPointer(const QPointF scenePosOfPointer);

        inline BaseWidget * appUnderPointer() {return app;}

        /**
          * Moves pointer itself
          * If dragging and there's app under the pointer then move the app widget too
          */
        virtual void pointerMove(const QPointF &scenePos, Qt::MouseButtons buttonFlags);

        /**
          * setAppUnderPointer() is called
          */
        virtual void pointerPress(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);

		/**
          * simulate mouse click by sending mousePressEvent followed by mouseReleaseEvent
          */
        virtual void pointerClick(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);




        /**
           This function generates and sends real doubleclick event to the viewport widget
          */
        virtual void pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags);

        /**
          * This function generates and sends real mouse wheel event to the viewport widget
          */
        virtual void pointerWheel(const QPointF &scenePos, int delta = 120);

private:
        /**
          * The unique ID of UI client to which this pointer belongs
          */
        const quint64 uiclientid;


        const QSettings *settings;

        /**
          * The pointer name
          */
        QGraphicsSimpleTextItem *textItem;

        /**
          The app widget under this pointer
          This is set when pointerPress is called
          */
        BaseWidget *app;

		/**
		  Returns the view on which the event occured.
		  pos is on the scene coordinate
		  */
		QGraphicsView * eventReceivingViewport(const QPointF scenePos);
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


/**
  The same pixmapclose button
  But this is using scene object
  */
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
