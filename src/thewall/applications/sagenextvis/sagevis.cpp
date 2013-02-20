#include "sagevis.h"

#include "../base/basewidget.cpp"

#include "GenoSage/genosage.h"
#include "VisBaseClasses/visbasewidget.h"
//#include "GenoSage/MultiGeneRegionVisWidget/multigeneregionviswidget.h"

//********************************CONSTRUCTOR and DESTRUCTOR***************************************//

//SageVis constructor
//  Create the program files directory - where intermediate files can be stored
//  Create timer that controls updating and animation of objects
SageVis::SageVis(QGraphicsItem *parent) :
  QGraphicsWidget(parent)
{
  qDebug() << "SageVis class created" << endl;

  //-------------------------------------------------------------
  //PROGRAM FILES DIRECTORY: Create a directory to store application files
  //  This is where intermediate data files can be stored
  //  It is in the same path as the application itself
  //   which makes it easier to launch a standalone application
  qDebug() << "Creating SageVis program files directory at: " << QApplication::applicationDirPath().append("/programFiles/dataFiles") << endl;
  QDir dir(QApplication::applicationDirPath().append("/programFiles/dataFiles"));
  if (!dir.exists()) {
    dir.mkpath(".");
  }
  //-------------------------------------------------------------
  //KEYPRESS EVENTS: Ensure that SageVis can recieve keypress events
  //  Note: not sure this is needed anymore
  setFlag( QGraphicsItem::ItemIsFocusable );
  //-------------------------------------------------------------
  //CREATE TIMER
  //  Note: this may make timer in main unnecessary
  timer = new QTimer(this);
  timer->setInterval( 100 );
  timer->start();
}

//Destructor
//  Remove widgets and delete timer
SageVis::~SageVis()
{
  //Turn on once widgets added
  //    for(int i = 0 ; i < widgets.size(); i++)
  //        if( widgets[i] )
  //            delete widgets[i];

  delete timer;

  //remove programFiles data directory
  //  need to debug this
  //QDir dir;
  //dir.removeRecursively( QApplication::applicationDirPath().append("/programFiles/dataFiles")
} 

void SageVis::addVisWidget( VisBaseWidget* w )
{
    w->addSageVisParent(this);
	qDebug() << "added vis widget " << endl;
}
