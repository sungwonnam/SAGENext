#include "sagenextscene.h"

#include <QtGui>

#include <unistd.h>

#include "sagenextlauncher.h"
#include "common/commonitem.h"
#include "common/sn_layoutwidget.h"
//#include "common/sn_drawingwidget.h"

#include "applications/base/basewidget.h"
#include "applications/base/appinfo.h"
#include "applications/base/sn_priority.h"

#include "system/resourcemonitor.h"
#include "uiserver/uiserver.h"

SN_TheScene::SN_TheScene(const QRectF &sceneRect, const QSettings *s, QObject *parent)
    : QGraphicsScene(sceneRect, parent)
    , _settings(s)
    , _uiserver(0)
//    , _rmonitor(0)
//    , _schedcontrol(0)
    , _rootLayoutWidget(0)
    , _closeFlag(false)
    , _closeButton(0)
    , _appRemoveButton(0)
//    , _drawingCanvas(0)
{
	//
	// This approach is ideal for dynamic scenes, where many items are added, moved or removed continuously.
	//
	setItemIndexMethod(QGraphicsScene::NoIndex);


	setBackgroundBrush(QColor(10,10,10));
	/*
	QGraphicsPixmapItem *pi = new QGraphicsPixmapItem(QPixmap(":/resources/evl-logo.png"));
	pi->setFlag(QGraphicsItem::ItemIsMovable, false);
	pi->setFlag(QGraphicsItem::ItemIsSelectable, false);
	pi->setFlag(QGraphicsItem::ItemIsFocusable, false);
	pi->setAcceptedMouseButtons(0); // Qt::NoButton
	pi->setOpacity(0.1);
	pi->setTransformOriginPoint(pi->boundingRect().width()/2, pi->boundingRect().height()/2); // origin to center
	addItem(pi);
	*/
//	pi->setPos(sceneRect.width()/2, sceneRect.height()/2);


    QPixmap evllogo(":/resources/evl-logo.png");
    QGraphicsPixmapItem *bg = new QGraphicsPixmapItem(evllogo.scaledToWidth(sceneRect.width()/2, Qt::SmoothTransformation));
    bg->setOpacity(0.2);
    bg->setFlag(QGraphicsItem::ItemIsMovable, false);
    bg->setFlag(QGraphicsItem::ItemIsSelectable, false);
    bg->setAcceptedMouseButtons(0);
    bg->setPos((sceneRect.width() - bg->boundingRect().width())/2, (sceneRect.height() - bg->boundingRect().height())/2);
    addItem(bg);


	/*
	  Attach close button on the scene. if clicked twice, scene->deleteLater will be called
      */
	QPixmap closeIcon(":/resources/x_circle_gray.png");
//	QPixmap closeIcon(":/resources/powerbutton_black_64x64.png");
//	PixmapCloseButtonOnScene *closeButton = new PixmapCloseButtonOnScene(closeIcon.scaledToWidth(sceneRect.width() * 0.02));
	_closeButton = new SN_PixmapButton(closeIcon, _settings->value("gui/iconwidth").toDouble());
	/*
	  The signature of a signal must match the signature of the receiving slot. (In fact a slot may have a shorter signature than the signal it receives because it can ignore extra arguments.)
	  */
	if ( ! QObject::connect(_closeButton, SIGNAL(clicked(int)), this, SLOT(prepareClosing())) ) {
		qDebug() << "SN_TheScene::SN_TheScene() : couldn't connect _closeButton - prepareClosing()";
	}
//	QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect;
//	opacity->setOpacity(0.2);
//	closeButton->setGraphicsEffect(opacity);
	_closeButton->setOpacity(0.2);
//	_closeButton->setPos(sceneRect.width() - _closeButton->boundingRect().width() - 5, 10); // top right corner
	_closeButton->setPos(10, 10); // top left corner
//	_closeButton->setScale(0.5);
	addItem(_closeButton);


	//
	// attach app remove button on the top
	//
	_appRemoveButton = new SN_PixmapButton(":/resources/default_button_up.png", QSize(512, 32), "Remove");
	_appRemoveButton->setFlag(QGraphicsItem::ItemIsMovable, false);
	_appRemoveButton->setFlag(QGraphicsItem::ItemIsSelectable, false);
	_appRemoveButton->setFlag(QGraphicsItem::ItemIsFocusable, false);
	_appRemoveButton->setAcceptedMouseButtons(0); // Qt::NoButton
	_appRemoveButton->setPos(sceneRect.width()/2 - _appRemoveButton->size().width()/2, 0); // position to the top center
	_appRemoveButton->setZValue(999999998); // 1 less than polygon arrow but always higher than apps
	addItem(_appRemoveButton);




	/**
	  Create base widget for wall partitioning.
	  */
	_rootLayoutWidget = new SN_LayoutWidget("ROOT", 0, this, _settings);
	_rootLayoutWidget->setRectangle(sceneRect);

	/**
	  Child widgets of _rootLayoutWidget, which are sagenext applications, will be added to the scene automatically.
      refer QGraphicsItem::setParentItem()
	  So, do NOT explicitly additem(sagenext application) !!
	  */
	addItem(_rootLayoutWidget);





	/*
	_drawingCanvas = new SN_DrawingWidget;
	_drawingCanvas->resize(sceneRect.size());
	addItem(_drawingCanvas);
	*/


}

SN_BaseWidget * SN_TheScene::getUserWidget(quint64 gaid) {
	QList<QGraphicsItem *>::const_iterator  it;
	SN_BaseWidget *bw = 0;
	for (it = items().constBegin(); it != items().constEnd(); it++) {
		QGraphicsItem *gi = (*it);
		if (!gi) continue;

		if (gi->type() < QGraphicsItem::UserType + BASEWIDGET_USER)
			continue;

		bw = static_cast<SN_BaseWidget *>(gi);
		Q_ASSERT(bw);

		if ( bw->globalAppId() == gaid) {
			return bw;
		}
	}
	return 0;
}

bool SN_TheScene::isOnAppRemoveButton(const QPointF &scenepos) {
	if (!_appRemoveButton) return false;

	return _appRemoveButton->geometry().contains(scenepos);
}

/**
  pos is in scene coordinate
  */
void SN_TheScene::addItemOnTheLayout(SN_BaseWidget *bw, const QPointF &scenepos) {
	if(_rootLayoutWidget) {
		_rootLayoutWidget->addItem(bw, _rootLayoutWidget->mapFromScene(scenepos));
//		QPointF pos = _rootLayoutWidget->mapFromScene(scenepos);
//		QMetaObject::invokeMethod(_rootLayoutWidget, "addItem", Qt::QueuedConnection, Q_ARG(void *, bw), Q_ARG(QPointF, pos));
	}
	else {
		addItem(bw);
		bw->setPos(scenepos);
	}
}

void SN_TheScene::prepareClosing() {
	if (_closeFlag) {
		// shared pointers need to be cleared before the scene closes so,
		// close UiServer so that all the shared pointers can be deleted first
		if (_uiserver) {
			qDebug() << "\n[ Scene is deleting UiServer ]\n";
			delete _uiserver;
		}

		// don't do this here
//		if (_rmonitor) {
//			qDebug() << "Scene is deleting ResourceMonitor";
//			delete _rmonitor;
//		}

		deleteLater();
	}
	else {
		// set the flag
		_closeFlag = true;
		_closeButton->setOpacity(1.0);
	}
}

SN_TheScene::~SN_TheScene() {
	if (_closeButton) {
		removeItem(_closeButton);
		delete _closeButton;
	}

	if (_appRemoveButton) {
		removeItem(_appRemoveButton);
		delete _appRemoveButton;
	}

	if (_rootLayoutWidget) {
		removeItem(_rootLayoutWidget);
		delete _rootLayoutWidget;
	}

	/*
	if (_drawingCanvas) {
		removeItem(_drawingCanvas);
		delete _drawingCanvas;
	}
	*/

	foreach (QGraphicsItem *item, items()) {
		if (!item) continue;
		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {
			// this is user application

			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			qDebug() << "Scene is deleting an application id" << bw->globalAppId();
//			bw->fadeOutClose();
			bw->hide();
			bw->close();
		}
		else {
			// this probably is common GUI
			removeItem(item);
			delete item;
		}
	}
	/*
	  close all views
	  */
	qDebug() << "\n[ Scene is closing all viewports ]\n";
	foreach (QGraphicsView *view, views()) {
		view->close(); // WA_DeleteOnClose is set
	}
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void SN_TheScene::closeAllUserApp() {
	foreach (QGraphicsItem *item, items()) {
		if (!item ) continue;
		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {
			// this is user application
			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			Q_ASSERT(bw);
//			bw->fadeOutClose();
			bw->close();
		}
	}
}



////////////////////////////////////////////////////////////////////////////


int SN_TheScene::getNumUserWidget() const {
	int count = 0;
	foreach(QGraphicsItem *item, items()) {
		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {
			++count;
		}
	}
	return count;
}

QSizeF SN_TheScene::getAvgWinSize() const {
	int count = 0;
	QSizeF size(0.0, 0.0);

	foreach(QGraphicsItem *item, items()) {
		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {

			++count;

			size += (item->scale() * item->boundingRect().size());
		}
	}

	if (!count) {
		return QSizeF();
	}

	QSizeF avgsize = size / (qreal)count;

//	foreach(QGraphicsItem *item, items()) {
//		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {

//			QSizeF size = item->scale() * item->boundingRect().size();

//			QSizeF temp = size - avgsize;
//			temp = temp * temp;
//		}
//	}

	return avgsize;
}

qreal SN_TheScene::getAvgEVRSize() const {
	qreal result = 0.0;
	int count = 0;

	foreach(QGraphicsItem *item, items()) {
		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {

			++count;

			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			Q_ASSERT(bw);
			Q_ASSERT(bw->priorityData());

			result += bw->priorityData()->evrSize();
		}
	}

	if (!count) {
		return 0;
	}
	else {
		return result / count;
	}
}


qreal SN_TheScene::getRatioEmptySpace() const {
	QRegion wallregion(0, 0, width(), height());

	qreal wallsize = width() * height();

	foreach(QGraphicsItem *item, items()) {
		if (item->type() >= QGraphicsItem::UserType + BASEWIDGET_USER) {

			// subtract application's window rectangle from the wall rectangle
			wallregion -= QRegion(item->sceneBoundingRect().toRect());
		}
	}

	// now wall region only contains empty rectangles

	qreal emptysize = 0;
	foreach(QRect rect, wallregion.rects()) {
		emptysize += (rect.width() * rect.height());
	}

	return emptysize / wallsize;
}

qreal SN_TheScene::getRatioOverlapped() const {
	qreal overlappedsize = 0.0;

	foreach(QGraphicsItem *item, items()) {
		if (item->type() < QGraphicsItem::UserType + BASEWIDGET_USER)
			continue;

		foreach(QGraphicsItem *citem, item->collidingItems()) {

			if (citem->type() < QGraphicsItem::UserType + BASEWIDGET_USER)
				continue;

			QRectF irectf = citem->sceneBoundingRect() & item->sceneBoundingRect();
			overlappedsize += (irectf.width() * irectf.height());
		}
	}
	return (overlappedsize / (width() * height()));
}





////////////////////////////////////////////////////////////////////////////


void SN_TheScene::saveSession() {
	QString sessionFilename = QDir::homePath() + "/.sagenext/sessions/";

	sessionFilename.append(QDateTime::currentDateTime().toString("hh.mm.ss_MM.dd.yyyy_"));

	QString screenshotFilename = sessionFilename;
	screenshotFilename.append(".jpg");

	sessionFilename.append(".session");

	QFile sessionfile(sessionFilename);
	if(!sessionfile.open(QIODevice::ReadWrite)) {
		qCritical() << "SN_TheScene::saveSession() : couldn't open the file" << sessionFilename;
		return;
	}
	qWarning() << "\nSN_TheScene::saveSession() : save current layout to" << sessionFilename;


	////
	// create snapshot
	////
	QPixmap screenshot(sceneRect().size().toSize());
	QPainter painter(&screenshot); // QPixmap is the paintDevice for this painter
	render(&painter);
	screenshot.save(screenshotFilename, "jpg", 100);


	QDataStream out(&sessionfile);

	if (_rootLayoutWidget) {
		_rootLayoutWidget->saveSession(out);
	}
	else {
		//
		// just save widget states
		//
		foreach(QGraphicsItem *item, items()) {

			// only consider user application
			if (!item || item->type() < QGraphicsItem::UserType + BASEWIDGET_USER) continue;

			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(item);
			if (!bw) continue;

            out << QString("ITEM");

            out << bw->globalAppId();

            AppInfo *ai = bw->appInfo();
//			bw->appInfo()->mediaType()
            out << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale();

            switch (ai->mediaType()) {
			case SAGENext::MEDIA_TYPE_IMAGE :
			case SAGENext::MEDIA_TYPE_LOCAL_VIDEO :
			case SAGENext::MEDIA_TYPE_PDF :
			case SAGENext::MEDIA_TYPE_PLUGIN : {

				Q_ASSERT(ai->fileInfo().exists());
				out << ai->mediaFilename();
				qDebug() << "SN_TheScene::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->mediaFilename();

				break;
			}
			case SAGENext::MEDIA_TYPE_WEBURL : {

				Q_ASSERT(!ai->webUrl().isEmpty());
				out << ai->webUrl().toString();
				qDebug() << "SN_TheScene::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->webUrl().toString();

				break;
			}
			case SAGENext::MEDIA_TYPE_VNC : {

				out << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();
				qDebug() << "SN_TheScene::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->srcAddr() << ai->vncUsername() << ai->vncPassword();

				break;
			}
			case SAGENext::MEDIA_TYPE_SAGE_STREAM : {

				out << ai->srcAddr() << ai->executableName() << ai->cmdArgsString();
				qDebug() << "SN_TheScene::saveSession() : " << (int)ai->mediaType() << bw->scenePos() << bw->size() << bw->scale() << ai->srcAddr() << ai->executableName() << ai->cmdArgsString();

				break;
			}
			}

			///
			/// and application specific state can be followed
			///
		}
	}
	sessionfile.flush();
	sessionfile.close();
}

void SN_TheScene::loadSession(QDataStream &in, SN_Launcher *launcher) {
	if (_rootLayoutWidget) {
		_rootLayoutWidget->loadSession(in, launcher);
	}
	else {
		QString header; /// ITEM
		int mtype;
		QPointF scenepos;
		QSizeF size;
		qreal scale;
        quint64 gaid;

		while (!in.atEnd()) {
			in >> header;
			if (header != "ITEM") continue;

            in >> gaid;

			in >> mtype >> scenepos >> size >> scale;
	//		qDebug() << "\tentry : " << mtype << scenepos << size << scale;

			QString file;
			QString user;
			QString pass;
			QString srcaddr;
            QString sageappname;
            QString cmdargs;

			SN_BaseWidget *bw = getUserWidget(gaid);

            if (bw) {
                qDebug() << "SN_TheScene::loadSession() : There exist a widget with gid" << gaid;
            }

            switch ( (SAGENext::MEDIA_TYPE)mtype ) {
            case SAGENext::MEDIA_TYPE_IMAGE :
            case SAGENext::MEDIA_TYPE_PDF :
            case SAGENext::MEDIA_TYPE_PLUGIN : {
                in >> file;
                if (!bw)
                    bw = launcher->launch(mtype, file, scenepos, gaid);
                break;
            }
            case SAGENext::MEDIA_TYPE_WEBURL : {
                break;
            }
            case SAGENext::MEDIA_TYPE_VNC : {
                in >> srcaddr >> user >> pass;
                if (!bw)
                    bw = launcher->launch(user, pass, 0, srcaddr, 10, scenepos, gaid);
                break;
            }
            case SAGENext::MEDIA_TYPE_LOCAL_VIDEO : {
                in >> file;
                // command and its arguments will be overriden
                if (!bw) {
                    bw = launcher->launchSageApp(SAGENext::MEDIA_TYPE_LOCAL_VIDEO, file, scenepos, "127.0.0.1", "", "mplayer", gaid);
                    ::usleep(100 * 1e+3); // 100 msec
                }
                break;
            }
            case SAGENext::MEDIA_TYPE_SAGE_STREAM : {
                in >> srcaddr >> sageappname >> cmdargs;
                if (!bw) {
                    bw = launcher->launchSageApp(SAGENext::MEDIA_TYPE_SAGE_STREAM, file, scenepos, srcaddr, cmdargs, sageappname, gaid);
                    ::usleep(100 * 1e+3); // 100 msec
                }
                break;
            }
            }


			if (!bw) {
				qDebug() << "SN_TheScene::loadSession() : Error : can't launch this entry from the session file" << mtype << file << srcaddr << user << pass << scenepos << size << scale;
				continue;
			}
            else {
                QApplication::sendPostedEvents();
                QCoreApplication::processEvents();

                //
                // at this point, the bw has added to the _rootLayoutWidget already
                //

                /******

                  in SN_BaseWidget::resizeEvent(), item's transformation origin is set to its center.
                  Because of this, if an image, whose pos is an edge of the scene, is scaled down (from its CENTER), the image can end up positioning out of scene's visible area.
                  So, I need to apply scale not from its center but from its topleft.
                  //bw->setScale(scale);
                  *****/
                //QTransform scaleTrans(scale, 0, 0, scale, 0, 0); // m31(dx) and m32(dy) which specify horizontal and vertical translation are set to 0
                //bw->setTransform(scaleTrans, true); // false -> this transformation matrix won't be combined with the current matrix. It will replace the current matrix.

                /**
                  below will work ok if item's transformOrigin is its top left
                  **/
                bw->setScale(scale);

                bw->resize(size);

                bw->setPos(scenepos);
            }
		}
	}
}

/*
QDataStream out(&sFile);

foreach (QGraphicsItem *gi, scene()->items()) {
	if (!gi) continue;

	// only consider user application
	if (gi->type() < QGraphicsItem::UserType + 12) continue;

	BaseWidget *bw = static_cast<BaseWidget *>(gi);
	out << bw->pos() << bw->scale() << bw->zValue();
}
*/


























//PartitionTreeNode::PartitionTreeNode(QGraphicsScene *s, PartitionTreeNode *p, const QRectF &r, QObject *parent)
//    : QObject(parent)
//    , _scene(s)
//    , _parentNode(p)

//    , _leftOrTop(0)
//    , _rightOrBottom(0)
//    , _orientation(0)
//    , _lineItem(0)

//    , _tileButton(0)
//    , _hButton(0)
//    , _vButton(0)
//    , _closeButton(0)
//{
//	_rectf = r;

//	qDebug() << "PartitionTreeNode() " << _rectf;

//	_hButton = new PixmapButton(":/resources/minimize_shape.gif");
//	connect(_hButton, SIGNAL(clicked()), this, SLOT(createNewHPartition()));
//	// add this button to the scene based on _rectf
//	_scene->addItem(_hButton);
//	_hButton->setPos(_rectf.width() - _hButton->rect().width(), _rectf.y() +  _rectf.height() / 2);


//	if ( _parentNode ) {
//		_closeButton = new PixmapButton(":/resources/close_over.png");
//		connect(_closeButton, SIGNAL(clicked()), _parentNode, SLOT(deleteChildPartitions()));
//		_scene->addItem(_closeButton);
//		_closeButton->setPos(_rectf.width() - _closeButton->rect().width(),  _rectf.y() + _rectf.height()/2);
//	}
//}
//PartitionTreeNode::~PartitionTreeNode() {
//	if (_lineItem) delete _lineItem;

//	if (_hButton) delete _hButton;
//	if (_vButton) delete _vButton;
//	if (_closeButton) delete _closeButton;
//	if (_tileButton) delete _tileButton;

//	if (_leftOrTop) delete _leftOrTop;
//	if (_rightOrBottom) delete _rightOrBottom;
//}

//void PartitionTreeNode::lineHasMoved(const QPointF &newpos) {
//	Q_ASSERT(_leftOrTop);
//	Q_ASSERT(_rightOrBottom);

//	QRectF leftrect;
//	QRectF rightrect;

//	if (_orientation == 1) {
//		// horizontal
//		// line only moves up and down

//		// top
//		leftrect = QRectF( _rectf.x(), _rectf.y(), _rectf.width(), newpos.y() - _rectf.y() );

//		// bottom
//		rightrect = QRectF(_rectf.x(), newpos.y(), _rectf.width(), _rectf.bottomRight().y() - newpos.y());
//	}
//	else if (_orientation ==2) {

//	}

//	_leftOrTop->setNewRectf(leftrect);
//	_rightOrBottom->setNewRectf(rightrect);
//}

//void PartitionTreeNode::createNewPartition(int o) {
//	_orientation = o;

//	QRectF leftrect;
//	QRectF rightrect;
//	Qt::Orientation ori;

//	if (o == 1) {
//		// horizontal
//		ori = Qt::Horizontal;
//		qreal newHeight = _rectf.height() / 2.0;
//		leftrect = QRectF(_rectf.x(), _rectf.y(), _rectf.width(), newHeight);
//		rightrect = QRectF(_rectf.x(), _rectf.y() + newHeight, _rectf.width(), newHeight);
//	}
//	else if ( o == 2) {
//		ori = Qt::Vertical;
//	}
//	_leftOrTop = new PartitionTreeNode(_scene, this, leftrect);
//	_rightOrBottom = new PartitionTreeNode(_scene, this, rightrect);

//	// hide my buttons
//	if (_hButton) _hButton->hide();
//	if (_closeButton) _closeButton->hide();


//	// create line item and I'm the owner
////	_lineItem = new PartitionBar(ori, this);
//	_lineItem->setPos(rightrect.topLeft());
//	_scene->addItem(_lineItem);

//}

//void PartitionTreeNode::deleteChildPartitions() {
//	Q_ASSERT(_lineItem);
//	Q_ASSERT(_leftOrTop);
//	Q_ASSERT(_rightOrBottom);

//	delete _lineItem;
//	delete _leftOrTop;
//	delete _rightOrBottom;

//	// show my buttons
//	_hButton->show();
//	if (_parentNode && _closeButton) _closeButton->show();
//}

//void PartitionTreeNode::setNewRectf(const QRectF &r) {
//	qDebug() << "PartitionTreeNode::setNewRectf()" << r;

//	// because I'm an actual partition
//	Q_ASSERT( !_lineItem );
//	Q_ASSERT( !_leftOrTop );
//	Q_ASSERT( !_rightOrBottom );

//	_rectf = r;

//	// adjust belonging widget's pos
//}
