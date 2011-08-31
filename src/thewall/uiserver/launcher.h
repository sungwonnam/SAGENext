#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QtGui>

class QParallelAnimationGroup;
class GraphicsViewMain;

/*!
  right click on the empty scene surface triggers this
  */
class Launcher : public QGraphicsWidget
{
public:
	Launcher(GraphicsViewMain *gvm, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = Qt::Window);
	~Launcher();

private:
	GraphicsViewMain *gvMain;

	QGraphicsLinearLayout *linearLayout;

	QToolButton *webButton;

	QParallelAnimationGroup *animGroup;

	QGraphicsPixmapItem *imageIcon;
	QGraphicsPixmapItem *videoIcon;

	QGraphicsPixmapItem *docIcon;
	QGraphicsPixmapItem *webIcon;
	QGraphicsPixmapItem *pluginIcon;

	QGraphicsPixmapItem *prefIcon;

	QGraphicsPixmapItem *saveIcon;
	QGraphicsPixmapItem *loadIcon;
};

#endif // LAUNCHER_H
