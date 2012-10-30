#ifndef COMMONITEM_H
#define COMMONITEM_H

#include <QtGui>

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

	~SN_PixmapButton();

	inline void setPriorityOverride(int v) {_priorityOverride = v;}
	inline int priorityOverride() const {return _priorityOverride;}

    /*!
      toggles pixmap for the button
      */
    void togglePixmap();

private:
    QGraphicsPixmapItem *_primary;

    QGraphicsPixmapItem *_secondary;

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

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
	void clicked(int);
    void clicked();

public slots:
    inline virtual void handlePointerClick() {emit clicked(0);}
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







class SN_PolygonArrowPointer;

/*!
  This is used to allow users to feed string data to a LineEdit item
  */
class SN_LineEdit : public QGraphicsWidget
{
	Q_OBJECT
public:
	SN_LineEdit(QGraphicsItem *parent=0);
	SN_LineEdit(const SN_LineEdit &);
	~SN_LineEdit() {}

	QLineEdit *_lineedit;
	QGraphicsProxyWidget *_proxywidget;

	SN_PolygonArrowPointer *_pointer;

	void setText(const QString &text);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);


signals:
	void pressed();

	void textChanged(QString);

public slots:
	void setThePointer(SN_PolygonArrowPointer *p);
};

//Q_DECLARE_METATYPE(SN_LineEdit)


#endif // COMMONITEM_H
