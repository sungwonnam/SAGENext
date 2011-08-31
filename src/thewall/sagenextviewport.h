#ifndef SAGENEXTVIEWPORT_H
#define SAGENEXTVIEWPORT_H

#include <QGraphicsView>
#include <QtGui>

class SAGENextLauncher;

class SAGENextViewport : public QGraphicsView
{
	Q_OBJECT
public:
	explicit SAGENextViewport(int viewportId = 0, QWidget *parent = 0);
	~SAGENextViewport();

	inline int viewportId() const {return _viewportID;}

	inline void setLauncher(SAGENextLauncher *l) {_launcher = l;}


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

	QAction *_openMediaAction;
	QAction *_closeAllAction;

	SAGENextLauncher *_launcher;

signals:

public slots:
	/**
	  CTRL + O will trigger file dialog
	  */
	void on_actionOpen_Media_triggered();

	/**
	  When a file is selected on the file dialog, trigger launcher to launch a widget
	  */
	void on_actionFilesSelected(const QStringList &selected);

	/**
	  CTRL + SHIFT + W  closes all user application
	  */
	void on_actionCloseAll_triggered();



	/**
	  Saves geometry info of all running user widgets.
	  Currently it save only geometry infos
	  */
	void on_actionSaveSession_triggered();
};

#endif // SAGENEXTVIEWPORT_H
