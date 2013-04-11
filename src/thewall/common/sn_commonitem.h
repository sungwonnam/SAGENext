#ifndef COMMONITEM_H
#define COMMONITEM_H

#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

/*!
  \brief A QGraphicsWidget type widget that shows pixmap and function as a button (emits signal when clicked).
  This item reacts to the system mouse event (press/release)
  So, this item can be interacted with SN_PolygonArrowPointer because it generates system mouse event(press/release) in pointerClick()
  */
class SN_PixmapButton : public QGraphicsWidget
{
	Q_OBJECT
	Q_PROPERTY(int priorityOverride READ priorityOverride WRITE setPriorityOverride)
public:
	/**
	  creates button with resource
	  */
	explicit SN_PixmapButton(const QString &res_normal, qreal desiredWidth = 0.0, const QString &label = QString(), QGraphicsItem *parent = 0);

	/**
	  creates button with pixmap
	  */
	explicit SN_PixmapButton(const QPixmap &pixmap_normal, qreal desiredWidth = 0.0, const QString &label = QString(), QGraphicsItem *parent=0);

	/**
	  creates button with custom size
	  */
	explicit SN_PixmapButton(const QString &res_normal, const QSize &size = QSize(), const QString &label = QString(), QGraphicsItem *parent=0);

    explicit SN_PixmapButton(QGraphicsItem *parent=0) : QGraphicsWidget(parent, Qt::Widget) {}

    void setPrimaryPixmap(const QString &resource, const QSize &size);
    void setPrimaryPixmap(const QPixmap &pixmap, const QSize &size);
    void setPrimaryPixmap(const QString &resource, int width);
    void setPrimaryPixmap(const QPixmap &pixmap, int width=0);
    
    void setSecondaryPixmap(const QString &resource);
    void setSecondaryPixmap(const QPixmap &pixmap);

	virtual ~SN_PixmapButton();

    void setLabel(const QString &label);

	inline void setPriorityOverride(int v) {_priorityOverride = v;}
	inline int priorityOverride() const {return _priorityOverride;}

    /*!
      toggles pixmap for the button
      */
    void togglePixmap();

    inline virtual void click() {emit clicked();}

private:
    QGraphicsPixmapItem *_primary;

    QGraphicsPixmapItem *_secondary;
    
    void _init();

	void _attachLabel(const QString &labeltext, QGraphicsItem *parent);

	/*!
	  set to true if mousePressEvent on this widget
	  */
	bool _mousePressFlag;

	/*!
	  1 : indicate clicking this button should raise the priority of the application owns this button
	  0 : default
	  -1 : indicate clicking this button should lower the priority of the application owns this button
	  */
	int _priorityOverride;

    QPixmap _selectiveRescale(const QPixmap &pixmap, const QSize &size);

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
    void clicked();

};














/*!
  Base class of all GUI components
  */
class SN_ProxyGUIBase : public QGraphicsProxyWidget {
    Q_OBJECT
public:
    SN_ProxyGUIBase(QGraphicsItem* parent=0, Qt::WindowFlags wf=0) : QGraphicsProxyWidget(parent,wf) {}
    virtual ~SN_ProxyGUIBase() {}

    inline virtual void click(const QPoint & = QPoint()) {emit clicked();}

signals:
    void clicked();
};

class SN_ProxyScrollBar : public SN_ProxyGUIBase {
    Q_OBJECT
public:
    SN_ProxyScrollBar(Qt::Orientation, QGraphicsItem *parent=0);

    inline void setOrientation(Qt::Orientation o) {_scrollbar->setOrientation(o);}
    inline void setRange(int min, int max) {_scrollbar->setRange(min,max);}

    inline void click(const QPoint &clickedpos = QPoint()) {drag(clickedpos);}
    void drag(const QPointF &pos);

protected:
    QScrollBar* _scrollbar;

signals:
    void valueChanged(int value);
};



class SN_ProxyPushButton : public SN_ProxyGUIBase {
    Q_OBJECT
public:
    SN_ProxyPushButton(QGraphicsItem* parent=0);
    SN_ProxyPushButton(const QString& text, QGraphicsItem* parent=0);

    inline void setDown(bool b) {_button->setDown(b);}

protected:
    QPushButton* _button;
};

class SN_ProxyRadioButton : public SN_ProxyGUIBase {
    Q_OBJECT
public:
    SN_ProxyRadioButton(QGraphicsItem* parent=0);
    SN_ProxyRadioButton(const QString& text, QGraphicsItem* parent=0);

protected:
    QRadioButton* _radiobtn;
};



/*!
  This is used to allow users to feed string data to a LineEdit item
  */
class SN_ProxyLineEdit : public SN_ProxyGUIBase
{
	Q_OBJECT
public:
    SN_ProxyLineEdit(QGraphicsItem *parent=0, const QString &placeholdertext =QString());
    ~SN_ProxyLineEdit();

    /*!
     * \brief setText
     * \param text
     */
    void setText(const QString &text, bool emitSignal = true);

    inline void click(const QPoint &) {_lineedit->selectAll();}

protected:
    QLineEdit* _lineedit;

signals:
	void pressed();
	void textChanged(QString);

public slots:
    inline void selectAll() {_lineedit->selectAll();}
};










/**
  This is used to display text on background rectangle
  */
class SN_SimpleTextItem : public QGraphicsSimpleTextItem
{
public:
	SN_SimpleTextItem(int pointSize=0, const QColor &fontcolor = QColor(Qt::black), const QColor &bgcolor = QColor(Qt::gray), QGraphicsItem *parent=0);
	~SN_SimpleTextItem() {}

	void setFontPointSize(int ps);

private:
	QColor _fontcolor;
	QColor _bgcolor;

protected:
	void wheelEvent(QGraphicsSceneWheelEvent * event);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
};




/**
  This is used to display text on background rectangle
  And supports signal/slot 
  */
class SN_SimpleTextWidget : public QGraphicsWidget
{
	Q_OBJECT
public:
	SN_SimpleTextWidget(int pointSize=0, const QColor &fontcolor = QColor(Qt::black), const QColor &bgcolor = QColor(Qt::gray), QGraphicsItem *parent=0);

	void setText(const QString &text);

private:
	SN_SimpleTextItem *_textItem;
};



#endif // COMMONITEM_H
