#ifndef SN_SHAREDPOINTER_H
#define SN_SHAREDPOINTER_H

#include <QGraphicsPolygonItem>
#include <QGraphicsWidget>

#include "commondefinitions.h"

class SN_BaseWidget;
class SN_TheScene;
class UiMsgThread;

class QSettings;
class QGraphicsView;
class QFile;

/**
  A selection rectangle when the pointer selects a set of basewidget with dragging (Right button)
  */
class SelectionRectangle : public QGraphicsWidget
{
	Q_OBJECT
public:
	explicit SelectionRectangle(QGraphicsItem *parent = 0);
};





/**
  When a user shares his pointer through ui client,
  This class is instantiated and added to the scene.


  The sequence of messges result from each mouse action
  click     : PRESS -> CLICK
  drag      : PRESS -> DRAGGING -> RELEASE
  move      : MOVING
  dbl click : PRESS -> CLICK -> DBLCLICK


  This means, for example, if user double click with his shared pointer then,
  pointerPress() , pointerClick() , and pointerDoubleClick() will be called consecutively

  */
class SN_PolygonArrowPointer : public QGraphicsPolygonItem
{
public:
	SN_PolygonArrowPointer(const quint32 uicid, UiMsgThread *msgthread, const QSettings *, SN_TheScene *scene, const QString &name = QString(), const QColor &c = QColor(Qt::red), QFile *scenarioFile=0, QGraphicsItem *parent=0);
	~SN_PolygonArrowPointer();

	void setPointerName(const QString &text);
	inline QString name() const {return _textItem->text();}

	inline quint32 id() const {return _uiclientid;}

	inline QColor color() const {return _color;}
	
	inline void setDrawing(bool b=true) {_isDrawing = b;}
	void setErasing(bool b=true);

    /*!
      For running a scenario
      */
	void pointerOperation(int opcode, const QPointF &scenepos, Qt::MouseButton btn, int delta, Qt::MouseButtons btnflags);


    inline void setTrapWidget(SN_BaseWidget *w) {_trapWidget = w;}


	/*!
	  Upon receiving RESPOND_STRING from a user, the pointer will set text to _guiItem
	  */
	void injectStringToItem(const QString &str);

        /**
          This is called by pointerPress()
          It sets a user widget (type() >= QGraphicsItem::UserType + 12) under the pointer.
          */
	bool setAppUnderPointer(const QPointF &scenePosOfPointer);

	inline SN_BaseWidget * appUnderPointer() {return _basewidget;}

        /**
          Moves pointer itself and ...
          there's app under the pointer then move the app widget too (left button)
		  resize selection rectangle if right button
          */
	virtual void pointerMove(const QPointF &scenePos, Qt::MouseButtons buttonFlags, Qt::KeyboardModifier modifier = Qt::NoModifier);

        /**
		  This is called by UiServer upon receiving SAGENext::POINTER_PRESS.

          The setAppUnderPointer() is called if left button is pressed.
		  Initiate selection rectangle if right button.
          */
	virtual void pointerPress(const QPointF &scenePos, Qt::MouseButton button, Qt::KeyboardModifier modifier = Qt::NoModifier);


		/**
          Note that, only DRAGGING will generate SAGENext::POINTER_RELEASE.

		  Both left and right release can be sent if the manhattan distance b/w pressed pos and released pos is greater than 3.
		  So, this is triggered at the end of mouseDragging by the client.

		  LeftRelease can mean the end of widget moving or resizing.
		  RightRelease can mean the end position of selection rectangle.
		  
		  Note that, _basewidget/_specialItem will be reset in this function.
		  */
	virtual void pointerRelease(const QPointF &scenePos, Qt::MouseButton button, Qt::KeyboardModifier modifier = Qt::NoModifier);

		/**
		  This is called by UiServer upon receiving SAGENext::POINTER_CLICK.
		  Note that, POINTER_CLICK always follows the POINTER_PRESS.
		  
          This handler simulates mouse click by sending mousePressEvent followed by mouseReleaseEvent
		  unless SN_BaseWidget::handlePointerClick() returns TRUE.
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
	SN_TheScene *_scene;

	/*!
	  Pointer to my UiMsgThread
	  */
	UiMsgThread *_uimsgthread;

        /**
          * The unique ID of UI client to which this pointer belongs
          */
	const quint32 _uiclientid;

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

    /*!
      A widget that traps this pointer in it.
      */
    SN_BaseWidget *_trapWidget;

        /**
          The app widget under this pointer
          This is set when pointerPress (left button) is called
          */
	SN_BaseWidget *_basewidget;

	/**
	  The graphics item under this pointer. This is to keep track general items on the scene that can be interacted with shared pointer
	  Such as SN_PartitonBar
      */
	QGraphicsItem *_specialItem;


	QGraphicsWidget *_guiItem;

		
		/**
		  A rectangle when mouse draggin on empty space
		  */
	SelectionRectangle *_selectionRect;
		
		/**
		  a set of basewidgtes selected by selection rectangle
		  */
	QGraphicsItemGroup *_selectedApps;

		/**
		  To record all the actions that users gave
		  */
	QFile *_scenarioFile;
		
	bool _isDrawing;
	bool _isErasing;


		/**
		  Returns the view on which the event occured.
		  pos is on the scene coordinate
		  */
	QGraphicsView * eventReceivingViewport(const QPointF scenePos);
};


#endif // SN_SHAREDPOINTER_H
