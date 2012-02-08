#ifndef PDFVIEWERWIDGET_H
#define PDFVIEWERWIDGET_H

#include "base/basewidget.h"

#include <poppler-qt4.h>

class SN_PixmapButton;

/*!
  This application uses poppler-qt4 library to render pdf document.
  Rendered image is stored in QImage object and QPixmap is used to draw onto the screen.
  */
class SN_PDFViewerWidget : public SN_BaseWidget
{
	Q_OBJECT
public:
    SN_PDFViewerWidget(const QString filename, quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);
	~SN_PDFViewerWidget();

protected:
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);


private:
	/**
	  PDF file name
	  */
	QString _pdfFileName;

	/**
	  Poppler document pointer
	  */
	Poppler::Document * _document;

	/**
	  pointer to the current page

	  Use QImage Poppler::Page::renderToImage or bool Poppler::Page::renderToPainter to render the page
	  Use QImage Poppler::Page::thumbnail() for thumbnail
	  */
//	Poppler::Page * _currentPage;

	/**
	  0 as the first page
	  */
	int _currentPageIndex;

	QMap<int, QPixmap> _pixmaps;

	int _dpix;
	int _dpiy;

	qreal _pagewidth;
	qreal _pageheight;

	qreal _pagespacing;

	/**
	  how many pages to show
	  */
	int _multipageCount;

	SN_PixmapButton *_prevButton;
	SN_PixmapButton *_nextButton;
	SN_PixmapButton *_incPageButton;
	SN_PixmapButton *_decPageButton;

	void setButtonPos();

public slots:
	void setCurrentPage(int pageNumber);

	void nextPage();

	void prevPage();

	/**
	  increase number of pages showing on this widget
	  */
	void increasePage();

	void decreasePage();

};

#endif // PDFVIEWERWIDGET_H
