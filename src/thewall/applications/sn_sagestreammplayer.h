#ifndef SN_SAGESTREAMMPLAYER_H
#define SN_SAGESTREAMMPLAYER_H

#include "base/sagestreamwidget.h"


class SN_PixmapButton;

class SN_SageStreamMplayer : public SN_SageStreamWidget
{
	Q_OBJECT

public:
	SN_SageStreamMplayer(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm = 0, QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);

private:
	SN_PixmapButton *_pauseButton;
	SN_PixmapButton *_playButton;
	SN_PixmapButton *_fforwardButton;
	SN_PixmapButton *_rewindButton;

public slots:
	void pauseMplayer(int priotiry);
	void playMplayer(int priotiry);
	void fforwardMplayer(int priotiry);
	void rewindMplayer(int priotiry);
};

#endif // SN_SAGESTREAMMPLAYER_H
