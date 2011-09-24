#include "pdfviewerwidget.h"
#include "base/perfmonitor.h"

#include <QPainter>
#include <QPushButton>
#include <QGraphicsProxyWidget>

PDFViewerWidget::PDFViewerWidget(const QString filename, quint64 globalappid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: BaseWidget(globalappid, s, parent, wflags)
	, _pdfFileName(filename)
	, _document(0)
	, _currentPage(0)
	, _currentPageIndex(0)
{
	_document = Poppler::Document::load(filename);
	if (!_document || _document->isLocked()) {
		qCritical("%s::%s() : Couldn't open pdf file %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
		deleteLater();
		return;
	}

	// this is slow
//	_document->setRenderBackend(Poppler::Document::ArthurBackend); // qt4

	// faster..
	_document->setRenderBackend(Poppler::Document::SplashBackend); // default

//	_document->setRenderHint(Poppler::Document::Antialiasing, true);
//	_document->setRenderHint(Poppler::Document::TextAntialiasing, true);


	/*
	  The caller gets the ownership of the returned object. Deleting _document won't delete the page automatically
	  */
//	_currentPage = _document->page(_currentPageIndex);
//	if (_currentPage) {
//		qDebug() << _currentPage->pageSizeF();
//		resize(_currentPage->pageSizeF());
//		setTransformOriginPoint(size().width()/2 , size().height()/2);
//	}

	setCurrentPage(0);

/**
	QPushButton *_rbtn = new QPushButton(QIcon(":/resources/rightbox.png"), "");
	connect(_rbtn, SIGNAL(clicked()), this, SLOT(nextPage()));
	QGraphicsProxyWidget *rButton = new QGraphicsProxyWidget(this);
	rButton->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);  // button won't scale when parent's scale changes
	rButton->setWidget(_rbtn);
	rButton->setPos( 10, size().height() - rButton->size().height() );
	*/

}

PDFViewerWidget::~PDFViewerWidget() {

	if (_currentPage) delete _currentPage;
	if (_document) delete _document;

	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void PDFViewerWidget::prevPage() {
	if ( _currentPageIndex == 0 ) return;
	setCurrentPage(_currentPageIndex - 1);
}

void PDFViewerWidget::nextPage() {
	if ( _currentPageIndex >= _document->numPages() - 1) return;
	setCurrentPage(_currentPageIndex + 1);
}

void PDFViewerWidget::setCurrentPage(int pageNumber) {
	if (pageNumber < 0  || pageNumber >=  _document->numPages()) return;

	if (_currentPage) {
		delete _currentPage;
		_currentPage = 0;
	}
	_currentPage = _document->page(pageNumber);
	_currentPageIndex = pageNumber;

//	qint64 start = QDateTime::currentMSecsSinceEpoch();

#if QT_VERSION >= 0x040700
	_pixmap.convertFromImage(_currentPage->renderToImage(250, 250));
#else
	_pixmap = QPixmap::fromImage(_currentPage->renderToImage(300, 300));
#endif

//	qint64 end = QDateTime::currentMSecsSinceEpoch();
//	qDebug() << end - start << "msec for rendering";

//	qDebug() << _pixmap.size();
	resize(_pixmap.size());
}

void PDFViewerWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	if (!_currentPage) return;
	painter->setRenderHint(QPainter::SmoothPixmapTransform);


	if (_perfMon)
		_perfMon->getDrawTimer().start();


	/**
	  This is slow with Arthur backend
	  */
//	_currentPage->renderToPainter(painter); // with Arthur renderBackend

	if (!_pixmap.isNull())
		painter->drawPixmap(0, 0, _pixmap);


	BaseWidget::paint(painter, option, widget);

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}