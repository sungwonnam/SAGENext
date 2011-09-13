#ifndef BASEWIDGET_H
#define BASEWIDGET_H

#include <QGraphicsWidget>

class QSettings;
class QPropertyAnimation;
class QParallelAnimationGroup;
class QGraphicsProxyWidget;

class ResourceMonitor;
class AppInfo;
class PerfMonitor;
class AffinityInfo;
class SwSimpleTextItem;

class BaseWidget : public QGraphicsWidget
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
          Required for plugin to instantiate
          Don't forget to set _globalAppId, settings, rMonitor
          */
        BaseWidget();

        BaseWidget(quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);

        virtual ~BaseWidget();

        /*!
          Only a user application will have this type number
         */
        enum { Type = QGraphicsItem::UserType + 2 };
        virtual int type() const { return Type;}

        enum Window_State { W_NORMAL, W_MINIMIZED, W_MAXIMIZED, W_HIDDEN, W_ICONIZED };

        inline Window_State windowState() const {return _windowState;}

        enum Widget_Type { Widget_RealTime, Widget_Image, Widget_GUI, Widget_Web, Widget_Misc };

        inline Widget_Type widgetType() const {return _widgetType;}

        inline void setSettings(const QSettings *s) {settings = s;}

        inline void setRMonitor(ResourceMonitor *rm) {_rMonitor = rm;}

        /*!
          AnimationWidget will create affInfo when this function is called
          */
        inline virtual void setGlobalAppId(quint64 gaid) {_globalAppId=gaid;}
        inline quint64 globalAppId() const {return _globalAppId;}

        inline AppInfo * appInfo() const {return _appInfo;}

        /*!
          ResourceMonitor needs this to update its textItem
          */
        inline PerfMonitor * perfMon() const {return _perfMon;}



        virtual void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


        /*!
          This sets the highest zValue among all app widgets on the on the scene.
          This is triggered by mousePressEvent
          */
        void setTopmost();

        /*!
          Resizing app window with this. Note that this makes pixel bigger/smaller.

          Widgets with Qt::Window as the window flag should resize by resizeHandleSceneRect().
          So, using this function in those cases is not recommended.
          */
        virtual void reScale(int tick, qreal factor);

        /*!
          The widget type Qt::Window should have resize handle for the pointer.
          So, wheel event doesn't resize the widget, and assume there's no aspect ratio requirement.
          Resizing from this doesn't make pixel bigger/smaller. Therefore, resizeEvent should resize actual Widget.

          This function does nothing but return scene rect of 100x100 on the bottom right corner of the widget
          */
        QRectF resizeHandleSceneRect();


        /*!
          If shared pointer calls mouseClick() in response to ui client's mousePress followed by mouseRelease event, then it means theat the real mouse events are not generated.

          In that case, a user can reimplement this function to define widget's behavior in response to mouse click on the widget.
          The default implementation provides real mouse press and release event.
          */
        virtual void mouseClick(const QPointF &, Qt::MouseButton);


        /*!
          Actual system mouse event can't be used when it comes to mouse dragging because if multiple users do this simultaneously, system will be confused and leads to weird behavior. So this should be implemented in child class manually.
          */
        virtual void mouseDrag(const QPointF &, Qt::MouseButton);


        /*!
          Sets the proxyWidget as a child widget.
          When a plugin isn't BaseWidget type (it didn't inherit BaseWidget) then create BaseWidget for the plugin
          */
        virtual void setProxyWidget(QGraphicsProxyWidget *proxyWidget);


        /*!
          How big is this image compared to the wall
          */
        qreal ratioToTheWall() const;

        /*!
          How much is actually visible to a user
          */
        QRegion effectiveVisibleRegion() const;

        /*!
          Parameters determining priority are visible region, and recency of interaction.
          It is important to note that priority is calculated whenever this function is called.

          @param current time in msec since Epoch
          */
        qreal priority(qint64 currTimeEpoch = 0);
        inline void setPriority(qreal p) {_priority=p;}

        inline int priorityQuantized() const {return _priorityQuantized;}
        inline void setPriorityQuantized(int p) {_priorityQuantized = p;}

        int adjustQuality(qreal adjust) {return setQuality(_quality + adjust);}

        /*!
          This is dummy function. A schedulable widget should reimplement this
          */
        virtual int setQuality(qreal newQuality) {_quality = newQuality; return 0;}

        inline qreal desiredQuality() const {return _quality;}

        /*!
          This is dummy function. A schedulable widget should reimplement this
          */
        virtual qreal observedQuality() {return _quality;}


        inline QParallelAnimationGroup *animGroup() {return _parallelAnimGroup;}


        /*!
          To manually adjust _lastTouch
          This function shouldn't be used in normal situation
          */
        void setLastTouch();
        inline qint64 lastTouch() const {return _lastTouch;}

protected:
        quint64 _globalAppId; /**< Unique identifier */

        const QSettings *settings; /**< Global configuration parameters */

        Window_State _windowState; /**< app wnidow state */

        Widget_Type _widgetType; /**< widget type */


        /*!
         * Used to display app info overlay
         */
        AppInfo *_appInfo; /**< app name, frame dimension, rect */

        bool showInfo; /**< flag to toggle show/hide info item */


        /*!
          PerfMonitor class is only meaningful with animation widget
          */
        PerfMonitor *_perfMon;


        /*!
          Instantiated when RailawareWidget
          */
        AffinityInfo *_affInfo;


        /*!
          To display information in appInfo, perfMonitor, and affInfo
          */
        SwSimpleTextItem *infoTextItem;


        /*!
          effective visible region (used to calculate priority)
          It changes whenever scale and zValue changes

          Also, it should trigger other widgets effective region be changed
          */
        QRegion _effectiveVisibleRegion;


        /*!
          is called every 1 second by timerEvent. Reimplement this to update infoTextItem to display real-time information.
          draw/hideInfo() sets/kills the timer
          */
        virtual void updateInfoTextItem();


        /*!
          When was this widget touched last time. This value is updated in mousePressEvent().
          It is to keep track user interaction recency on this widget
          */
        qint64 _lastTouch; // long long int


        ResourceMonitor *_rMonitor; /**< A pointer to the global resource monitor object */

        /*!
          0 <= quality <= 1.0
          How to calculate quality is differ by windowState
          */
        qreal _quality;


        /*!
          mouse right click on widget opens context menu provided by Qt
          This is protected because derived class could want to add its own actions
          */
        QMenu *_contextMenu;

        QAction *_showInfoAction; /**< drawInfo() */
        QAction *_hideInfoAction; /**< hideInfo() */
        QAction *_minimizeAction; /**< minimize() */
        QAction *_restoreAction; /**< restore() */
        QAction *_maximizeAction; /**< maximize() */
        QAction *_closeAction; /**< fadeOutClose() */


        inline void setWindowState(Window_State ws) {_windowState = ws;}

        void setWidgetType(Widget_Type wt);

        void init();


		/**
		  This will draw infoTextItem followed by resize rectangle
		  */
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        /*!
          Derived class should implement paint()
          */
//	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) = 0;

        /*!
          * timeout every 1 sec to update info of appInfo, perfMon, affInfo
          */
        void timerEvent(QTimerEvent *);

        /*!
          */
        virtual void resizeEvent(QGraphicsSceneResizeEvent *event);

        /*!
          * Just changes z value and keep base implementation
          */
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

//        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

        /*!
          * maximize window
          */
        virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

        /*!
          * pops up context menu this->_contextMenu
          */
        virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

        /*!
          * calls reScale()
          */
        virtual void wheelEvent(QGraphicsSceneWheelEvent *event);


        /**
          child class should provide widget specific behavior.
          If sagewidget (mplayer) then, provide video control overlay.

          BaseWidget will provide resize handle on four corners
          */
        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);

        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);


private:
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
        int timerID;


        /*!
          Always between 0 and 1. 1 means the highest priority
          */
        qreal _priority;


        /*!
          To store _priority * offset (for quantization)
          Getting this value is much faster
          */
        int _priorityQuantized;



        void createActions();

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
