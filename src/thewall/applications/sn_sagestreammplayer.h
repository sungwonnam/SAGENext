#ifndef SN_SAGESTREAMMPLAYER_H
#define SN_SAGESTREAMMPLAYER_H

#include "applications/base/sn_sagestreamwidget.h"

class SN_PixmapButton;

class SN_SageStreamMplayer : public SN_SageStreamWidget
{
	Q_OBJECT

public:
	SN_SageStreamMplayer(const quint64 globalappid, const QSettings *s, SN_ResourceMonitor *rm = 0, QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);

//    int setQuality(qreal newQuality);

private:
	SN_PixmapButton *_pauseButton;
	SN_PixmapButton *_playButton;
	SN_PixmapButton *_fforwardButton;
	SN_PixmapButton *_rewindButton;

    bool _isMplayerPaused;

public slots:
    void setButtonPosition();

	void pauseMplayer();
	void playMplayer();
	void fforwardMplayer();
	void rewindMplayer();
};

#endif // SN_SAGESTREAMMPLAYER_H
