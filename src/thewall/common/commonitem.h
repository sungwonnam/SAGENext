#ifndef COMMONITEM_H
#define COMMONITEM_H

#include <QtGui>

class BaseWidget;


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
          It sets a user widget (type() >= QGraphicsItem::UserType + 12) under the pointer.
          */
        bool setAppUnderPointer(const QPointF scenePosOfPointer);

        inline BaseWidget * appUnderPointer() {return app;}

        /**
          * Moves pointer itself
          * If dragging and there's app under the pointer then move the app widget too
          */
        virtual void pointerMove(const QPointF &scenePos, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
          This doesn't generate any mouse event. The setAppUnderPointer() is called.
		  Only left press is sent from uiclient.
          */
        virtual void pointerPress(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);


		/**
		  Both left and right release can be sent if the manhattan distance b/w pressed pos and released pos is greater than 3.
		  So, this is triggered at the end of mouseDragging by the client.

		  LeftRelease can mean app droping
		  RightRelease can mean the end position of selection rectangle
		  */
		virtual void pointerRelease(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);

		/**
          simulate mouse click by sending mousePressEvent followed by mouseReleaseEvent

		  press
		  release
          */
        virtual void pointerClick(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
           This function generates and sends real doubleclick event to the viewport widget.

		   press
		   release
		   doubleclick
		   release

          */
        virtual void pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton button, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
           This function generates and sends real mouse wheel event to the viewport widget
		   An item doesn't have to be the mousegraber to receive this event
          */
        virtual void pointerWheel(const QPointF &scenePos, int delta = 120, Qt::KeyboardModifier modifier = Qt::NoModifier);

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
		  The graphics item under this pointer. This is to keep track general items on the scene
		  */
		QGraphicsItem *_item;

		/**
		  Returns the view on which the event occured.
		  pos is on the scene coordinate
		  */
		QGraphicsView * eventReceivingViewport(const QPointF scenePos);
};






/**
  * The general button pixmap that emits signal when clicked
  */
class PixmapButton : public QGraphicsWidget
{
	Q_OBJECT
public:
	/**
	  creates button with resource
	  */
	explicit PixmapButton(const QString &res, qreal desiredWidth = 0.0, const QString &label = QString(), QGraphicsItem *parent = 0);

	/**
	  creates button with image
	  */
	explicit PixmapButton(const QPixmap &pixmap, qreal desiredWidth = 0.0, const QString &label = QString(), QGraphicsItem *parent=0);

	/**
	  creates button with custom size
	  */
	explicit PixmapButton(const QString &res, const QSize &size = QSize(), const QString &label = QString(), QGraphicsItem *parent=0);

	~PixmapButton();

private:
	void _attachLabel(const QString &labeltext, QGraphicsItem *parent);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
	void clicked();
};





/**
  An widget will be moved to this button to be removed on the scene
  */
class WidgetRemoveButton : public PixmapButton
{
	Q_OBJECT
public:
	explicit WidgetRemoveButton(QGraphicsItem *parent = 0);
	~WidgetRemoveButton() {}
};




/**
  This is used to display text on gray rectangle
  */
class SwSimpleTextItem : public QGraphicsSimpleTextItem
{
public:
	SwSimpleTextItem(int pointSize=0, QGraphicsItem *parent=0);
	~SwSimpleTextItem();

protected:
	void wheelEvent(QGraphicsSceneWheelEvent * event);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

};


#endif // COMMONITEM_H
