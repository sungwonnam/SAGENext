#ifndef INTERACTIVEAPPEXAMPLE_H
#define INTERACTIVEAPPEXAMPLE_H

#include "../../base/dummyplugininterface.h"
#include <QtGui>

namespace Ui {
	class InteractiveAppExample;
}

/*!
  This example shows how you can convert your regular Qt GUI application to SAGENext application.
  SAGENext use QGraphicsProxyWidget for this application to be added into SAGENext QGraphicsScene.

  Note that this class inherits QWidget not the BaseWidget.
  Also, proxywidget performs bad in QGraphics framework.
  */
class InteractiveAppExample : public QWidget, DummyPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(DummyPluginInterface)

public:
	/*!
	  Constructor sets root with itself
	  */
	InteractiveAppExample();
	~InteractiveAppExample();


	/*!
	  DummyPluginInterface has these pure virtual function
	  */
	QString name() const {return "QWidget";}


	/*!
	  You must provide pointer to your root widget. See constructor.
	  */
	QGraphicsProxyWidget * rootWidget();

protected:
//	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
	Ui::InteractiveAppExample *ui;

	/*!
	  proxy widget represent this application. Set in constructor.
	  */
	QGraphicsProxyWidget *root;


private slots:
	void setLabelText1();
	void setLabelText2();
};



#endif // INTERACTIVEAPPEXAMPLE_H
