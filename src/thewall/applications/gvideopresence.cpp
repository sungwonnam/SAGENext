#include "gvideopresence.h"
#include "qxmpp/QXmppUtils.h"
#include <QtDebug>
#include <QtXml/QDomElement>
#include <QXmlStreamWriter>
#include "qxmpp/QXmppConstants.h"

GVideoPresence::GVideoPresence()
    :QXmppPresence()
{
}

void GVideoPresence::toXml(QXmlStreamWriter *xmlWriter) const
{
    xmlWriter->writeStartElement("presence");
    helperToXmlAddAttribute(xmlWriter,"xml:lang", lang());
    helperToXmlAddAttribute(xmlWriter,"id", id());
    helperToXmlAddAttribute(xmlWriter,"to", to());
    helperToXmlAddAttribute(xmlWriter,"from", from());
    //helperToXmlAddAttribute(xmlWriter,"type", presence_types[d->type]);

    status().toXml(xmlWriter);
    //d->status.toXml(xmlWriter);

    error().toXml(xmlWriter);

    // XEP-0045: Multi-User Chat
    /*if(d->mucSupported) {
        xmlWriter->writeStartElement("x");
        xmlWriter->writeAttribute("xmlns", ns_muc);
        if (!d->mucPassword.isEmpty())
            xmlWriter->writeTextElement("password", d->mucPassword);
        xmlWriter->writeEndElement();
    }*/

    /*if(!mucItem().isNull() || mucStatusCodes.isEmpty())
    {*/
        xmlWriter->writeStartElement("x");
        xmlWriter->writeAttribute("xmlns", ns_muc_user);
        //if (!mucItem.isNull())
        //    mucItem.toXml(xmlWriter);
        /*foreach (int code, mucStatusCodes) {
            xmlWriter->writeStartElement("status");
            xmlWriter->writeAttribute("code", QString::number(code));
            xmlWriter->writeEndElement();
        }*/
        xmlWriter->writeEndElement();
    //}

    // XEP-0153: vCard-Based Avatars
    /*if(d->vCardUpdateType != VCardUpdateNone)
    {
        xmlWriter->writeStartElement("x");
        xmlWriter->writeAttribute("xmlns", ns_vcard_update);
        switch(d->vCardUpdateType)
        {
        case VCardUpdateNoPhoto:
            helperToXmlAddTextElement(xmlWriter, "photo", "");
            break;
        case VCardUpdateValidPhoto:
            helperToXmlAddTextElement(xmlWriter, "photo", d->photoHash.toHex());
            break;
        case VCardUpdateNotReady:
            break;
        default:
            break;
        }
        xmlWriter->writeEndElement();
    }*/

    /*if(capabilityNode.isEmpty() && capabilityVer.isEmpty()
            && capabilityHash.isEmpty())
    {*/
        // TODO: For now hard code these and then come back to them later
        xmlWriter->writeStartElement("c");
        xmlWriter->writeAttribute("xmlns", ns_capabilities);
        helperToXmlAddAttribute(xmlWriter, "hash", "sha-1");
        helperToXmlAddAttribute(xmlWriter, "node", "http://code.google.com/p/qxmpp");
        helperToXmlAddAttribute(xmlWriter, "ver", "nCPcZQqu7V2labjzk1+n1cRN26I=");
        helperToXmlAddAttribute(xmlWriter, "ext", "video-v1");
        //helperToXmlAddAttribute(xmlWriter, "ext", "voice-v1 video-v1 camera-v1");
        xmlWriter->writeEndElement();
    //}

    // other extensions
    QXmppStanza::extensionsToXml(xmlWriter);

    xmlWriter->writeEndElement();

}
