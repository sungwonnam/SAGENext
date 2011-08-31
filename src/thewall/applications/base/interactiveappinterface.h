#ifndef INTERACTIVEAPPINTERFACE_H
#define INTERACTIVEAPPINTERFACE_H

#include "imagedoublebuffer.h"

class InteractiveAppInterface
{
public:
	virtual QString name() const = 0;

	/**
	  * This just returns "this" pointer
	  * QGraphicsProxyWidget is created for this widget
	  * Note that proxy widget is slow
	  */
	virtual QObject * widgetPtr() = 0;

	/**
	  * The painter object from app widget
	  */
//	virtual void setPainter(QPainter *painter) = 0;

//	virtual void paint(QPainter *painter) = 0;


	DoubleBuffer * doubleBuffer() {return doublebuffer;}


protected:
	DoubleBuffer *doublebuffer;
//	QPainter *painter;
};

Q_DECLARE_INTERFACE( InteractiveAppInterface, "edu.uic.evl.sagenext.plugin.InteractiveApp/0.1")



#endif // INTERACTIVEAPPINTERFACE_H
