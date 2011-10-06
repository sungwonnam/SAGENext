#include "sagenextviewport.h"
#include "sagenextlauncher.h"
#include "sagenextscene.h"
#include "applications/base/basewidget.h"

SN_Viewport::SN_Viewport(SN_TheScene *s, int viewportId, SN_Launcher *l, QWidget *parent) :
    QGraphicsView(s, parent),
    _viewportID(viewportId),
    _fdialog(0),
    _openMediaAction(0),
	_saveSessionAction(0),
    _closeAllAction(0),
    _launcher(l),
	_sageNextScene(s)
{
	setViewportMargins(0, 0, 0, 0);

	if ( _viewportID == 0 ) {
		_fdialog = new QFileDialog(this, "Open Files", QDir::homePath().append("/.sagenext") , "Any (*);;Images (*.tif *.tiff *.svg *.bmp *.png *.jpg *.jpeg *.gif *.xpm);;RatkoData (*.log);;Session (*.session);;Recordings (*.recording);;Plugins (*.so *.dll *.dylib);;Videos (*.mov *.avi *.mpg *.mp4 *.mkv *.flv *.wmv)");
		_fdialog->setModal(false);
		//fdialog->setFileMode(QFileDialog::Directory);
		connect(_fdialog, SIGNAL(filesSelected(QStringList)), _launcher, SLOT(launch(QStringList)));


		_openMediaAction = new QAction(this);
		_openMediaAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_O) );
		connect(_openMediaAction, SIGNAL(triggered()), this, SLOT(on_actionOpen_Media_triggered()));
		addAction(_openMediaAction);

		_saveSessionAction = new QAction(this);
		_saveSessionAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
		connect(_saveSessionAction, SIGNAL(triggered()), _sageNextScene, SLOT(saveSession()));
		addAction(_saveSessionAction);


		_closeAllAction = new QAction(this);
		_closeAllAction->setShortcut( QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W) );
		connect(_closeAllAction, SIGNAL(triggered()), _sageNextScene, SLOT(closeAllUserApp()));
	}
}

SN_Viewport::~SN_Viewport() {
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void SN_Viewport::on_actionOpen_Media_triggered()
{
	if ( _fdialog && _fdialog->isHidden())
		_fdialog->show();
}

