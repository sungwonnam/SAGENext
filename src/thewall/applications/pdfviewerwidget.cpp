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
//	, _currentPage(0)
	, _currentPageIndex(0)
    , _dpix(200)
    , _dpiy(200)
    , _pagewidth(0)
    , _pageheight(0)
    , _pagespacing(8)
    , _multipageCount(1)
    , _prevButton(0)
    , _nextButton(0)
    , _incPageButton(0)
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

	Poppler::Page *p = _document->page(_currentPageIndex);
	if (p) {
		QPixmap pixmap = QPixmap::fromImage(p->renderToImage(_dpix, _dpiy));
		_pixmaps.insert(0, pixmap);
		delete p;

		_pagewidth = pixmap.width();
		_pageheight = pixmap.height();

		resize(pixmap.size());
		_appInfo->setFrameSize(size().width(), size().height(), pixmap.depth());
	}
	else{
		qCritical("%s::%s() : Couldn't create page", metaObject()->className(), __FUNCTION__);
		deleteLater();
		return;
	}




/**
	QPushButton *_rbtn = new QPushButton(QIcon(":/resources/rightbox.png"), "");
	connect(_rbtn, SIGNAL(clicked()), this, SLOT(nextPage()));
	QGraphicsProxyWidget *rButton = new QGraphicsProxyWidget(this);
	rButton->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);  // button won't scale when parent's scale changes
	rButton->setWidget(_rbtn);
	rButton->setPos( 10, size().height() - rButton->size().height() );
	*/

	_prevButton = new SN_PixmapButton(":/resources/arrow-left-64x64.png", 0, "", this);
	_nextButton = new SN_PixmapButton(":/resources/arrow-right-64x64.png", 0, "", this);
	connect(_prevButton, SIGNAL(clicked()), this, SLOT(prevPage()));
	connect(_nextButton, SIGNAL(clicked()), this, SLOT(nextPage()));


	_incPageButton = new SN_PixmapButton(":/resources/plus_64x64.png", 0, "", this);
	_decPageButton = new SN_PixmapButton(":/resources/minus_64x64.png", 0, "", this);
	connect(_incPageButton, SIGNAL(clicked()), this, SLOT(increasePage()));
	connect(_decPageButton, SIGNAL(clicked()), this, SLOT(decreasePage()));

	setButtonPos();
}

SN_PDFViewerWidget::~SN_PDFViewerWidget() {
	if (_document) delete _document;

//	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);
}

void SN_PDFViewerWidget::prevPage() {
	if ( _currentPageIndex == 0 ) return;
	setCurrentPage(_currentPageIndex - 1);
}

void SN_PDFViewerWidget::nextPage() {
	if ( _currentPageIndex >= _document->numPages() - 1) return;

	if ( _currentPageIndex + _multipageCount >= _document->numPages() ) return;

	setCurrentPage(_currentPageIndex + 1);
}

void SN_PDFViewerWidget::setCurrentPage(int pageNumber) {
	if (pageNumber < 0  || pageNumber >=  _document->numPages()) return;

	_perfMon->getConvTimer().start();

//	if (_currentPage) {
//		delete _currentPage;
//		_currentPage = 0;
//	}

	int newpageidx = 0;
	if (pageNumber > _currentPageIndex)
		newpageidx = pageNumber + _multipageCount - 1;
	else if (pageNumber < _currentPageIndex)
		newpageidx = pageNumber;

	_currentPageIndex = pageNumber;

	//
	// A new page needed to be rendered
	//
	Poppler::Page *newpage  = _document->page(newpageidx);
	if (newpage) {
		_pixmaps.insert(newpageidx, QPixmap::fromImage(newpage->renderToImage(_dpix, _dpiy)));

		delete newpage;
	}
	else {
		qCritical("%s::%s() : Couldn't create page %d", metaObject()->className(), __FUNCTION__, newpageidx);

	}


	for (int i=0; i<_document->numPages(); i++) {
		if ( _currentPageIndex <= i && i < _currentPageIndex + _multipageCount) {
			// is being shown
//			qDebug() << "setCurrentPage() : page" << i << "will be shown.";
		}
		else {
			_pixmaps.erase( _pixmaps.find(i) );
		}
	}

//	qDebug() << "setCurrentPage() : _pixmaps size" << _pixmaps.size();

	_perfMon->updateConvDelay();

	update();
}

void SN_PDFViewerWidget::increasePage() {
	if(_multipageCount >= _document->numPages()) return;

	_multipageCount++;

	int newpageidx = _currentPageIndex + _multipageCount -1;
	if (newpageidx < _document->numPages()) {
		// new page is going to be the next page
	}
	else {
		// new page is going to be the previous page
		// and _currentPageIndex should point it
		_currentPageIndex--;
		newpageidx = _currentPageIndex;
	}
	Poppler::Page *p = _document->page(newpageidx);
	if (p) {
		QPixmap pixmap = QPixmap::fromImage(p->renderToImage(_dpix, _dpiy));
		_pixmaps.insert(newpageidx, pixmap);
		delete p;

		resize(size().width() + _pagewidth + _pagespacing, size().height());

		setButtonPos();
	}
	else {
		qCritical("%s::%s() : Couldn't create page %d", metaObject()->className(), __FUNCTION__, newpageidx);
		_multipageCount--; // reset;
	}
}

void SN_PDFViewerWidget::decreasePage() {
	if (_multipageCount == 1) return;

	int pageToBeDeleted = _currentPageIndex + _multipageCount - 1;

	_multipageCount--;

	_pixmaps.erase(_pixmaps.find(pageToBeDeleted));

	resize(size().width() - _pagewidth - _pagespacing, size().height());

	setButtonPos();
}

void SN_PDFViewerWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

	painter->setRenderHint(QPainter::SmoothPixmapTransform);

	if (_perfMon)
		_perfMon->getDrawTimer().start();

	SN_BaseWidget::paint(painter, option, widget);

	/**
	  This is slow with Arthur backend
	  */
//	_currentPage->renderToPainter(painter); // with Arthur renderBackend


	int count = 0;
	int page = _currentPageIndex;
	while (count < _multipageCount) {

		painter->drawPixmap(count * (_pagewidth + _pagespacing), 0, _pixmaps.value(page));

		page++;
		count++;
	}

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_PDFViewerWidget::setButtonPos() {
	_incPageButton->setPos(boundingRect().center().x(), boundingRect().bottom() - _incPageButton->size().height());

	_decPageButton->setPos(_incPageButton->geometry().left() - _decPageButton->size().width(), _incPageButton->geometry().y());
	_prevButton->setPos( _decPageButton->geometry().left() - _prevButton->size().width()     , _incPageButton->geometry().y());
	_nextButton->setPos(_incPageButton->geometry().right()                                   , _incPageButton->geometry().y());
}
