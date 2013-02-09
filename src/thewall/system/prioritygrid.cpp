#include "prioritygrid.h"
#include "common/commondefinitions.h"
#include "applications/base/basewidget.h"

#include <QGraphicsScene>

SN_PriorityGrid::SN_PriorityGrid(const QSize &rectSize, QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , _dimx(0)
    , _dimy(0)
    , _thescene(scene)
    , _rectSize(rectSize)
    , _isEnabled(false)
    , _theTotalPriority(0)
{
	buildRectangles();
}

SN_PriorityGrid::SN_PriorityGrid(int numrow, int numcol, QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , _dimx(numcol)
    , _dimy(numrow)
    , _thescene(scene)
    , _isEnabled(false)
    , _theTotalPriority(0)
{
    buildRectangles();
}

SN_PriorityGrid::~SN_PriorityGrid() {
    /*
    qint64 gridDuration = QDateTime::currentMSecsSinceEpoch() - _gridStartTimeSinceEpoch;
    QFile f(QDir::homePath() + "/.sagenext/pGrid_" + QString::number(gridDuration / (1000*60)) +"min_" + QDateTime::currentDateTime().toString(Qt::ISODate));
    if ( !f.open(QIODevice::WriteOnly)) {
        qDebug() << "~SN_PriorityGrid() : failed to open a file" ;
        return;
    }


    QTextStream ts(&f);

    ts << _dimx << "," << _dimy << "," << _theTotalPriority << "\n";

    const QVector<QRect> cells = _theSceneRegion.rects();

    //
    // for each cell in the grid region
    //
    for (int i=0; i<_theSceneRegion.rectCount(); i++) {
        const QRect cell = cells.at(i);

        ts << cell.topLeft().x()
           << "," << cell.topLeft().y()
           << "," << cell.width()
           << "," << cell.height()
           << "," << _priorityRawVec.at(i)
           << "\n";

        ts.flush();
    }

    f.flush();
    f.close();
    */
}

void SN_PriorityGrid::buildRectangles() {
	Q_ASSERT(_thescene);

    if (_dimx == 0 || _dimy == 0) {
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
    }
    else {
        _rectSize.rwidth() = _thescene->width() / _dimx;
        _rectSize.rheight() = _thescene->height() / _dimy;
    }



    qDebug() << "Building the priority grid" << _dimx << "by" << _dimy;

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

        qDebug() << "\tCell" << col << "," << row << ":" << rects[i];

		++col;
		if ( col == _dimx ) {
			// move to next row
			col = 0;
			++row;
		}
	}

	_theSceneRegion.setRects(rects, _dimx * _dimy);

	_isEnabled = true;

    _gridStartTimeSinceEpoch = QDateTime::currentMSecsSinceEpoch();
}


void SN_PriorityGrid::addPriorityOfApp(const QRect &windowSceneRect, qreal priorityvalue) {
    const QVector<QRect> cells = _theSceneRegion.rects();

    //
    // for each cell in the grid region
    //
    for (int i=0; i<_theSceneRegion.rectCount(); i++) {

        const QRect cell = cells.at(i);

        QRect irect = cell & windowSceneRect;

        if (irect.isEmpty()) continue;

        qreal intersectRatio = (qreal)(irect.width() * irect.height()) / (qreal)(windowSceneRect.width() * windowSceneRect.height());

        _priorityRawVec[i] += (priorityvalue * intersectRatio);

    }
}


void SN_PriorityGrid::updatePriorities() {
	const QVector<QRect> cells = _theSceneRegion.rects();

    //
    // sum of priority values of all cells
    //
    qreal totalPriority = 0.0;

    //
    // for each cell in the grid
    //
	for (int i=0; i<_theSceneRegion.rectCount(); i++) {

		const QRect cell = cells.at(i);

//        qDebug() << "The Cell" << cell << "intersects with ...";

		//
		// For all the applications that intersects with current cell
		//
		QList<QGraphicsItem *> items = _thescene->items(cell, Qt::IntersectsItemBoundingRect);
		foreach(QGraphicsItem *gi, items) {
			if (gi->type() < QGraphicsItem::UserType + BASEWIDGET_USER) continue;
			SN_BaseWidget *bw = static_cast<SN_BaseWidget *>(gi);

            QRect windowSceneRect = bw->sceneBoundingRect().toRect();

            //
			// get intersected rectangle.
            // Note that sceneBoundingRect() reflects widget's scale
            // This means the rect returned by sceneBoundingRect() represents presentation rectangle..
            //
			QRect irect = cell & windowSceneRect;

            if (irect.isEmpty()) continue;

            //
            // How much of widget's window is overlapping with the cell ?
            //
            qreal intersectRatio = (qreal)(irect.width() * irect.height()) / (qreal)(windowSceneRect.width() * windowSceneRect.height());

			/*
			  The priority of the application whose window intersect with the current cell
			  multiplied by the ratio of intersected rectangle to the cell rectangle.

              Equation (3) in the paper.
			  */
			_priorityRawVec[i] += (bw->priority() * intersectRatio);
//            qDebug() << "\twidget" << bw->globalAppId() << bw->sceneBoundingRect() << "iRect" << irect << "Overlap" << 100 * intersectRatio <<"%";
		}

        totalPriority += _priorityRawVec.at(i);
	}

    // Update the sum of the all the values of the cells
    _theTotalPriority = totalPriority;

    // Update the value of each cell
	for (int i=0; i<_priorityVec.size(); i++) {
		_priorityVec[i] = _priorityRawVec.at(i) / totalPriority; // always less than 1.0
	}
}

qreal SN_PriorityGrid::getPriorityOffset(const SN_BaseWidget *widget) {
    return getPriorityOffset(widget->sceneBoundingRect().toRect());
}

qreal SN_PriorityGrid::getPriorityOffset(const QRect &widgetscenerect) {
	const QVector<QRect> cells = _theSceneRegion.rects();

	qreal result = 0.0;

	for (int i=0; i<_theSceneRegion.rectCount(); i++) {
		const QRect cell = cells.at(i);

		const QRect irect = cell.intersected(widgetscenerect);

		if (!irect.isNull() && !irect.isEmpty()) {
            // How much of the intersected rectangle overlaps with the cell
			qreal intersectRatio = (qreal)(irect.width() * irect.height()) / (qreal)(cell.width() * cell.height());
			result += (intersectRatio * _priorityRawVec.at(i)); // always less than 1.0
		}
	}

    m_getTotalPriority();

	return (result / _theTotalPriority);
}

qreal SN_PriorityGrid::getPriorityOffset(int row, int col) {
	int arrayindex = col + (_dimx * row);
	if (arrayindex < 0 || arrayindex >= _priorityVec.size()) {
		qDebug() << "PriorityGrid::getPriorityOffset() : arrayindex out of bound" << arrayindex;
		return 0;
	}

    m_getTotalPriority();

    return ( _priorityRawVec.at(arrayindex) / _theTotalPriority );
}


void SN_PriorityGrid::printTheGrid() {
    // row
    for (int i=0; i<_dimy; i++) {

        // column
        for (int j=0; j<_dimx; j++) {
            qDebug() << i << "," << j << ":" << getPriorityOffset(i, j);
        }
    }
}

qreal SN_PriorityGrid::m_getTotalPriority() {
    qreal total = 0.0;
    // row
    for (int i=0; i<_priorityRawVec.size(); i++) {
        total += _priorityRawVec.at(i);
    }
    _theTotalPriority = total;
    return _theTotalPriority;
}
