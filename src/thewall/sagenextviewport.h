#ifndef SAGENEXTVIEWPORT_H
#define SAGENEXTVIEWPORT_H

#include <QGraphicsView>
#include <QtGui>

class SAGENextLauncher;
class SAGENextScene;

class SAGENextViewport : public QGraphicsView
{
	Q_OBJECT
public:
	explicit SAGENextViewport(SAGENextScene *s, int viewportId = 0, SAGENextLauncher *l = 0, QWidget *parent = 0);
	~SAGENextViewport();

	inline int viewportId() const {return _viewportID;}

private:
	/**
	  The unique viewportID when there are multiple views (for example, a view for each X screen)
	  Only viewportID 0 will implement QAction related stuff here
	  */
	int _viewportID;

	/*!
	  File dialog (CMD or CTRL - O) to open local files
	  */
	QFileDialog *_fdialog;

	/**
	  This action fires by CMD + o and connected to the open media slot
	  */
	QAction *_openMediaAction;

	/**
	  save session through SAGENextScene::saveSession slot
	  */
	QAction *_saveSessionAction;

	/**
	  CMD + SHIFT + w closes all baseWidget using SAGENextScene::closeAllUserApp() slot
	  */
	QAction *_closeAllAction;

	SAGENextLauncher *_launcher;

	SAGENextScene *_sageNextScene;

public slots:
	/**
	  This slot opens the file dialog
	  */
	void on_actionOpen_Media_triggered();

};

#endif // SAGENEXTVIEWPORT_H
