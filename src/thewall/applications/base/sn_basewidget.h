#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QGraphicsWidget>
#include "common/sn_commondefinitions.h"

class QSettings;

//extern QSettings *ptrSettings;


class QPropertyAnimation;
class QParallelAnimationGroup;

class SN_ResourceMonitor;
class SN_AppInfo;
class SN_PerfMonitor;
class SN_Priority;
class SN_AffinityInfo;

class SN_SimpleTextItem;
class SN_PolygonArrowPointer;

/*!
 * \brief The SN_BaseWidget class is the base class of all user applications.
 *
 * It is important to note that you must call resize() in your class once your know the native size of your widget.
 *
 * To define your own custom interaction, reimplement SN_BaseWidget::handlePointerXXXX() functions instead of QGraphicsItem::mouseXXXXEvent().
 */
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
         * \brief The default constructor is required for a plugin to instantiate.
         *
         * A plugin can't determine its _globalAppId by itself and also a plugin
         * can't see SAGENext's global QSettings object. So these two have to be
         * set in SN_Launcher.
         *
         * \param wflags sets the type of the widget. Qt::Window type widget can
         * change its native resolution (e.g. web browser) whereas Qt::Widget type
         * widget never changes its native resolution (e.g. image viewer).
         */
        SN_BaseWidget(Qt::WindowFlags wflags = Qt::Window);

        SN_BaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);

        virtual ~SN_BaseWidget();

        /*!
         * \brief This sets the type of widget that will handled differently by SN_PolygonArrowPointer.
         *
         * Only a user application will have the value >= QGraphicsItem::UserType + BASEWIDGET_USER.
         * So, set QGraphicsItem::UserType + BASEWIDGET_USER for your own custom application.
         * This is default if you don't specify anything.
         * A widget which is not a user application (it doesn't inherit SN_BaseWidget) but needs to be
         * interactible with shared pointers should be QGraphicsItem::UserType + INTERACTIVE_ITEM.
         * An example of this type of widget is SN_PartitionBar
         */
        enum {
            Type = QGraphicsItem::UserType + BASEWIDGET_USER
        };
        virtual int type() const { return Type;}

        /*!
         * \brief The Window_State enum tells a state of the widget's window geometry
         */
        enum Window_State { 
            W_NORMAL,

            /*!
              A widget is minimized when it's dragged to the minimizeBar on the bottom of the wall
              */
            W_MINIMIZED,

            /*!
              pointer doubl click will maximizes the widget
              */
            W_MAXIMIZED,

            /*!
              not used
              */
            W_HIDDEN,

            /*!
              not used
              */
            W_ICONIZED
        };
        Window_State _windowState; /*!< A widget's wnidow state */
        inline Window_State windowState() const {return _windowState;}
		inline void setWindowState(Window_State ws) {_windowState = ws;}



        /*!
         * \brief The Widget_Type enum tells a type of a widget.
         */
        enum Widget_Type {
            /*!
              A streaming widget such as SAGE application
              */
            Widget_RealTime = 0x01,

            /*!
              A widget only shows a static image
              */
            Widget_Image = 0x02,

            /*!
              A widget designed to be heavily interacted by user
              */
            Widget_GUI = 0x04,

            /*!
              A widget shows web contents such as SN_WebWidget
              */
            Widget_Web = 0x08,

            /*!
              Everything else
              */
            Widget_Misc = 0x10
        };
        Q_DECLARE_FLAGS(Widget_Types, Widget_Type)
        inline Widget_Type widgetType() const {return _widgetType;}


        /*!
         * \brief _isSelectedByPointer tells if the widget has been selected through a mouse right-click
         */
        bool _isSelectedByPointer;
        inline void setSelectedByPointer(bool b=true) {_isSelectedByPointer = b;}
        inline bool selectedByPointer() const {return _isSelectedByPointer;}

        inline bool isMoving() const {return _isMoving;}
        inline bool isResizing() const {return _isResizing;}

        void setSettings(const QSettings *s);

        inline void setRMonitor(SN_ResourceMonitor *rm) {_rMonitor = rm;}

		/*!
          \brief Sets the _globalAppId of the widget.

		  Noramlly, the globalAppId is assigned and passed to an application's constructor by the SN_Launcher.
		  But for the plugins, this is impossible because the launcher can't call the plugin's constructor directly.
		  So plugins are created without globalAppId then assigned one using this function by the SN_Launcher.
		  */
        virtual void setGlobalAppId(quint64 gaid);

        inline quint64 globalAppId() const {return _globalAppId;}

        inline SN_AppInfo * appInfo() const {return _appInfo;}

        inline SN_PerfMonitor * perfMon() const {return _perfMon;}

        inline SN_AffinityInfo * affInfo() {return _affInfo;}

		inline SN_Priority * priorityData() {return _priorityData;}

        inline bool isShowInfo() const {return _showInfo;}


        inline bool isSchedulable() const {return _isSchedulable;}
        inline void setSchedulable() {_isSchedulable = true;}


		/*!
		  This seems only valid on Qt::Window and windowFrameMargins > 0
		  */
//        virtual void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


        /*!
          \brief Makes this widget to be top of all other widgets.

		  The application window will be placed on top of everything if this function is called
		  by setting the highest zValue among all user widgets on the on the scene.
          This function can be called when ...
		  i) In the handlePointerPress() as a result of SN_PolygonArrowPointer::pointerPress()
		  ii) The URL box of the SN_WebWidget is clicked.
		  iii) Right after the application has started by the SN_Launcher
          */
        void setTopmost();

        /*!
          \brief Resizes Qt::Widget type widget window with this.

          Note that this doesn't change the native resolution of the widget.
          Qt::Window type widgets should resize by resize().
          */
        virtual void reScale(int tick, qreal factor);

        /*!
		  An application window can be resized (or rescaled) using resize handle.
		  handlePointerDrag() will use this..
          This function does nothing but return scene rect of 128x128 on the bottom right corner of the widget.

		  To enable all four corners of the widget,use QRegion.
          */
        virtual void resizeHandleRect();




		/*!
		  \brief Reimplementing QGraphicsWidget::paint()

		  This will draw the infoTextItem followed by the window's border
		  */
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

		/*!
		  \brief Reimplementing QGraphicsWidget::paintWindowFrame()

		  This is for Qt::Window type widgets.
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
         * \brief computeEVS calls effectiveVisibleRegion() to get the size of visible area of the widget.
         *
         * The size of the effective visible area of the widget is stored in _EVS
         *
         * \sa effectiveVisibleRegion()
         */
        void computeEVS();

        /*!
         * \brief EVS
         * \return effective visible size of the widget
         */
        quint64 EVS() const {return _EVS;}


        /*!
         * \brief priority The priority value stored in SN_Priority is returned.
         * \param currTimeEpoch The current time in msec since Epoch
         * \return the priority value of the widget.
         *
         * \sa SN_Priority::priority()
         */
        qreal priority(qint64 currTimeEpoch = 0);

        /*!
         * \brief This function is deprecated.
         * \param currTimeEpoch
         * \param bias
         * \return
         * \sa SN_AbstractScheduler::configureRail()
         */
        int priorityQuantized(qint64 currTimeEpoch = 0, int bias = 1);


        /*!
          This is a placeholder.
          A schedulable widget must reimplement this function to make the application thread do necessary operations that can reflect the quality set by this function.
          \param newQuality is an absolute value.
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
          \brief This function is called every 1 second by timerEvent.
          Reimplement this to update infoTextItem to display real-time information.
          draw/hideInfo() sets/kills the timer
          */

        /*!
         * \brief displays widget's information.
         *
         * This function is called by SN_ResourceMonitor::refresh()
         */
        virtual void updateInfoTextItem();




//        inline QParallelAnimationGroup *animGroup() {return _parallelAnimGroup;}


        /*!
          Updates _lastTouch with the current time.
          */
//        void setLastTouch();

//        inline qint64 lastTouch() const {return _lastTouch;}




        /*!
         * \brief Sets whether the widget will handle pointer hovering.
         *
         * Call this function in your constructor if you want to handle pointer hover.
         *
         * \param v TRUE to handle pointer hovering
         * \sa src/thewall/applications/plugins/MouseHoverExample
         */
		inline void setRegisterForMouseHover(bool v = true) {_registerForMouseHover = v;}


        /*!
         * \brief
         *
         * This function is called by SN_Launcher when the widget is about to added to the scene.
         * If this function returns TRUE, then SN_Launcher will add the widget to the scene's list
         * of SN_TheScene::hoverAcceptingApps container.
         *
         * \return TRUE if this widget is handling pointer hover
         */
		inline bool isRegisteredForMouseHover() const {return _registerForMouseHover;}



        /*!
         * \brief This function is used to handle pointer hover.
         *
         * This function is called in SN_PolygonArrowPointer::pointerMove()
         * if the widget set _registerForMouseHover to TRUE.
         * This function does nothing by default.
         *
         * \param pointer The pointer instance who is hover on this widget and has called this function
         * \param point The pointer's position in this widget's coordinate
         * \param isHovering TRUE if the pointer is hovering on this widget
         */
		virtual void handlePointerHover(SN_PolygonArrowPointer *pointer, const QPointF & point, bool isHovering) {
            /* do nothing by default */
            Q_UNUSED(pointer);
            Q_UNUSED(point);
            Q_UNUSED(isHovering);
        }


        /*!
         * \brief This function is used to handle pointer press and called in SN_PolygonArrowPointer::pointerPress()
         *
         * SN_PolygonArrowPointer::pointerPress() calls this function upon receiving mouse press from a user.
         * Default implementation calls setTopmost() that sets this widget's z value the highest among all other widgets,
         * and recalculates the widget's resize handle rectangles by calling the resizeHandleRect().
         * If the pointer is pressed on one of those rectangles then _isResizing is set to TRUE so that the widget can
         * later be resized or rescaled properly when handlePointerRelease() is called.
         *
         * Note that this is called when a user does mouse doubleclick because the sequence that triggers the dbl click
         * is mouse press -> mouse dbl click
         *
         * \param pointer The pointer who pressed this widget
         * \param point The pressed position in this widget's local coordinate
         * \param btn
         * \sa SN_PolygonArrowPointer::pointerPress(), handlePointerDrag(), handlePointerRelease(), resizeHandleRect()
         */
		virtual void handlePointerPress(SN_PolygonArrowPointer *pointer, const QPointF &point, Qt::MouseButton btn = Qt::LeftButton);

        /*!
         * \brief This function is used to handle pointer release and called in SN_PolygonArrowPointer::pointerRelease()
         *
         * SN_PolygonArrowPointer::pointerRelease() calls this function upon receiving mouse release from a user.
         * Default implementation does nothing but finalizing resize/rescale if _isResizing was set before.
         *
         * \param pointer The pointer who release on this widget
         * \param point The released position in this widget's local coordinate
         * \param btn
         * \sa SN_PolygonArrowPointer::pointerRelease(), handlePointerPress()
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
         * \brief This value is set automatically if the viewport is a OpenGL widget.
         */
		bool _useOpenGL;

        /*!
         * \brief _globalAppId is a unique identifier of a widget.
         *
         * The value is automatically determined by SN_Launcher.
         *
         * \sa SN_Launcher::_getUpdatedGlobalAppId()
         */
        quint64 _globalAppId;

        /*!
         * \brief _settings is the instance of QSettings that holds application-wide configurations.
         *
         * This is instantiated in main.cpp
         */
        const QSettings *_settings;


        /*!
         * \brief _widgetType indicated the type of the widget.
         */
        Widget_Type _widgetType;

		inline void setWidgetType(Widget_Type wt) {_widgetType = wt;}


        /*!
         * \brief infoTextItem holds texts of widget's information.
         *
         * The text is updated by updateInfoTextItem()
         */
        SN_SimpleTextItem *infoTextItem;


        /*!
         * \brief _appInfo holds the widget's information
         */
        SN_AppInfo *_appInfo;


        /*!
         * \brief _showInfo is a bool to toggle show/hide info item
         *
         * \sa updateInfoTextItem(), drawInfo(), hideInfo()
         */
        bool _showInfo;


        /*!
         * \brief _priorityData holds the widget's priority information and functions
         */
		SN_Priority *_priorityData;


        /*!
         * \brief _perfMon holds the widget's performance information.
         *
         * Usually, streaming widgets (such as a SAGE streaming widget) will have meaningful information.
         */
        SN_PerfMonitor *_perfMon;


        /*!
         * \brief _affInfo is used to hold thread-affinity information.
         *
         * This is no longer used.
         */
        SN_AffinityInfo *_affInfo;



        /*!
         * \brief _rMonitor is a pointer to the SN_ResourceMonitor
         */
        SN_ResourceMonitor *_rMonitor;



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
         * \brief TRUE if the widget's is about to resize or rescale.
         * If a pointer's left button is pressed on one of the resize handle rectangle,
         * this value is set to 1, 2, 3, or 4 to initiate resizing or rescaling from
         * one of the corners of the widget.
         *
         * 1 north west corner
         * 2 north east corner
         * 3 south west corner
         * 4 south east corner
         */
        int _isResizing;


        /*!
         * \brief A rectangle used to resize or rescale the widget.
         *
         * If a pointer is dragging while _isResizing is set to a value greater than 0
         * then this rectangle will be shown. Upon the pointer releasing, the window
         * will be resized/rescaled based on the size of this rectangle.
         */
        QGraphicsRectItem *_resizeRectangle;
        
        /*!
          \brief A resize handle rectangle at the top left corner (NW) of the widget
          */
        QRect _resizeHandle_NW;

        /*!
         * \brief A resize handle rectangle at the top right corner (NE) of the widget
         */
        QRect _resizeHandle_NE;

        /*!
         * \brief A resize handle rectangle at the bottom left corner (SW) of the widget
         */
        QRect _resizeHandle_SW;

        /*!
         * \brief A resize handle rectangle at the bottom right corner (SE) of the widget
         */
        QRect _resizeHandle_SE;


        /*!
         * \brief The size of the effective visible area.
         *
         * This is for the visual factor of the priority (Pvisual)
         */
        quint64 _EVS;


        /*!
         * \brief effectiveVisibleRegion calculates the size of the effective visible area.
         *
         * If no widget is placed over this widget, then the _EVS is the area of this widget itself.
         *
         * \return the size of the effective visible area
         */
        QRegion effectiveVisibleRegion() const;


        /*!
          Derived class should implement paint()
          */
//	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) = 0;

        /*!
          * timeout every 1 sec to update info of appInfo, perfMon, affInfo
          */
//        void timerEvent(QTimerEvent *);

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

        /*!
         * \brief createActions initializes QActions of this widget
         */
        void createActions();


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
          * We dont' need this anymore.
          * To display infoTextItem, enable resource monitor
          */
//        int _timerID;


        /*!
         * \brief This variable is used to make a widget to declare itself as a pointer hover handling widget.
         *
         * If set to TRUE, SN_Launcher will add this widget to the scene's hover accepting apps list.
         * Whenever a user pointer's SN_PolygonArrowPointer::pointerMove() is called, the pointer search
         * for the widget that set this value to TRUE then calls handlePointerHover()
         */
		bool _registerForMouseHover;


		/*!
		  The size (in pixel) of window frame
		  */
		int _bordersize;



        /*!
         * If this is set, and the widget is launched by the SN_Launcher then
         * the launcher will add this widget to the resourceMonitor
          */
        bool _isSchedulable;


		/*!
		 * \brief initializes the widget.
		 *
		 * This is called in the constructor.
		 */
		void init();

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
