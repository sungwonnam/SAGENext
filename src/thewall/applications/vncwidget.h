#ifndef VNCWIDGET_H
#define VNCWIDGET_H

#include "base/railawarewidget.h"
#include "base/basewidget.h"

#include <rfb/rfbproto.h>
#include <rfb/rfbclient.h>

#include <QHostAddress>
#include <QtCore>



class SN_VNCClientWidget : public SN_RailawareWidget
{
public:
        SN_VNCClientWidget(quint64 globalappid, const QString senderIP, int display, const QString username, const QString passwd, int framerate, const QSettings *s, QGraphicsItem *parent = 0, Qt::WindowFlags wflags = 0);
        ~SN_VNCClientWidget();

protected:
        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
        rfbClient *vncclient;
        QHostAddress serverAddr;
        quint16 serverPort;

        /*!
          pixel buffer
          */
        QImage *_image;

        /*!
          paint() will draw this
          */
        QPixmap pixmap;

        /*!
          copy vnc framebuffer to QImage
          */
        void receivingThread();

        bool _end;

        int framerate;

        QFuture<void> future;

		static rfbCredential * getCredential(struct _rfbClient *client, int credentialType);

        static rfbBool got_data;
		static QString username;
        static QString vncpasswd;

        static void signal_handler(int signal);

        static rfbBool resize_func(rfbClient* client);

        static void frame_func(rfbClient* client);

        static rfbBool position_func(rfbClient* client, int x, int y);

        static char *password_func(rfbClient* client);

        static void update_func(rfbClient* client,int x,int y,int w,int h);

public slots:
        /*!
          Reimplementing virtual function.
          convert QImage to QPixmap and schedule QGraphicsItem::update()
          */
        void scheduleUpdate();

        /*!
          Reimplementing virtual function.
          */
        void scheduleReceive() {}

};

#endif // VNCWIDGET_H
