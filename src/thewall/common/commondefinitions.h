#ifndef COMMONDEFINITIONS_H
#define COMMONDEFINITIONS_H

//namespace SAGENEXT {
//#define EXTUI_MSG_SIZE 1280
//#define BITMASK_SIZE 32 // number of bits in the mask

//#define EXTUI_MSG_BASE 100
//#define EXTUI_MSG_REG EXTUI_MSG_BASE + 1

//
// widget inherits SN_BaseWidget and a user application
//
#define BASEWIDGET_USER 100000

//
// widget inherits SN_BaseWidget but NOT a user application
// e.g. mediabrowser
// So, it shouldn't be affected by the scheduler
//
#define BASEWIDGET_NONUSER 1000

//
// widget inherits QGraphicsWidget so it's not a user application
// But an item that can be interacted with user shared pointers
// e.g. SN_PartitionBar
//
#define INTERACTIVE_ITEM 10

namespace SAGENext {

enum MEDIA_TYPE { MEDIA_TYPE_UNKNOWN = 100, MEDIA_TYPE_IMAGE, MEDIA_TYPE_VIDEO, MEDIA_TYPE_LOCAL_VIDEO, MEDIA_TYPE_AUDIO, MEDIA_TYPE_PLUGIN , MEDIA_TYPE_VNC, MEDIA_TYPE_WEBURL, MEDIA_TYPE_PDF , MEDIA_TYPE_SAGE_STREAM};

}

//}



        /*
          What I must do is all that concerns me, not what the people think.
          An artist use lie to reveal the truth, a politician use lie to cover the truth.
          Only without fear, you're free.
          No religion, No war.
          People shouldn't be afraid of their government. Government should be afraid of her people.


          http://labs.trolltech.com/blogs/2008/10/22/so-long-and-thanks-for-the-blit/
          */
#endif // COMMONDEFINITIONS_H
