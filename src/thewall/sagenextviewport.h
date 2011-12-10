#ifndef SAGENEXTVIEWPORT_H
#define SAGENEXTVIEWPORT_H

#include <QGraphicsView>
#include <QtGui>

class SN_Launcher;
class SN_TheScene;

class SN_Viewport : public QGraphicsView
{
	Q_OBJECT
public:
	explicit SN_Viewport(SN_TheScene *s, int viewportId = 0, SN_Launcher *l = 0, QWidget *parent = 0);
	~SN_Viewport();

	inline int viewportId() const {return _viewportID;}
	
protected:
	void timerEvent(QTimerEvent *);

private:
	/**
	  The unique viewportID when there are multiple views (for example, a view for each X screen)
	  Only viewportID 0 will implement QAction related stuff here
	  */
	int _viewportID;

	int _swapBufferTimerId;

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

	SN_Launcher *_launcher;

	SN_TheScene *_sageNextScene;

public slots:
	/**
	  This slot opens the file dialog
	  */
	void on_actionOpen_Media_triggered();

};

#endif // SAGENEXTVIEWPORT_H
