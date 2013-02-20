#ifndef SAGEVIS_H
#define SAGEVIS_H

#include <QObject>
#include <QGraphicsWidget>
#include <QRectF>
#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <QVector>
#include <QPointer>
#include <QDebug>
#include <QList>
#include <QTimer>
#include <QProcess>
#include <QFileDialog>
#include <QApplication>

//Forward declarations to avoid circular dependencies
//class GenoSage;// removing GenoSage for now... No need for genosage organizing class.  
class VisBaseWidget;
class MultiGeneRegionVisWidget;

class SN_BaseWidget;

//Which data type is being loaded.
//CHANGE NEEDED:  Later should be able to load data from different origins.
#define     PROPRIETARY_DATA     0
#define     PUBMED_DATA     1
const int DATA_TYPE = PUBMED_DATA;

/**SageVis class.
 * SageVis manages VisWidgets in SageNext.
 * At the moment it primarily launches and stores the VisWidgets.
 * Eventual functions in this class likely include:
 *    - control linking between vis widgets
 *    - control placement of vis widgets -- special tiling schemes
 *    - coordinates between GUI control bars and actions within the widget
 */

class SageVis : public QGraphicsWidget
{
    Q_OBJECT

public:
  /** SageVis constructor.
   * Constructor may take a QGraphicsItem as a parent, but it is not essential.
   * Presently runs parent-less in the scene.
   */
  explicit SageVis(QGraphicsItem *parent = 0);

  /** SageVis destructor.
   * Deletes VisWidgets that have been created
   * Also deletes internal timer
   */
  ~SageVis();
	
  void addVisWidget(VisBaseWidget* w );

 private:
  QList< QPointer<VisBaseWidget> > widgets;  /**< list of widgets that SageVis is managing. */
  QPointer<QTimer> timer; /**< Timer permits animations within VisWidgets.  VisWidgets must implement an 'advance' function, to catch these signals and advance the animation*/

  // QPointer<GenoSage> genoSage;  /**< genoSage object is given a pointer to all genoSageWidgets and will have its own management operations that control these widgets.*/
  //NOTE:  If more managers are created, I could make a base 'manager' class, then store a list of managers.
  //       Not sure if this is needed though, so I'll leave it for later

};

#endif // SAGEVIS_H
