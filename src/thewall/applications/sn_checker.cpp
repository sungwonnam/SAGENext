#include "sn_checker.h"

#include "base/appinfo.h"
#include "base/perfmonitor.h"

#include <sys/time.h>

#include <QtGui>

SN_Checker::SN_Checker(const QSize &imagesize, qreal framerate, const quint64 appid, const QSettings *s, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : SN_BaseWidget(appid, s, parent, wFlags)
    , _image(0)
{
	_image = new QImage(imagesize , QImage::Format_RGB32);
	qDebug() << "SN_Checker : bytecount" << _image->byteCount() << "Byte";
	qDebug() << "SN_Checker : depth" << _image->depth() << "bpp";
	qDebug() << "SN_Checker : w x h" << _image->width() * _image->height();

	int fmargin = _settings->value("gui/framemargin", 0).toInt();
	resize(_image->width() + fmargin*2, _image->height() + fmargin * 2 );

	_appInfo->setFrameSize(_image->width(), _image->height(), _image->depth());
	_perfMon->setExpectedFps(framerate);
	_perfMon->setAdjustedFps(framerate);

	_timerid = startTimer(1000/framerate);
//	qDebug() << _timerid;

//	connect(&_timer, SIGNAL(timeout()), this, SLOT(doUpdate()));
//	_timer.start(1000/framerate);

	if(_perfMon) {
		_perfMon->getRecvTimer().start(); //QTime::start()
	}
}

SN_Checker::~SN_Checker() {
//	killTimer(_timerid);
	if (_image) delete _image;
}

void SN_Checker::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {

	if (_perfMon) {
		_perfMon->getDrawTimer().start();
		//perfMon->startPaintEvent();
	}

	SN_BaseWidget::paint(painter,o,w);

	if (_image && !_image->isNull()) {
		painter->drawImage(_settings->value("gui/framemargin", 0).toInt(), _settings->value("gui/framemargin", 0).toInt(), *_image);
	}

	if (_perfMon)
		_perfMon->updateDrawLatency(); // drawTimer.elapsed() will be called.
}

void SN_Checker::timerEvent(QTimerEvent *e) {

	if (e->timerId() == _timerid) {
		if (_image) {

			gettimeofday(&lats, 0);

			unsigned char *buffer = _image->bits();

			int byteperpixel = _image->depth() / 8;
			int numpixel = _image->width() * _image->height();
			//int numpixel = _image->byteCount() / (_image->depth() / 8); // image size (in Byte) / Byte per pixel

			unsigned char red = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
			unsigned char green = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));
			unsigned char blue = (unsigned char)(255 * ((qreal)qrand() / (qreal)RAND_MAX));

			//
			// for each pixel. This loop is expensive because of the internal detach()
			// so use QRgb * instead of unsigned char *
			//
//			for (int i=0; i<numpixel; i++) {
//				buffer[byteperpixel * i + 3] = 0xff; // alpha
//				buffer[byteperpixel * i + 2] = red; // red
//				buffer[byteperpixel * i + 1] = green; // green
//				buffer[byteperpixel * i + 0] = blue; // blue
//			}


			QRgb *rgb = (QRgb *)(_image->scanLine(0)); // An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.
			for (int i=0; i<numpixel; i++) {
				rgb[i] = qRgb(red, green, blue);
			}


			gettimeofday(&late, 0);
			update();



			if (_perfMon) {
				qreal networkrecvdelay = ((double)late.tv_sec + (double)late.tv_usec * 0.000001) - ((double)lats.tv_sec + (double)lats.tv_usec * 0.000001);
				_perfMon->updateObservedRecvLatency(_image->byteCount(), networkrecvdelay, ru_start, ru_end);
			}
		}
	}

	SN_BaseWidget::timerEvent(e);
}

