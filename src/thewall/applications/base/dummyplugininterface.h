#ifndef DUMMYPLUGININTERFACE_H
#define DUMMYPLUGININTERFACE_H

#include <QtCore>
class QGraphicsProxyWidget;

class DummyPluginInterface
{
public:
	virtual ~DummyPluginInterface() {}

	/**
	  Do whatever you have to initialize here
	  */
//	virtual void postInit() = 0;

	virtual QString name() const = 0;
	virtual QGraphicsProxyWidget * rootWidget() = 0;
};

Q_DECLARE_INTERFACE(DummyPluginInterface, "edu.uic.evl.Sungwon.SageNext.DummyInterface/0.1")

#endif // DUMMYPLUGININTERFACE_H
