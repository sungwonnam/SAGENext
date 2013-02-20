#ifndef SN_PDFVIEWEROPENGLWIDGET_H
#define SN_PDFVIEWEROPENGLWIDGET_H

#include "base/basewidget.h"

#include <poppler-qt4.h>

/*
#if defined(Q_OS_LINUX)
#include <GL/gl.h>
#elif defined(Q_OS_MAC)
#include <OpenGL/gl.h>
#endif
*/
#include <QtOpenGL>

class SN_PixmapButton;

/*!
  This application uses poppler-qt4 library to render pdf document.
  Rendered image is stored in QImage object and QPixmap is used to draw onto the screen.
  */
class SN_PDFViewerOpenGLWidget : public SN_BaseWidget
{
	Q_OBJECT
public:
    SN_PDFViewerOpenGLWidget(const QString filename, quint64 globalappid, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);
	~SN_PDFViewerOpenGLWidget();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
	/*!
	  PDF file name (absolute path)
	  */
	QString _pdfFileName;

    GLuint _texid;

    unsigned char * _texture;

	/*!
	  Poppler document pointer
	  */
	Poppler::Document * _document;

	/*!
	  pointer to the current page

	  Use QImage Poppler::Page::renderToImage or bool Poppler::Page::renderToPainter to render the page
	  Use QImage Poppler::Page::thumbnail() for thumbnail
	  */
//	Poppler::Page * _currentPage;

	/*!
	  0 as the first page
	  */
	int _currentPageIndex;

    int _imageBpp;

	QMap<int, QImage> _pageImages;

	int _dpix;
	int _dpiy;

	int _pagewidth;
	int _pageheight;

	qreal _pagespacing;

	/*!
	  how many pages to show at once
	  */
	int _multipageCount;

	SN_PixmapButton *_prevButton;
	SN_PixmapButton *_nextButton;
	SN_PixmapButton *_incPageButton;
	SN_PixmapButton *_decPageButton;

	void setButtonPos();

    void _updateGLtexture();

public slots:
	void setCurrentPage(int pageNumber);

	void nextPage();

	void prevPage();

	/**
	  increase number of pages showing on this widget
      This function will resize the widget after updating _pageImages
	  */
	void increasePage();

    /*!
     * \brief decreasePage
     * This function will resize the widget after updating _pageImages
     */
	void decreasePage();
};


#endif // SN_PDFVIEWEROPENGLWIDGET_H
