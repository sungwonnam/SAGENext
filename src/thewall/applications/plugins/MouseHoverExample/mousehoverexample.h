#ifndef MOUSEHOVEREXAMPLE_H
#define MOUSEHOVEREXAMPLE_H

#include <QtGui>

#include "applications/base/SN_plugininterface.h"
#include "applications/base/basewidget.h"



class TrackerItem : public QGraphicsItem
{
public:
	TrackerItem(qreal x, qreal y, qreal w, qreal h, const QColor &c, QGraphicsItem *parent = 0);

	QRectF boundingRect() const;

private:
	QColor _color;
	QTime _startTime;
	QSizeF _size;

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
};



class MouseHoverExample : public SN_BaseWidget, SN_PluginInterface
{
    Q_OBJECT
	Q_INTERFACES(SN_PluginInterface)

public:
    MouseHoverExample();
    ~MouseHoverExample();


	/**
	  Implementing pure virtual defined in SN_PluginInterface
	  */
	SN_BaseWidget * createInstance();


	/**
	  reimplementing BaseWidget::toggleHover(quint64 pointerid, bool isHovering)
	  This function is called by pointers in pointerMove() function

	  pointerPosOnMe is in my local coordinate
	  */
	void handlePointerHover(SN_PolygonArrowPointer *pointer, const QPointF &pointerPosOnMe, bool isHovering);

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
	/**
	  true if there is a pointer on this widget
	  */
	bool _hoverFlag;

	/**
	  pointerid(uiclientid) , hover traking item
	  */
//	QMap<quint64, TrackerItem *> _hoverTrackerItemList;
	QMap<SN_PolygonArrowPointer *, TrackerItem *> _hoverTrackerItemList;


	/**
	  contents margins to show window frame color defined in BaseWidget::paintWindowFrame()
	  */
	int _marginleft;
	int _marginright;
	int _margintop;
	int _marginbottom;
};

#endif // MOUSEHOVEREXAMPLE_H
