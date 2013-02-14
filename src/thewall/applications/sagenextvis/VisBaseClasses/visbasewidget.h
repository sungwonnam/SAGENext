#ifndef VISBASEWIDGET_H
#define VISBASEWIDGET_H

#include <QObject>
#include <QString>
#include <QGraphicsWidget>
#include <QGraphicsItem>
#include <QDebug>
#include <QRectF>
#include <QPainterPath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QFile>
#include <QGraphicsScene>
#include <QCursor>
#include <QPointer>
#include <QList>

#include "sagenextvisbasewidget.h"

//Forward declarations to avoid circular dependencies
class SageVis;
class VisBaseContainer;
class VisBaseElement;
class VisBaseLayout;
class LayoutThread;
class LineConnectorElement;


/** VisBaseWidget class.
  * All VisWidgets in SAGEVis will inherit from this class.
  * VisWidgets load, store and present data.
  * They are capable of forming connections with other VisWidgets.
  *
  *
  * VisWidgets store sets of elements, sets of containers and sets of dataPackets associated with the elements on the map.
  *     -Sets of elements example:  Suppose this is a graph visualization.  You will need a set of nodes, a set of edges and a set of labels.
  *     -Sets of containers example:  Suppose this is a visualization with multiple timelines.
  *                                     Each timeline is a container for other elements- such as labels, dates, pictures, etc.
  *                                     The timeslines are containers, and are stored as a set of timelines.
  *     -Sets of data packets example:  Suppose this is a map visualization.  Locations on the map may store the place name, population, average temperature, rainfall ... etc
  *
  * Note:  I am not currently including the sets of data packets, because I am not sure how to treat an abstract class of them.
  *         But, I may want to include them in the future.
  *         The intended function of storing sets of data packets is to make it easier for the widget to search over the data and perform operations on relevant data items.
  *         Alternatively, I could just operate on elements and containers, but sometimes elements contain other elements and this hierarchy can be annoying to traverse
  *         Instead, I might store all the data packets within the widget, allowing for faster retrieval of relevant child elements
  *
  * The goal of this class is to:
  *     1) provide a common language and structure to support communication between visualizations
  *     2) implement common operations that can be performed on sets of generic elements and sets of generic containers
  *
  * At the moment, VisBaseWidgets all have to contain the following functions:
  *     Loading data:
  *         void loadData()
  *         void loadData( QString pathToData )
  *     Arranging data:
  *         void processLayoutSignal();
            void runLayoutSlot();
  *     Animations:
  *         slot void advance();
  *     Painting:
  *          QRectF boundingRect() const;
  *          QPainterPath shape() const;
  *          void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
  *
  * Eventually, I will implement ways to connect VisWidgets and store the possible connection types that this widget accepts.
  * Also, I will need to implement mechanisms to get data out of the widget to be passed to other widgets
  * Finally, it may be useful to implement common operations in this class - searching for an element, for instance,
  * or laying out elements in a particular way.  For now, however, I will leave this up to the specific widget until
  * I decide if it is possible to generalize these operations to include within the VisBaseWidget class.
*/
class VisBaseWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    VisBaseWidget();
};

#endif // VISBASEWIDGET_H
