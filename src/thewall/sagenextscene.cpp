#include "sagenextscene.h"
#include <QGraphicsItem>

#include "common/commonitem.h"
#include "applications/base/basewidget.h"

#include "uiserver/uiserver.h"

SAGENextScene::SAGENextScene(QObject *parent)
    : QGraphicsScene(parent)
    , _uiserver(0)
    , _rmonitor(0)
    , _schedcontrol(0)
{
}

void SAGENextScene::prepareClosing() {
	// close UiServer

	if (_uiserver) {
		qDebug() << "Scene is deleting UiServer";
		delete _uiserver;
	}


	if (_rmonitor) {
		qDebug() << "Scene is deleting ResourceMonitor";
		delete _rmonitor;
	}

	::sleep(1);
}

SAGENextScene::~SAGENextScene() {
	foreach (QGraphicsItem *item, items()) {
		if (!item) continue;
		if (item->type() == QGraphicsItem::UserType + 2) {
			// this is user application
			BaseWidget *bw = static_cast<BaseWidget *>(item);
//			bw->fadeOutClose();
                        bw->hide();
                        bw->close();
		}
		else {
			// this probably is common GUI
			delete item;
		}
	}


	qDebug() << "\nScene is closing all views";

	/*
	  close all views
	  */
	foreach (QGraphicsView *view, views()) {
		view->close(); // WA_DeleteOnClose is set
	}

	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}


void SAGENextScene::closeAllUserApp() {
	foreach (QGraphicsItem *item, items()) {
		if (!item ) continue;
		if (item->type() == QGraphicsItem::UserType + 2) {
			// this is user application
			BaseWidget *bw = static_cast<BaseWidget *>(item);
			Q_ASSERT(bw);
			bw->fadeOutClose();
		}
	}
}
