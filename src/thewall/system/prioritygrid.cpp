#include "prioritygrid.h"
#include "common/commondefinitions.h"
#include "applications/base/basewidget.h"

PriorityGrid::PriorityGrid(const QSize &rectSize, QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , _thescene(scene)
{
	Q_ASSERT(scene);

	int wallwidth = scene->width();
	int wallheight = scene->height();

	int dimx,dimy;
	if ( wallwidth % rectSize.width() != 0 ) {
		qWarning() << "PriorityGrid() : wall width isn't mutiple of rect width";
		deleteLater();
	}
	else if (wallheight % rectSize.height() != 0) {
		qWarning() << "PriorityGrid() : wall height isn't mutiple of rect height";
		deleteLater();
	}
	else {
		dimx = wallwidth / rectSize.width();
		dimy = wallheight / rectSize.height();
	}

//	ROIRect rects[dimx*dimy];
	QRect *rects = new QRect[dimx*dimy];

	int row = 0;
	int col = 0;
	for (int i=0; i<dimx*dimy; i++) {

		int posx = col * rectSize.width();
		int posy = row * rectSize.height();

		_priorityVec.push_back(0.0);
		rects[i] = QRect(posx, posy, rectSize.width(), rectSize.height());

		++col;
		if ( col == dimx ) {
			// move to next row
			col = 0;
			++row;
		}
	}

	_theSceneRegion.setRects(rects, dimx*dimy);
//	qDebug() << "PriorityGrid() : " << _theSceneRegion.rects();
}

void PriorityGrid::updatePriorities() {
	const QVector<QRect> rects = _theSceneRegion.rects();

	for (int i=0; i<_theSceneRegion.rectCount(); i++) {

		const QRect rect = rects.at(i);
		_priorityVec[i] = 0.0; // reset priority of this rect

		//
		// For all basewidget that intersects with current rect
		//
		QList<QGraphicsItem *> items = _thescene->items(rect, Qt::IntersectsItemBoundingRect);
		foreach(QGraphicsItem *gi, items) {
			if (gi->type() < QGraphicsItem::UserType + BASEWIDGET_USER) continue;
			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(gi);

			// intersected rectangle
			QRect irect = rect & bw->mapRectToScene(bw->boundingRect()).toRect();

			qreal irectsize = irect.width() * irect.height();
			qreal rectsize = rect.width() * rect.height();

			_priorityVec[i] += (bw->priority() * (irectsize/rectsize));

//			qDebug() << "Rect" << rect.topLeft() << "intersects:" << 100.0 * irectsize/rectsize << "%.." << bw->mapRectToScene(bw->boundingRect()).toRect() << bw->scale();
		}
	}
}

//int PriorityGrid::addPriority(const QRect &windowSceneRect, qreal priorityvalue) {
//	const QVector<QRect> rects = _theSceneRegion.rects();

//	for (int i=0; i<_theSceneRegion.rectCount(); i++) {

//		const QRect rect = rects.at(i);

//		QRect intersected = rect & windowSceneRect;

//		intersectionSize = intersected.width() * intersected.height();
//		rSize = rect.width() * rect.height();

//		qreal percentage = (qreal)intersectionSize / (qreal)rSize;

//		_priorityVec[i] += (priorityvalue * percentage);

//	}
//}
