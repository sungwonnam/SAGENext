#ifndef MOUSEHOVEREXAMPLE_H
#define MOUSEHOVEREXAMPLE_H


#include "../../base/dummyplugininterface.h"
#include "../../base/basewidget.h"

#include <QtGui>

class MouseHoverExample : public BaseWidget, DummyPluginInterface
{
    Q_OBJECT
	Q_INTERFACES(DummyPluginInterface)

public:
    MouseHoverExample();
    ~MouseHoverExample();

	/*!
      One must implement the interface (DummyPluginInterface)
	  Return "BaseWidget" for the plugin that inherits BaseWidget
      */
    QString name() const;

	/*!
	  QGraphicsProxyWidget *root is going to be null
	  for the plugin that inherits BaseWdiget
	  */
    QGraphicsProxyWidget * rootWidget();


	/**
	  reimplementing BaseWidget::toggleHover(quint64 pointerid, bool isHovering)
	  This function is called by pointers in pointerMove() function
	  */
	void toggleHover(SAGENextPolygonArrow *pointer, const QPointF &pointerPosOnMe, bool isHovering);

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	/**
	  to move _textItem properly
	  */
	void resizeEvent(QGraphicsSceneResizeEvent *event);

private:
	QGraphicsSimpleTextItem *_textItem;

	bool _hoverFlag;
};

#endif // MOUSEHOVEREXAMPLE_H
