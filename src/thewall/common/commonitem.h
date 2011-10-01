#ifndef COMMONITEM_H
#define COMMONITEM_H

#include <QtGui>

class BaseWidget;


/**
  When a user shares his pointer through ui client,
  This class is instantiated and added to the scene
  */
class SAGENextPolygonArrow : public QGraphicsPolygonItem {
public:
	SAGENextPolygonArrow(const quint64 uicid, const QSettings *, const QString &name = QString(), const QColor &c = QColor(Qt::red), QFile *scenarioFile=0, QGraphicsItem *parent=0);
	~SAGENextPolygonArrow();

	void setPointerName(const QString &text);
	inline QString name() const {return _textItem->text();}

	inline quint64 id() const {return _uiclientid;}

	inline QColor color() const {return _color;}

	void pointerOperation(int opcode, const QPointF &scenepos, Qt::MouseButton btn, int delta, Qt::MouseButtons btnflags);

        /**
          This is called by pointerPress()
          It sets a user widget (type() >= QGraphicsItem::UserType + 12) under the pointer.
          */
        bool setAppUnderPointer(const QPointF scenePosOfPointer);

        inline BaseWidget * appUnderPointer() {return _basewidget;}

        /**
          Moves pointer itself and ...
          there's app under the pointer then move the app widget too (left button)
		  resize selection rectangle if right button
          */
        virtual void pointerMove(const QPointF &scenePos, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
          This doesn't generate any mouse event. This is used to know the start of mouse dragging

          The setAppUnderPointer() is called if left button.
		  Initiate selection rectangle if right button.
          */
        virtual void pointerPress(const QPointF &scenePos, Qt::MouseButton button, Qt::KeyboardModifier modifier = Qt::NoModifier);


		/**
		  Both left and right release can be sent if the manhattan distance b/w pressed pos and released pos is greater than 3.
		  So, this is triggered at the end of mouseDragging by the client.

		  LeftRelease can mean app droping
		  RightRelease can mean the end position of selection rectangle
		  */
		virtual void pointerRelease(const QPointF &scenePos, Qt::MouseButton button, Qt::KeyboardModifier modifier = Qt::NoModifier);

		/**
          simulate mouse click by sending mousePressEvent followed by mouseReleaseEvent

		  press
		  release
          */
        virtual void pointerClick(const QPointF &scenePos, Qt::MouseButton button,  Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
           This function generates and sends real doubleclick event to the viewport widget.

		   press
		   release
		   doubleclick
		   release

          */
        virtual void pointerDoubleClick(const QPointF &scenePos, Qt::MouseButton button,  Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
           This function generates and sends real mouse wheel event to the viewport widget
		   An item doesn't have to be the mousegraber to receive this event
          */
        virtual void pointerWheel(const QPointF &scenePos, int delta = 120, Qt::KeyboardModifier modifier = Qt::NoModifier);

private:
        /**
          * The unique ID of UI client to which this pointer belongs
          */
        const quint64 _uiclientid;

		/**
		  The global settings
		  */
        const QSettings *_settings;

        /**
          * The pointer name
          */
        QGraphicsSimpleTextItem *_textItem;

		/**
		  THe pointer color
		  */
		QColor _color;

        /**
          The app widget under this pointer
          This is set when pointerPress (left button) is called
          */
        BaseWidget *_basewidget;

		/**
		  The graphics item under this pointer. This is to keep track general items on the scene
		  */
		QGraphicsItem *_item;

		/**
		  To record all the actions that users gave
		  */
		QFile *_scenarioFile;

		/**
		  Returns the view on which the event occured.
		  pos is on the scene coordinate
		  */
		QGraphicsView * eventReceivingViewport(const QPointF scenePos);
};






/**
  * The general button pixmap that emits signal when clicked
  */
class SAGENextPixmapButton : public QGraphicsWidget
{
	Q_OBJECT
public:
	/**
	  creates button with resource
	  */
	explicit SAGENextPixmapButton(const QString &res_normal, qreal desiredWidth = 0.0, const QString &label = QString(), QGraphicsItem *parent = 0);

	/**
	  creates button with pixmap
	  */
	explicit SAGENextPixmapButton(const QPixmap &pixmap_normal, qreal desiredWidth = 0.0, const QString &label = QString(), QGraphicsItem *parent=0);

	/**
	  creates button with custom size
	  */
	explicit SAGENextPixmapButton(const QString &res_normal, const QSize &size = QSize(), const QString &label = QString(), QGraphicsItem *parent=0);

	~SAGENextPixmapButton();

private:
	QPixmap _normalPixmap;
	QPixmap _hoveredPixmap;

	void _attachLabel(const QString &labeltext, QGraphicsItem *parent);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
	void clicked();
};








/**
  This is used to display text on gray rectangle
  */
class SAGENextSimpleTextItem : public QGraphicsSimpleTextItem
{
public:
	SAGENextSimpleTextItem(int pointSize=0, const QColor &fontcolor = QColor(Qt::black), const QColor &bgcolor = QColor(Qt::gray), QGraphicsItem *parent=0);
	~SAGENextSimpleTextItem();

private:
	QColor _fontcolor;
	QColor _bgcolor;

protected:
	void wheelEvent(QGraphicsSceneWheelEvent * event);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

};


#endif // COMMONITEM_H
