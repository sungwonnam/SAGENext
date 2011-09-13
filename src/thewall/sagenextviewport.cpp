#include "sagenextviewport.h"
#include "sagenextlauncher.h"
#include "sagenextscene.h"
#include "applications/base/basewidget.h"

SAGENextViewport::SAGENextViewport(int viewportId, QWidget *parent) :
    QGraphicsView(parent),
    _viewportID(viewportId),
    _fdialog(0),
    _openMediaAction(0),
    _closeAllAction(0),
    _launcher(0)
{


	if ( _viewportID == 0 ) {
		_fdialog = new QFileDialog(this, "Open Files", QDir::homePath().append("/.sagenext/image") , "Images (*.tif *.tiff *.svg *.bmp *.png *.jpg *.jpeg *.gif *.xpm);;RatkoData (*.log);;Session (*.session);;Plugins (*.so *.dll *.dylib);;Videos (*.mov *.avi *.mpg *.mp4 *.mkv *.flv *.wmv);;Any (*)");
		_fdialog->setModal(false);
		//fdialog->setFileMode(QFileDialog::Directory);
		connect(_fdialog, SIGNAL(filesSelected(QStringList)), this, SLOT(on_actionFilesSelected(QStringList)));



		_openMediaAction = new QAction(this);
		_openMediaAction->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_O) );
		connect(_openMediaAction, SIGNAL(triggered()), this, SLOT(on_actionOpen_Media_triggered()));
		addAction(_openMediaAction);


		_closeAllAction = new QAction(this);
		_closeAllAction->setShortcut( QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W) );
		connect(_closeAllAction, SIGNAL(triggered()), this, SLOT(on_actionCloseAll_triggered()));
	}
}

SAGENextViewport::~SAGENextViewport() {
	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}




void SAGENextViewport::on_actionSaveSession_triggered() {
	QString filename(QDir::homePath());
	filename.append("/.sagenext/sessions/");
	filename.append( QDateTime::currentDateTime().toString() );
	filename.append(".session");

	QFile sFile(filename);
	if (!sFile.open(QIODevice::WriteOnly) ) {
		qCritical() << "file open error";
		return;
	}
	QDataStream out(&sFile);

	foreach (QGraphicsItem *gi, scene()->items()) {
		if (!gi) continue;
		if (gi->type() != QGraphicsItem::UserType + 2) continue;

		BaseWidget *bw = static_cast<BaseWidget *>(gi);
		out << bw->pos() << bw->scale() << bw->zValue();
	}

	sFile.close();
}




void SAGENextViewport::on_actionCloseAll_triggered() {
	QGraphicsScene *s = scene();
	foreach(QGraphicsItem *item, s->items()) {
		if (item->type() == QGraphicsItem::UserType + 2) {
			BaseWidget *bw = static_cast<BaseWidget *>(item);
			Q_ASSERT(bw);
			bw->fadeOutClose();
		}
	}
}


/*!
  Openining a media file
  */
void SAGENextViewport::on_actionOpen_Media_triggered()
{
	if ( _fdialog && _fdialog->isHidden())
		_fdialog->show();
}

void SAGENextViewport::on_actionFilesSelected(const QStringList &filenames) {
	Q_ASSERT(_launcher);

	if ( filenames.empty() ) {
		return;
	}
	else {
		//qDebug("GraphicsViewMain::%s() : %d", __FUNCTION__, filenames.size());
	}

	QRegExp rxVideo("\\.(avi|mov|mpg|mpeg|mp4|mkv|flv|wmv)$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxImage("\\.(bmp|svg|tif|tiff|png|jpg|bmp|gif|xpm|jpeg)$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxSession("\\.session$", Qt::CaseInsensitive, QRegExp::RegExp);
	QRegExp rxRatkoData("\\.log");
	QRegExp rxPlugin;
	rxPlugin.setCaseSensitivity(Qt::CaseInsensitive);
	rxPlugin.setPatternSyntax(QRegExp::RegExp);
#if defined(Q_OS_LINUX)
	rxPlugin.setPattern("\\.so$");
#elif defined(Q_OS_WIN32)
	rxPlugin.setPattern("\\.dll$");
#elif defined(Q_OS_DARWIN)
	rxPlugin.setPattern("\\.dylib$");
#endif

	for ( int i=0; i<filenames.size(); i++ ) {
		//qDebug("GraphicsViewMain::%s() : %d, %s", __FUNCTION__, i, qPrintable(filenames.at(i)));
		QString filename = filenames.at(i);


		if ( filename.contains(rxVideo) ) {
			qDebug("%s::%s() : Opening a video file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
//			w = new PhononWidget(filename, globalAppId, settings);
//			QFuture<void> future = QtConcurrent::run(qobject_cast<PhononWidget *>(w), &PhononWidget::threadInit, filename);

//			startApp(MEDIA_TYPE_LOCAL_VIDEO, filename);
			_launcher->launch((int)MEDIA_TYPE_LOCAL_VIDEO, filename);
//			QMetaObject::invokeMethod(_launcher, "launch", Qt::QueuedConnection,
//									  Q_ARG(int, MEDIA_TYPE_LOCAL_VIDEO),
//									  Q_ARG(QString, filename));
			return;
		}

		/*!
		  Image
		  */
		else if ( filename.contains(rxImage) ) {
			qDebug("%s::%s() : Opening an image file %s",metaObject()->className(), __FUNCTION__, qPrintable(filename));
//			bw = new PixmapWidget(filename, globalAppId, settings);
//			startApp(MEDIA_TYPE_IMAGE, filename);
			_launcher->launch((int)MEDIA_TYPE_IMAGE, filename);
			return;
		}


		/**
		  * plugin
		  */
		else if (filename.contains(rxPlugin) ) {
			qDebug("%s::%s() : Loading a plugin %s", metaObject()->className(),__FUNCTION__, qPrintable(filename));
			_launcher->launch((int)MEDIA_TYPE_PLUGIN, filename);
		}

		/*!
		  session
		  */
		else if (filename.contains(rxSession) ) {
//			qDebug() << "Loading a session " << filename;
//			loadSavedSession(filename);
		}


		/*!
		  Ratko data
		  */
		else if (filename.contains(rxRatkoData) ) {
		//loadSavedScenario(filename);
			/*
			QFuture<void> future = QtConcurrent::run(this, &GraphicsViewMain::loadSavedScenario, filename);
			*/
//			RatkoDataSimulator *rdsThread = new RatkoDataSimulator(filename);
//			rdsThread->start();
		}


		else if ( QDir(filename).exists() ) {
			qDebug("%s::%s() : DIRECTORY", metaObject()->className(), __FUNCTION__);
			return;
		}
		else {
			qCritical("%s::%s() : Unrecognized file format", metaObject()->className(),__FUNCTION__);
			return;
		}
	}
}
