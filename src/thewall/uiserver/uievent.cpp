#include "uievent.h"

UiEvent::UiEvent(QByteArray *m, QEvent::Type t) : QEvent(t), msg(m)
{
	//this->msg = msg;
	msgCode = MSG_NULL;
}

UiEvent::~UiEvent() {
	qDebug("UiEvent::%s() ",__FUNCTION__);
	if (msg ) delete msg;
}
