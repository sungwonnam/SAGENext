#ifndef COMMONITEM_H
#define COMMONITEM_H

#include <QtGui>

/**
  * The general button pixmap that emits signal when clicked.
  This item reacts to the system mouse event (press/release)
  So, this item can be interacted with SN_PolygonArrowPointer because the Pointer generates system mouse event(press/release) upon receiving mouseClick event from uiclient
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

	~SN_PixmapButton();

	inline void setPriorityOverride(int v) {_priorityOverride = v;}
	inline int priorityOverride() const {return _priorityOverride;}

private:
	QPixmap _normalPixmap;
	QPixmap _hoveredPixmap;

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
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
	void clicked(int);
};



/**
  This is used to display text on background rectangle
  */
class SN_SimpleTextItem : public QGraphicsSimpleTextItem
{
public:
	SN_SimpleTextItem(int pointSize=0, const QColor &fontcolor = QColor(Qt::black), const QColor &bgcolor = QColor(Qt::gray), QGraphicsItem *parent=0);
	~SN_SimpleTextItem();

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
