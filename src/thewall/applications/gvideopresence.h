#ifndef GVIDEOPRESENCE_H
#define GVIDEOPRESENCE_H

#include <qxmpp/QXmppPresence.h>

class GVideoPresence : public QXmppPresence
{

public:
    GVideoPresence();
    void toXml(QXmlStreamWriter *xmlWriter) const;
};

#endif // GVIDEOPRESENCE_H
