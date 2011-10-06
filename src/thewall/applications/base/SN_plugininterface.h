#ifndef DUMMYPLUGININTERFACE_H
#define DUMMYPLUGININTERFACE_H

#include <QtCore>
class SN_BaseWidget;


class SN_PluginInterface
{
public:
	virtual ~SN_PluginInterface() {}

	/**
	  Do whatever you have to initialize here
	  */
//	virtual void postInit() = 0;

//	virtual QString name() const = 0;
//	virtual QGraphicsProxyWidget * rootWidget() = 0;

	virtual SN_BaseWidget * createInstance() = 0;
};

Q_DECLARE_INTERFACE(SN_PluginInterface, "edu.uic.evl.Sungwon.SageNext.DummyInterface/0.1")

#endif // DUMMYPLUGININTERFACE_H
