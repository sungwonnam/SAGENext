#include "prioritygrid.h"
#include "common/commondefinitions.h"
#include "applications/base/basewidget.h"

PriorityGrid::PriorityGrid(const QSize &rectSize, QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , _thescene(scene)
    , _rectSize(rectSize)
    , _isEnabled(false)
{
}

int PriorityGrid::buildRectangles() {
	Q_ASSERT(_thescene);
	Q_ASSERT(!_rectSize.isEmpty());

	int wallwidth = _thescene->width();
	int wallheight = _thescene->height();

	int dimx,dimy;
	if ( wallwidth % _rectSize.width() != 0 ) {
		qWarning() << "PriorityGrid() : wall width isn't mutiple of rect width";
		deleteLater();
	}
	else if (wallheight % _rectSize.height() != 0) {
		qWarning() << "PriorityGrid() : wall height isn't mutiple of rect height";
		deleteLater();
	}
	else {
		dimx = wallwidth / _rectSize.width();
		dimy = wallheight / _rectSize.height();
	}

//	ROIRect rects[dimx*dimy];
	QRect *rects = new QRect[dimx*dimy];

	int row = 0;
	int col = 0;
	for (int i=0; i<dimx*dimy; i++) {

		int posx = col * _rectSize.width();
		int posy = row * _rectSize.height();

		_priorityVec.push_back(0.0);
		rects[i] = QRect(posx, posy, _rectSize.width(), _rectSize.height());

		++col;
		if ( col == dimx ) {
			// move to next row
			col = 0;
			++row;
		}
	}

	_theSceneRegion.setRects(rects, dimx*dimy);

	_isEnabled = true;
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
