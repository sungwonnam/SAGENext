#include "snclock.h"

//#include "../../../common/commonitem.h"
//#include "../../base/perfmonitor.h"
//#include "../../base/appinfo.h"

snClock::snClock()
    :SN_BaseWidget(Qt::Window)
{
    setWindowFlags(Qt::Window);

    QTimer *timer = new QTimer();
    layout =  new QGraphicsLinearLayout(Qt::Horizontal);

    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(100);

    // create label
    time_label = new QLabel();
    time_label->setFrameStyle(QFrame::Box);
    time_label->setLineWidth(8);

    //
    // create proxywidget for label
    //
    proxy_time_label = new QGraphicsProxyWidget(this, Qt::Widget);
    proxy_time_label->setWidget(time_label);
    proxy_time_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding, QSizePolicy::Label);

    layout->addItem(proxy_time_label);

    setLayout(layout);

    resize(175 , 100);

    //setWindowFrameMargins(0, 0, 0, 0);

    //setContentsMargins(0, 0,0,0);
}

snClock::~snClock()
{
    
}

void snClock::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    SN_BaseWidget::paint(painter, option, widget);
}

SN_BaseWidget * snClock::createInstance() {
    return new snClock;
}

void snClock::update() {
    // grab current time and set the label to that
    QTime currTime = QTime::currentTime();
    time_label->setText(currTime.toString("h:mm ap"));
    time_label->update();
}

Q_EXPORT_PLUGIN2(snClockPlugin, snClock)
