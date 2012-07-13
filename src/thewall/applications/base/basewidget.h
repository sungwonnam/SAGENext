#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QGraphicsWidget>
#include "../../common/commondefinitions.h"

/*
  If you don't make a choice,
  time will make one instead of you.
  */


class QSettings;
class QPropertyAnimation;
class QParallelAnimationGroup;

class SN_ResourceMonitor;
class AppInfo;
class PerfMonitor;
class SN_Priority;
class AffinityInfo;

class SN_SimpleTextItem;
class SN_PolygonArrowPointer;

class SN_BaseWidget : public QGraphicsWidget
{
        Q_OBJECT
//	Q_PROPERTY(qreal priority READ priority WRITE setPriority NOTIFY priorityChanged)
        Q_PROPERTY(Window_State windowState READ windowState WRITE setWindowState)
//	Q_PROPERTY(qreal quality READ quality WRITE setQuality)
        Q_PROPERTY(Widget_Type widgetType READ widgetType WRITE setWidgetType)
        Q_ENUMS(Window_State)
        Q_ENUMS(Widget_Type)

public:
        /*!
          This constructor is required for a plugin to instantiate.
          Don't forget to set _globalAppId, settings, in SN_Launcher for plugin types.
          */
        SN_BaseWidget(Qt::WindowFlags wflags = Qt::Window);

        SN_BaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);

        virtual ~SN_BaseWidget();

        /*!
          Only a user application will have the value >= UserType + BASEWIDGET_USER.
		  So, set QGraphicsItem::UserType + BASEWIDGET_USER for your own custom application.
          This is default.


          A widget which isn't a user application (doesn't inherit SN_BaseWidget) but needs to be interactible with shared pointers is
          QGraphicsItem::UserType + INTERACTIVE_ITEM // such as SN_PartitionBar
         */
        enum { Type = QGraphicsItem::UserType + BASEWIDGET_USER };
        virtual int type() const { return Type;}



        enum Window_State { W_NORMAL, W_MINIMIZED, W_MAXIMIZED, W_HIDDEN, W_ICONIZED };
        Window_State _windowState; /**< app wnidow state */
        inline Window_State windowState() const {return _windowState;}
		inline void setWindowState(Window_State ws) {_windowState = ws;}



        enum Widget_Type { Widget_RealTime, Widget_Image, Widget_GUI, Widget_Web, Widget_Misc };
        inline Widget_Type widgetType() const {return _widgetType;}



        void setSettings(const QSettings *s);

        inline void setRMonitor(SN_ResourceMonitor *rm) {_rMonitor = rm;}

		/*!
		  Noramlly, the globalAppId is assigned and passed to an application's constructor by the launcher.
		  But for the plugins, this is impossible because the launcher can't call plugins constructor directly.
		  So plugins are created without globalAppId then assigned one using this function
		  */
        inline virtual void setGlobalAppId(quint64 gaid) {_globalAppId=gaid;}

        inline quint64 globalAppId() const {return _globalAppId;}

        inline AppInfo * appInfo() const {return _appInfo;}

        inline PerfMonitor * perfMon() const {return _perfMon;}

        inline AffinityInfo * affInfo() {return _affInfo;}

		inline SN_Priority * priorityData() {return _priorityData;}


        inline bool isSchedulable() const {return _isSchedulable;}
        inline void setSchedulable() {_isSchedulable = true;}


		/**
		  This seems only valid on Qt::Window and windowFrameMargins > 0
		  */
//        virtual void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


        /*!
		  The application window will be placed on top of everything if this function is called.
		  (It sets the highest zValue among all app widgets on the on the scene.)
          This is called when ..
		  i) In the handlePointerPress() as a result of SN_PolygonArrowPointer::pointerPress()
		  ii) URL box of the webwidget is clicked.
		  iii) right after the application has started by the launcher
          */
        void setTopmost();

        /*!
          Resizing app window with this. Note that this makes pixel bigger/smaller.

          Widgets with Qt::Window as the window flag should resize by resizeHandleSceneRect().
          So, using this function in those cases is not recommended.
          */
        virtual void reScale(int tick, qreal factor);

        /*!
		  An application window can be resized (or rescaled) using resize handle.
		  handlePointerDrag() will use this..
          This function does nothing but return scene rect of 128x128 on the bottom right corner of the widget.

		  To enable all four corners of the widget,use QRegion.
          */
        virtual QRectF resizeHandleRect() const;




		/**
		  Reimplementing QGraphicsWidget::paint()
		  This will draw infoTextItem followed by window border
		  */
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

		/**
		  Reimplementing QGraphicsWidget::paintWindowFrame()
		  For Qt::Window
		  */
		void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);



        /*!
          If shared pointer calls mouseClick() in response to ui client's mousePress followed by mouseRelease event, then it means theat the real mouse events are not generated.

          In that case, a user can reimplement this function to define widget's behavior in response to mouse click on the widget.
          The default implementation provides real mouse press and release event.
          */
//        virtual void mouseClick(const QPointF &, Qt::MouseButton); // this is taken care of in shared pointer



        /*!
          Sets the proxyWidget as a child widget.
          When a plugin isn't BaseWidget type (it didn't inherit BaseWidget) then create BaseWidget for the plugin
          */
//        virtual void setProxyWidget(QGraphicsProxyWidget *proxyWidget);



        /*!
          How much of my window is actually visible to a user.
		  This is a function of my Z value and colliding widgets.
          */
        QRegion effectiveVisibleRegion() const;




        /*!
          Parameters determining priority are visible region, and recency of interaction.
          It is important to note that priority is calculated whenever this function is called.

          @param current time in msec since Epoch
          */
        qreal priority(qint64 currTimeEpoch = 0);

        int priorityQuantized(qint64 currTimeEpoch = 0, int bias = 1);




        /*!
          This is a placeholder.
          A schedulable widget must reimplement this function to make the application thread do necessary operations that can reflect the quality set by this function.
          @param newQuality is an absolute value.
          */
        virtual int setQuality(qreal newQuality) {_quality = newQuality; return 0;}

		/*!
		  This function is called by a scheduler to adjust resource consumption of the widget.
          Note that @param adjust is the amount of adjustment, not the absolute quality value.
		  */
		int adjustQuality(qreal adjust) {return setQuality(_quality + adjust);}

        /*!
          Returns the quality enforeced by the scheduler
          */
        inline qreal demandedQuality() const {return _quality;}

        /*!
          This is dummy function. A schedulable widget should reimplement this properly.
          The function should return the ratio of the current performance to the performance the application is expecting.
          */
        virtual qreal observedQuality_Rq() const;

        /*!
          This is dummy function. A schedulable widget should reimplement this properly.
          The function should return the ratio of the current performance to the performance the scheduler sets for this application.
          So, The return value indicates how much this application is obeying the scheduler's demand.
          */
        virtual qreal observedQuality_Dq() const;




        /*!
          is called every 1 second by timerEvent. Reimplement this to update infoTextItem to display real-time information.
          draw/hideInfo() sets/kills the timer
          */
        virtual void updateInfoTextItem();






//        inline QParallelAnimationGroup *animGroup() {return _parallelAnimGroup;}


        /*!
          Updates _lastTouch with the current time.
          */
//        void setLastTouch();

//        inline qint64 lastTouch() const {return _lastTouch;}



		/**
		  If your application needs pointer hovering feature, set this
		  */
		inline void setRegisterForMouseHover(bool v = true) {_registerForMouseHover = v;}

		/**
		  A pointer calls this function to know if this widget is listening for hovering
		  */
		inline bool isRegisteredForMouseHover() const {return _registerForMouseHover;}



		/**
		  If SN_BaseWidget is registered for hover listener widget, (by settings setRegisterForMouseHover()),
          then the SN_PolygonArrowPointer will call this function whenever a shared pointer hovers on this widget.

		  @param 'isHovering' is true only when the 'pointer' is hovering on this widget.
		  @param 'point' denotes the 'pointer's current xy coordinate in this item's local coordinate.
		  */
		virtual void handlePointerHover(SN_PolygonArrowPointer */*pointer*/, const QPointF & /*point*/, bool /* isHovering */) {}

		/**
		  SN_PolygonArrowPointer::pointerPress() calls this function upon receiving mousePressEvent from uiclient/sn_pointerui (sagenextPointer.app)
		  Default implementation calls setTopmost() that sets this widget's z value the highest among all other application windows.

          If press is occurred on the resizing rectangle, the _isResizing is set to true.
          Otherwise, _isMoving is set to true.

		  Note that you don't have to reimplement this function to receive the system's mouse click event.
		  The system's mouse click event (pressEvent immediately followed by releaseEvent) will be generated (and passed to the viewport widget) by
          SN_PolygonArrowPointer::pointerClick() upon receiving mouseClick event from the uiclient.

		  @param pointer is the shared pointer who has mouse pressed this widget
		  @param point is the position of the point being pressed in this widget's local coordinate
		  */
		virtual void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn = Qt::LeftButton);

		/*!
          This function is called by the SN_PolygonArrowPointer.
		  If _isResizing was set in the handlePointerPress() then the widget/windown will be rescaled/resized in this function.
		  */
		virtual void handlePointerRelease(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn = Qt::LeftButton);


        /*!
          SN_SharedPointer generates the real system mouse events (press followed by release) upon receiving POINTER_CLICK from UiServer.
          So, this function usually doesn't need to be reimplemented as long as a GUI component of your widget needs to respond system's mouse click event.
          Default implementation does nothing but returning false.

          Return TRUE in this function if you don't want the system mouse events being generated by users' mouse click on this widget.

          @return return TRUE to prevent SN_PolygonArrowPointer from generating mouse events
          */
        virtual bool handlePointerClick(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn) {
            Q_UNUSED(pointer);
            Q_UNUSED(point);
            Q_UNUSED(btn);
            return false;
        }

		/*!
          Actual system mouse event can't be used when it comes to mouse dragging because if multiple users do this simultaneously, system will be confused and leads to unexpected behavior.
          So this should be implemented in child class manually. Default implementation provides move/resize.

		  This function is called in SN_PolygonArrowPointer::pointerMove() when btnFlags & Qt::LeftButton evaluates to 1

		  @param pointer The pointer to the object dragging this widget
		  @param point pointer's xy coordinate in this widget's local coordinate
          */
        virtual void handlePointerDrag(SN_PolygonArrowPointer *pointer ,const QPointF &point, qreal pointerDeltaX, qreal pointerDeltaY, Qt::MouseButton button = Qt::LeftButton, Qt::KeyboardModifier modifier = Qt::NoModifier);


		/**
		  When a pointer is leaving (being deleted) it has to remove itself from the map that this widget monitors
		  */
		inline void removePointerFromPointerMap(SN_PolygonArrowPointer *pointer) {
			_pointerMap.remove(pointer);
		}

protected:
		/*!
		  OpenGL viewport is available ?
		  */
		bool _useOpenGL;

        quint64 _globalAppId; /**< Unique identifier */

        const QSettings *_settings; /**< Global configuration parameters */


        Widget_Type _widgetType; /**< widget type */
		inline void setWidgetType(Widget_Type wt) {_widgetType = wt;}

		/*!
          To display information in appInfo, perfMonitor, and affInfo
          */
        SN_SimpleTextItem *infoTextItem;


        /*!
         * Used to display app info overlay
         */
        AppInfo *_appInfo; /**< app name, frame dimension, rect */


        bool _showInfo; /**< flag to toggle show/hide info item */



		/*!
		  Interaction monitor
		  */
		SN_Priority *_priorityData;


        /*!
          PerfMonitor class is only meaningful with animation widget
          */
        PerfMonitor *_perfMon;


        /*!
          Instantiated when RailawareWidget
          */
        AffinityInfo *_affInfo;




        /*!
          When was this widget touched last time. This value is updated in mousePressEvent().
          It is to keep track user interaction recency on this widget
          */
//        qint64 _lastTouch; // long long int


        SN_ResourceMonitor *_rMonitor; /**< A pointer to the global resource monitor object */





        /*!
          0 <= quality <= 1.0
          How to calculate quality is differ by an application.
		  So it has to be implemented (by reimplementing SN_BaseWidget::setQuality()) by app developer.
          */
        qreal _quality;





        /*!
          mouse right click on widget opens context menu provided by Qt
          This is protected because derived class could want to add its own actions.
		  This is not used with shared pointers (needs console mouse)
          */
        QMenu *_contextMenu;


		/*!
		  This is to know if any SN_PolygonArrowPointer is currently hovering on my boundingRect()
		  Refer MouseHoverExample plugin to see how to use this
		  */
		QMap<SN_PolygonArrowPointer *, QPair<QPointF, bool> > _pointerMap;



        /*!
          If a pointer left button is pressed on the resizehandle, this flag is set
          and mouse dragging will move the window.

          Reimplment handlePointerPress() if you want to provide custom interaction. (See WebWidget & MandelbrotExample)
          */
        bool _isMoving;

        /*!
          If a pointer left button is pressed on the resizehandle, this flag is set
          and mouse dragging will resize/rescale window
          */
        bool _isResizing;


        /*!
          If pointer is draggin while _isResizing is set
          then this rectangle will be shown.
          Upon pointer releasing, window will be resized/rescaled based on the size of this rectangle
          */
        QGraphicsRectItem *_resizeRectangle;



        /*!
          Derived class should implement paint()
          */
//	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) = 0;

        /*!
          * timeout every 1 sec to update info of appInfo, perfMon, affInfo
          */
        void timerEvent(QTimerEvent *);

        /*!
		  setTransformOriginPoint() with center of the widget
		  Because of this, your widget's transformationOrigin is center
          */
//        virtual void resizeEvent(QGraphicsSceneResizeEvent *event);

        /*!
          * Just changes z value and recency of interaction

		  The default implementation makes this widget mouseGrabber (QGraphicsScene::mouseGrabberItem())
		  Only the mouseGrabber item can receive move/release/doubleClick events.

		  Reimplementing this makes the event->accept() which makes this widget the mousegrabber
          */
//        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

		/*!
		  QGraphicsItem::mouseReleaseEvent() handles basic interactions such as selection and moving upon receiving this event.
		  By reimplement this function with empty definition, those actions won't be enabled through console mouse.
		  This makes moving this item with console mouse incorrect.

		  This is intended. We focus on interactions through shared pointers.
		  */
//		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);

//        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

        /*!
          * Triggers maximize/restore window
          */
        virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

        /*!
          * pops up context menu this->_contextMenu upon receiving system's mouse right click.
		  * This won't work with shared pointers.
          */
        virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

        /*!
          * It calls reScale() incrementally for Qt::Widget.
          */
        virtual void wheelEvent(QGraphicsSceneWheelEvent *event);


private:
		QAction *_showInfoAction; /**< drawInfo() */
        QAction *_hideInfoAction; /**< hideInfo() */
        QAction *_minimizeAction; /**< minimize() */
        QAction *_restoreAction; /**< restore() */
        QAction *_maximizeAction; /**< maximize() */
        QAction *_closeAction; /**< fadeOutClose() */


        QPropertyAnimation *pAnim_pos; /**< Animation on an item's position */
        QPropertyAnimation *pAnim_scale; /**< Animation on an item's scale */
		QPropertyAnimation *pAnim_size; /**< Animation on widget's size */
        QParallelAnimationGroup *_parallelAnimGroup; /**< It contains pAnim_pos, pAnim_size, and pAnim_scale and used to execute both in parallel for max/minimize, restore visual effect */

        /*!
          * Animation on opacity property
          * When the animation finishes, this->close() is called
          */
        QPropertyAnimation *pAnim_opacity;

        /*!
          * startTimer() returns unique timerID.
          */
//        int _timerID;


		/*!
		  Set this true to enable the widget to listen on mouse hovering.
		  Reimplement handlePointerHover()
		  */
		bool _registerForMouseHover;


		/*!
		  The size (in pixel) of window frame
		  */
		int _bordersize;



        /*!
          is schedulable widget ?
          */
        bool _isSchedulable;


		/*!
		  This is called in the constructor.
		  Please note that OpenGL context (when using OpenGL viewport) is accessible only after this function returns.
		  */
		void init();



		/*!
          The effective visible region of the widget (this is used to calculate priority)
          It changes whenever the scale and zValue changes

          Also, it should trigger other widgets effective region be changed
          */
//        QRegion _effectiveVisibleRegion;



        void createActions();

signals:

public slots:
        virtual void drawInfo(); /**< It sets the showInfo boolean, starts timer */
        virtual void hideInfo(); /**< It resets the showInfo boolean, kills timer */
        virtual void minimize(); /**< minimizes app window. AnimationWidget could adjust frame rate or implement frame skipping. */
        virtual void restore(); /**< restores from minimized/maximized state */
        virtual void maximize(); /**< maximizes app window */

        /*!
          This implements opacity animation (pAnim_opacity)
          Once the animation emit finishes(), close() should be called with Qt::WA_DeleteOnClose set
        */
        virtual void fadeOutClose();
};


#endif // BASEWIDGET_H
