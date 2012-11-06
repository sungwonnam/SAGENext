#include "sn_pdfvieweropenglwidget.h"

#include "base/perfmonitor.h"
#include "base/appinfo.h"
#include "../common/commonitem.h"


SN_PDFViewerOpenGLWidget::SN_PDFViewerOpenGLWidget(const QString filename, quint64 globalappid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wflags)
	: SN_BaseWidget(globalappid, s, parent, wflags)
	, _pdfFileName(filename)
    , _texid(0)
    , _texture(0)
	, _document(0)
//	, _currentPage(0)
	, _currentPageIndex(0)
    , _imageBpp(32)
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

	setWidgetType(SN_BaseWidget::Widget_Image);

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
        QImage img = p->renderToImage(_dpix, _dpiy);

        if (!img.isNull()) {
            _pageImages.insert(0, img.rgbSwapped());
            delete p;

            _pagewidth = img.width();
            _pageheight = img.height();
            _imageBpp = img.depth();

            resize(img.size());
            _appInfo->setFrameSize(size().width(), size().height(), img.depth());

            _updateGLtexture();
        }
        else {
            qCritical("%s::%s() : Failed to convert a page to QImage", metaObject()->className(), __FUNCTION__);
            deleteLater();
            return;
        }
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

SN_PDFViewerOpenGLWidget::~SN_PDFViewerOpenGLWidget() {
	if (_document) delete _document;
//	qDebug("%s::%s()", metaObject()->className(), __FUNCTION__);

    if (glIsTexture(_texid)) {
		glDeleteTextures(1, &_texid);
	}

    if (_texture) {
        free(_texture);
        _texture = 0;
    }
}

void SN_PDFViewerOpenGLWidget::_updateGLtexture() {
    // determine texture buf size

    if (!_useOpenGL) return;

    if (glIsTexture(_texid)) {
		glDeleteTextures(1, &_texid);
	}

    if (_texture) {
        free(_texture);
        _texture = 0;
    }

    if (_pageImages.size() < 1) return;

    _texture = (unsigned char*)malloc(sizeof(unsigned char*) * _pagewidth * _pageheight * (_imageBpp/8) * _multipageCount);

    unsigned char * texpointer = _texture;

    // for each scanline
    for (int i=0; i<_pageheight; i++) {

        // for each page
        QMap<int, QImage>::iterator it = _pageImages.begin();
        for (; it!=_pageImages.end(); it++) {
            const QImage &image = it.value();
            Q_ASSERT(!image.isNull());

            memcpy(texpointer, image.scanLine(i), (_pagewidth * _imageBpp/8));

            // move to next page
            texpointer += (_pagewidth * _imageBpp/8);
        }
    }

//    resize(_pagewidth * _pageImages.size(), _pageheight);

    glGenTextures(1, &_texid);
    glBindTexture(GL_TEXTURE_2D, _texid);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _pagewidth * _multipageCount, _pageheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, _texture);

    update();
}

/*!
 * \brief SN_PDFViewerOpenGLWidget::setCurrentPage
 * \param pageNumber
 *
 * Read new page from the PDF and
 * update _pageImages data structure
 *
 * prevPage, nextPage call this function
 */
void SN_PDFViewerOpenGLWidget::setCurrentPage(int newCurPage) {
	if (newCurPage < 0  || newCurPage >=  _document->numPages()) return;

    //
    // this is to measure content update latency
    //
	if (_perfMon) _perfMon->getUpdtTimer().start();

//	if (_currentPage) {
//		delete _currentPage;
//		_currentPage = 0;
//	}

    //
    // which page should I read from the poppler
    //
	int newpageidx = 0;

    // _currentPageIndex is the first page of the multipage
    if (newCurPage > _currentPageIndex) {
		newpageidx = newCurPage + _multipageCount - 1;
    }
	else if (newCurPage < _currentPageIndex)
		newpageidx = newCurPage;

	_currentPageIndex = newCurPage;

	//
	// A new page is needed to be rendered
	//
	Poppler::Page *newpage  = _document->page(newpageidx);
	if (newpage) {
        _pageImages.insert(newpageidx, newpage->renderToImage(_dpix, _dpiy).rgbSwapped());

		delete newpage;
	}
	else {
		qCritical("%s::%s() : Couldn't create page %d", metaObject()->className(), __FUNCTION__, newpageidx);
	}

    //
    // Remove page images not being displayed
    //
	for (int i=0; i<_document->numPages(); i++) {
		if ( _currentPageIndex <= i && i < _currentPageIndex + _multipageCount) {
			// is being shown
//			qDebug() << "setCurrentPage() : page" << i << "will be shown.";
		}
		else {
			_pageImages.erase( _pageImages.find(i) );
		}
	}

//	qDebug() << "setCurrentPage() : _pixmaps size" << _pixmaps.size();

    _updateGLtexture();

	if (_perfMon) _perfMon->updateUpdateDelay();
}

void SN_PDFViewerOpenGLWidget::increasePage() {
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
		_pageImages.insert(newpageidx, p->renderToImage(_dpix, _dpiy).rgbSwapped());
		delete p;

		resize(size().width() + _pagewidth + _pagespacing, size().height());

		setButtonPos();

        _updateGLtexture();
	}
	else {
		qCritical("%s::%s() : Couldn't create page %d", metaObject()->className(), __FUNCTION__, newpageidx);
		_multipageCount--; // reset;
	}
}

void SN_PDFViewerOpenGLWidget::decreasePage() {
	if (_multipageCount == 1) return;

	int pageToBeDeleted = _currentPageIndex + _multipageCount - 1;

	_multipageCount--;

	_pageImages.erase(_pageImages.find(pageToBeDeleted));

	resize(size().width() - _pagewidth - _pagespacing, size().height());

    _updateGLtexture();

	setButtonPos();
}

void SN_PDFViewerOpenGLWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
//	painter->setRenderHint(QPainter::SmoothPixmapTransform);

	if (_perfMon) _perfMon->getDrawTimer().start();

    if (painter->paintEngine()->type() == QPaintEngine::OpenGL2) {
        if (glIsTexture(_texid))  {
            painter->beginNativePainting();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, _texid);

            glBegin(GL_QUADS);
            glTexCoord2f(0.0, 1.0); glVertex2f(0, _pageheight);
            glTexCoord2f(1.0, 1.0); glVertex2f(_pagewidth * _multipageCount, _pageheight);
            glTexCoord2f(1.0, 0.0); glVertex2f(_pagewidth * _multipageCount, 0);
            glTexCoord2f(0.0, 0.0); glVertex2f(0, 0);
            glEnd();

            painter->endNativePainting();
        }
    }
    else {
        for (int i=0; i<_multipageCount; i++) {
            painter->drawImage(i * _pagewidth, 0, _pageImages.value(i));
        }
    }


	SN_BaseWidget::paint(painter, option, widget);

	if (_perfMon) _perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_PDFViewerOpenGLWidget::setButtonPos() {
	_incPageButton->setPos(boundingRect().center().x(), boundingRect().bottom() - _incPageButton->size().height());

	_decPageButton->setPos(_incPageButton->geometry().left() - _decPageButton->size().width(), _incPageButton->geometry().y());
	_prevButton->setPos( _decPageButton->geometry().left() - _prevButton->size().width()     , _incPageButton->geometry().y());
	_nextButton->setPos(_incPageButton->geometry().right()                                   , _incPageButton->geometry().y());
}

void SN_PDFViewerOpenGLWidget::prevPage() {
	if ( _currentPageIndex == 0 ) return;
	setCurrentPage(_currentPageIndex - 1);
}

void SN_PDFViewerOpenGLWidget::nextPage() {
	if ( _currentPageIndex >= _document->numPages() - 1) return;

	if ( _currentPageIndex + _multipageCount >= _document->numPages() ) return;

	setCurrentPage(_currentPageIndex + 1);
}
