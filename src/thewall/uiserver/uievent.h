#ifndef UIEVENT_H
#define UIEVENT_H

#include <QEvent>
#include "common/commondefinitions.h"

class UiEvent : public QEvent
{
public:
	explicit UiEvent(QByteArray *m, QEvent::Type t);
	~UiEvent();

	inline void setMsg(QByteArray *m) { msg = m; }
	inline QByteArray * getMsg() { return msg; }

	inline void setMsgCode(EXTUI_MSG_TYPE mc) { msgCode = mc; }
	inline EXTUI_MSG_TYPE getMsgCode() const { return msgCode; }

private:
	EXTUI_MSG_TYPE msgCode;
	QByteArray *msg;
};

#endif // UIEVENT_H
