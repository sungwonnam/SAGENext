#ifndef SAGECOMMONDEFINITIONS_H
#define SAGECOMMONDEFINITIONS_H


namespace OldSage {

const int REG_APP = 1;
const int REG_MSG_SIZE = 1024;
const int NOTIFY_APP_SHUTDOWN = 7;
const int SAIL_INIT_MSG = 30102;
const int SAIL_CONNECT_TO_RCV = 30101;
const int SAIL_SHUTDOWN = 30003;
const int SAIL_INIT_STREAM = 30009;
const int SAIL_SET_RAIL = 30110;
const int APP_QUIT = 33000;

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
