#ifndef SAGECOMMONDEFINITIONS_H
#define SAGECOMMONDEFINITIONS_H


namespace OldSage {

const int REG_APP = 1; // from SAIL to fsm
//const int REG_MSG_SIZE = 1024; // previously google code version
const int REG_MSG_SIZE = 128; // cube Ratko version and google code version is now 128 Byte as well
const int NOTIFY_APP_SHUTDOWN = 7;


/**
  The offset for the message to sail
  */
const int SAIL_MESSAGE = 30000;

const int SAIL_INIT_MSG             = SAIL_MESSAGE + 102; // from fsm to SAIL
const int SAIL_CONNECT_TO_RCV       = SAIL_MESSAGE + 101; // from fsm to SAIL
const int SAIL_CONNECT_TO_RCV_PORT  = SAIL_MESSAGE + 109;
const int SAIL_CONNECTED_TO_RCV     = SAIL_MESSAGE + 5;
const int SAIL_SHUTDOWN             = SAIL_MESSAGE + 3; // from SAIL to fsm
const int SAIL_INIT_STREAM          = SAIL_MESSAGE + 9;
const int SAIL_SET_RAIL             = SAIL_MESSAGE + 110;
const int SAIL_FRAME_RATE           = SAIL_MESSAGE + 105;



/**
  app message offset. The message code larger than this will be queued in the appMsgQueue of SAIL.
  An application periodically calls SAIL::checkMsg() to pop from appMsgQueue
  */
const int APP_MESSAGE = 31000;
const int APP_QUIT = 33000; // from FSM to app
const int EVT_KEY = APP_MESSAGE + 7; // from UI to App. App will be able to read by runngin SAIL::checkMsg()

const int MESSAGE_FIELD_SIZE = 9;
const int MESSAGE_HEADER_SIZE = 36;

class sageMessage {
	private:
		char *dest;
		char *code;
		char *appCode;
		char *size;

		void *data;
		char *buffer;
		int bufSize;
		int clientID;

	public:
		sageMessage();
		~sageMessage();
		// init by message contents
		int init(int dst, int co, int app, int sz, const void *dat);
		// init by message buffer size
		//void init();
		int init(int);
		int destroy();
		int set(int dst, int co, int app, int sz, const void *dat);
		int getDest();
		int getCode();
		int getAppCode();
		int getSize();
		int getBufSize() { return bufSize; }
		void* getData();
		char* getBuffer();
		int getClientID() { return clientID; }

		int setDest(int);
		int setCode(int);
		int setAppCode(int);
		int setSize(int s);
		int setData(int, void*);
		void setClientID(int id) { clientID = id; }
};

int send(int, void *, int);
int recv(int, void *, int, int);

}

#endif // SAGECOMMONDEFINITIONS_H
