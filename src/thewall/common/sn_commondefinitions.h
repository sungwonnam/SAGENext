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

/*!
  From wall to uiclient
  */
#define EXTUI_MSG_SIZE 1280

/*!
  From uiclient to wall
  */
#define EXTUI_SMALL_MSG_SIZE 128


namespace SAGENext {

enum MEDIA_TYPE { MEDIA_TYPE_UNKNOWN = 100
	              , MEDIA_TYPE_IMAGE
	              , MEDIA_TYPE_VIDEO
	              , MEDIA_TYPE_LOCAL_VIDEO /* mplayer (using SAIL) plays video file located in local disk */
	              , MEDIA_TYPE_AUDIO
	              , MEDIA_TYPE_PLUGIN
	              , MEDIA_TYPE_VNC
	              , MEDIA_TYPE_WEBURL
	              , MEDIA_TYPE_PDF
	              , MEDIA_TYPE_SAGE_STREAM
                  , MEDIA_TYPE_QML
                };


/* transfer file / stream pixel / stream file */
//enum EXTUI_TRANSFER_MODE { FILE_TRANSFER, FILE_STREAM, PIXEL_STREAM };


/* types of messages between external UI and the wall */
enum EXTUI_MSG_TYPE { MSG_NULL
	                  , REG_FROM_UI
	                  , ACK_FROM_WALL
	                  , DISCONNECT_FROM_WALL
	                  , WALL_IS_CLOSING
	                  , TOGGLE_APP_LAYOUT, RESPOND_APP_LAYOUT /* not used */

	                  , VNC_SHARING

	                  , POINTER_PRESS, POINTER_RIGHTPRESS
	                  , POINTER_RELEASE, POINTER_RIGHTRELEASE
	                  , POINTER_CLICK, POINTER_RIGHTCLICK, POINTER_DOUBLECLICK
	                  , POINTER_DRAGGING, POINTER_RIGHTDRAGGING, POINTER_MOVING
	                  , POINTER_SHARE, POINTER_WHEEL, POINTER_UNSHARE

	                  , RESPOND_STRING /* to send text string from uiclient to wall */

	                  , REQUEST_FILEINFO /* upon receiving, wall sends file info to uiclient. uiclient must send _globalAppId of the application */
	                  , RESPOND_FILEINFO /* uiserver responds with the fileinfo of the selected application on the wall */

	                  /*
	                  Below is to interact with widget directly from a uiclient
	                  */
	                  , WIDGET_Z
	                  , WIDGET_REMOVE
	                  , WIDGET_CHANGE
	                  , WIDGET_CLOSEALL

                      , FILESERVER_RECVING_FILE /* FileServer is receiving a file from a uiClient. Msg body contains bytes received so far */
                      , FILESERVER_SENDING_FILE
                    };
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
