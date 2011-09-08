#include "sagenextscene.h"
#include <QGraphicsItem>
#include "common/commonitem.h"
#include "applications/base/basewidget.h"

SAGENextScene::SAGENextScene(QObject *parent) :
	QGraphicsScene(parent)
{


}

SAGENextScene::~SAGENextScene() {
	foreach (QGraphicsItem *item, items()) {
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
		if (item->type() == QGraphicsItem::UserType + 2) {
			// this is user application
			BaseWidget *bw = static_cast<BaseWidget *>(item);
			Q_ASSERT(bw);
			bw->fadeOutClose();
		}
	}
}
