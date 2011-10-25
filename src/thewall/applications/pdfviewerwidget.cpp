#include "pdfviewerwidget.h"
#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include "../common/commonitem.h"

#include <QPainter>
#include <QPushButton>
#include <QGraphicsProxyWidget>

SN_PDFViewerWidget::SN_PDFViewerWidget(const QString filename, quint64 globalappid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: SN_BaseWidget(globalappid, s, parent, wflags)
	, _pdfFileName(filename)
	, _document(0)
	, _currentPage(0)
	, _currentPageIndex(0)
    , _dpix(200)
    , _dpiy(200)
{
	_document = Poppler::Document::load(filename);
	if (!_document || _document->isLocked()) {
		qCritical("%s::%s() : Couldn't open pdf file %s", metaObject()->className(), __FUNCTION__, qPrintable(filename));
		deleteLater();
		return;
	}

	_appInfo->setFileInfo(filename);
	_appInfo->setMediaType(SAGENext::MEDIA_TYPE_PDF);


	// this is slow
	//_document->setRenderBackend(Poppler::Document::ArthurBackend); // qt4

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

	_currentPage = _document->page(_currentPageIndex);
	_pixmap = QPixmap::fromImage(_currentPage->renderToImage(_dpix, _dpiy));

//	qreal fmargin = _settings->value("gui/framemargin", 0).toInt();
//	resize(_pixmap.width() + fmargin*2, _pixmap.height() + fmargin*2);
	resize(_pixmap.size());

	_appInfo->setFrameSize(size().width(), size().height(), _pixmap.depth());


/**
	QPushButton *_rbtn = new QPushButton(QIcon(":/resources/rightbox.png"), "");
	connect(_rbtn, SIGNAL(clicked()), this, SLOT(nextPage()));
	QGraphicsProxyWidget *rButton = new QGraphicsProxyWidget(this);
	rButton->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);  // button won't scale when parent's scale changes
	rButton->setWidget(_rbtn);
	rButton->setPos( 10, size().height() - rButton->size().height() );
	*/

	SN_PixmapButton *left = new SN_PixmapButton(":/resources/media-forward-rtl_128x128.png", 0, "", this);
	SN_PixmapButton *right = new SN_PixmapButton(":/resources/media-forward-ltr_128x128.png", 0, "", this);
	connect(left, SIGNAL(clicked()), this, SLOT(prevPage()));
	connect(right, SIGNAL(clicked()), this, SLOT(nextPage()));
	left->setPos(0, size().height()/2);
	right->setPos(size().width() - right->size().width(), size().height()/2);
}

SN_PDFViewerWidget::~SN_PDFViewerWidget() {
	if (_currentPage) delete _currentPage;
	if (_document) delete _document;

	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void SN_PDFViewerWidget::prevPage() {
	if ( _currentPageIndex == 0 ) return;
	setCurrentPage(_currentPageIndex - 1);
}

void SN_PDFViewerWidget::nextPage() {
	if ( _currentPageIndex >= _document->numPages() - 1) return;
	setCurrentPage(_currentPageIndex + 1);
}

void SN_PDFViewerWidget::setCurrentPage(int pageNumber) {
	if (pageNumber < 0  || pageNumber >=  _document->numPages()) return;

	if (_currentPage) {
		delete _currentPage;
		_currentPage = 0;
	}
	_currentPage = _document->page(pageNumber);
	_currentPageIndex = pageNumber;

	qDebug() << "hello" << pageNumber;

//	qint64 start = QDateTime::currentMSecsSinceEpoch();

	/**
#if QT_VERSION >= 0x040700
	_pixmap.convertFromImage(_currentPage->renderToImage(_dpix, _dpiy));
#else
	_pixmap = QPixmap::fromImage(_currentPage->renderToImage(_dpix, _dpiy));
#endif
**/
	_pixmap = QPixmap::fromImage(_currentPage->renderToImage(_dpix, _dpiy));

	update();

//	qint64 end = QDateTime::currentMSecsSinceEpoch();
//	qDebug() << end - start << "msec for rendering";

//	qDebug() << _currentPage->pageSizeF();
//	QSizeF sizeinch = _currentPage->pageSizeF() / 72;
//	qDebug() << sizeinch * 250;
}

void SN_PDFViewerWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	if (!_currentPage) return;

	painter->setRenderHint(QPainter::SmoothPixmapTransform);

	if (_perfMon)
		_perfMon->getDrawTimer().start();

	SN_BaseWidget::paint(painter, option, widget);

	/**
	  This is slow with Arthur backend
	  */
//	_currentPage->renderToPainter(painter); // with Arthur renderBackend

	if (!_pixmap.isNull())
		painter->drawPixmap(0,0, _pixmap);


	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}
