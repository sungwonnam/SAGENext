#include "prioritygrid.h"
#include "common/commondefinitions.h"
#include "applications/base/basewidget.h"

SN_PriorityGrid::SN_PriorityGrid(const QSize &rectSize, QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , _dimx(0)
    , _dimy(0)
    , _thescene(scene)
    , _rectSize(rectSize)
    , _isEnabled(false)
{
	buildRectangles();
}

void SN_PriorityGrid::buildRectangles() {
	Q_ASSERT(_thescene);
	Q_ASSERT(!_rectSize.isEmpty());

	int wallwidth = _thescene->width();
	int wallheight = _thescene->height();

	if ( wallwidth % _rectSize.width() != 0 ) {
		qWarning() << "PriorityGrid() : wall width isn't mutiple of rect width";
		deleteLater();
	}
	else if (wallheight % _rectSize.height() != 0) {
		qWarning() << "PriorityGrid() : wall height isn't mutiple of rect height";
		deleteLater();
	}
	else {
		_dimx = wallwidth / _rectSize.width();
		_dimy = wallheight / _rectSize.height();
	}

//	ROIRect rects[dimx*dimy];
	QRect *rects = new QRect[_dimx * _dimy];

	int row = 0;
	int col = 0;
	for (int i=0; i<_dimx*_dimy; i++) {

		int posx = col * _rectSize.width();
		int posy = row * _rectSize.height();

		_priorityRawVec.push_back(0.0);
		_priorityVec.push_back(0.0);
		rects[i] = QRect(posx, posy, _rectSize.width(), _rectSize.height());

		++col;
		if ( col == _dimx ) {
			// move to next row
			col = 0;
			++row;
		}
	}

	_theSceneRegion.setRects(rects, _dimx * _dimy);

	_isEnabled = true;
}

void SN_PriorityGrid::updatePriorities() {
	const QVector<QRect> rects = _theSceneRegion.rects();

	// this is to normalize aggregate value
	qreal currentMax = 0.0;

	for (int i=0; i<_theSceneRegion.rectCount(); i++) {

		const QRect rect = rects.at(i);

		//
		// For all basewidget that intersects with current rect
		//
		QList<QGraphicsItem *> items = _thescene->items(rect, Qt::IntersectsItemBoundingRect);
		foreach(QGraphicsItem *gi, items) {
			if (gi->type() < QGraphicsItem::UserType + BASEWIDGET_USER) continue;
			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(gi);

			// intersected rectangle
//			QRect irect = rect & bw->mapRectToScene(bw->boundingRect()).toRect();
			QRect irect = rect & bw->sceneBoundingRect().toRect();

			qreal irectsize = irect.width() * irect.height();
			qreal rectsize = rect.width() * rect.height();

//			qDebug() << "grid rect" << rect << " irect" << irect;

			/*
			  priority of the application whose window intersect with the current grid
			  multiplied by
			  the ratio of intersected rectangle to the grid rectangle
			  */
			qreal priorityPortion = bw->priority() * (irectsize/rectsize);

			// since the priority value kept added
			// the value could be very high !
			// this value will later be normalized
			// so absolute value is meaningless anyway
			priorityPortion *= 0.001;

			_priorityRawVec[i] += priorityPortion;

			currentMax = qMax(currentMax, _priorityRawVec.at(i));

//			qDebug() << "Rect" << rect.topLeft() << "intersects:" << 100.0 * irectsize/rectsize << "%.." << bw->mapRectToScene(bw->boundingRect()).toRect() << bw->scale();
		}
	}

	for (int i=0; i<_priorityVec.size(); i++) {
		_priorityVec[i] = _priorityRawVec.at(i) / currentMax;
	}
}

qreal SN_PriorityGrid::getPriorityOffset(const QRect &scenerect) {
	const QVector<QRect> rects = _theSceneRegion.rects();

	qreal result = 0.0;

	for (int i=0; i<_theSceneRegion.rectCount(); i++) {
		const QRect gridrect = rects.at(i);

		const QRect irect = gridrect.intersected(scenerect);

		if (!irect.isNull() && !irect.isEmpty()) {
			qreal ratio = qreal(irect.width() * irect.height()) / qreal(scenerect.width() * scenerect.height());
			result += (ratio * _priorityVec.at(i));
		}
	}
	return result;
}

qreal SN_PriorityGrid::getPriorityOffset(int row, int col) {
	int arrayindex = col + (_dimx * row);
	if (arrayindex < 0 || arrayindex >= _priorityVec.size()) {
		qDebug() << "PriorityGrid::getPriorityOffset() : arrayindex out of bound" << arrayindex;
		return 0;
	}
	return _priorityVec.at(arrayindex);
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
