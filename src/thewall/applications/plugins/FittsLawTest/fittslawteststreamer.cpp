#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

int serversocket;

// return socket or -1 on error
int waitForConnection(int port);

int sendFrame(int socket, char *buf, int buflen);

void * send_thread(void *arg);

int streamsock;

pthread_t threadid;


// width, height, framerate, listening port

#define BUF_LEN 65536
#define MAX_FPS 60

class DoubleBuffer
{
public:
	DoubleBuffer(int width, int height, int byteperpixel=3);
	~DoubleBuffer();

	void * getFrontBuffer();
	void * getBackBuffer();
	void releaseBackBuffer();
	void swapBuffer();
	void releaseLocks();
    inline int bufSize() const {return _bufsize;}

protected:
	void **_doubleBuffer;

	int _bufferIndex;
	int _queueLength;

	int _bufsize;

	int _numbuff;

	pthread_mutex_t *_mutex;
	pthread_cond_t *_notEmpty;
	pthread_cond_t *_notFull;

	int _width;
	int _height;
};

DoubleBuffer::DoubleBuffer(int width, int height, int byteperpixel)
	: _doubleBuffer(0)
	, _bufferIndex(0)
	, _queueLength(0)
	, _bufsize(width * height * byteperpixel)
	, _numbuff(2)
	, _mutex(0)
	, _notEmpty(0)
	, _notFull(0)
	, _width(width)
	, _height(height)
{
	_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	if ( pthread_mutex_init(_mutex, 0) != 0 ) {
		perror("pthread_mutex_init");
	}
	if ( pthread_mutex_unlock(_mutex) != 0 ) {
		perror("pthread_mutex_unlock");
	}

	_notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	_notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

	if ( pthread_cond_init(_notEmpty, 0) != 0 ) {
		perror("pthread_cond_init");
	}
	if ( pthread_cond_init(_notFull, 0) != 0 ) {
		perror("pthread_cond_init");
	}

    _doubleBuffer = (void **)malloc(sizeof(void *) * _numbuff);
    for (int i=0; i<_numbuff; i++) {
        _doubleBuffer[i] = (void *)malloc(sizeof(char) * width * height * byteperpixel);
    }
}

DoubleBuffer::~DoubleBuffer() {
	if(_mutex) free(_mutex);
	if(_notEmpty) free(_notEmpty);
	if(_notFull) free(_notFull);
}


void DoubleBuffer::releaseLocks() {
	pthread_mutex_lock(_mutex);
	_queueLength = -1;
	pthread_cond_signal(_notEmpty);
	pthread_cond_signal(_notFull);
	pthread_mutex_unlock(_mutex);

}

// CONSUMER
// sageWidget convert from this image
void * DoubleBuffer::getBackBuffer() {
	if(!_mutex || !_doubleBuffer) return 0;

	pthread_mutex_lock(_mutex);
//	qDebug("%s() : mutex locked", __FUNCTION__);

	while(_queueLength == 0) {
		//std::cerr << "=== waiting for new frames === " << std::endl;
		pthread_cond_wait(_notEmpty, _mutex);
	}
//	qDebug("%s() : buffer isn't empty. producer finished producing so I can consume", __FUNCTION__);

	return _doubleBuffer[1-_bufferIndex];
}

// PRODUCER
// app is writing pixel into this
void * DoubleBuffer::getFrontBuffer() {
//	qDebug() << "front buffer " << bufferIndex;
	return _doubleBuffer[_bufferIndex];
}

void DoubleBuffer::releaseBackBuffer() {
	_queueLength--;
//	qDebug() << "releaseBackBuffer() queuelen " << queueLength;
	pthread_mutex_unlock(_mutex);
//	qDebug() << "releaseBackBuffer() mutex unlocked";

	if (_queueLength == 0) {
		pthread_cond_signal(_notFull);
//		qDebug() << "releaseBackBuffer() signaled notFull";
	}
}

void DoubleBuffer::swapBuffer() {

	pthread_mutex_lock(_mutex);
//	qDebug("%s() : mutex locked", __FUNCTION__);

	while(_queueLength > 0) {
		pthread_cond_wait(_notFull, _mutex);
	}
//	qDebug("%s() : buffer is not full (sageWidget consumed it)", __FUNCTION__);

	if (_queueLength == 0) {
		_bufferIndex = 1 - _bufferIndex;
		_queueLength++;
//		qDebug("%s() : swapped. queuelen is now %d", __FUNCTION__, queueLength);
	}

	pthread_mutex_unlock(_mutex);
//	qDebug("%s() : mutex unlocked", __FUNCTION__);
	pthread_cond_signal(_notEmpty);
//	qDebug("%s() : signaled notEmpty - producer produced", __FUNCTION__);
}










int main(int argc, char **argv) {

	if (argc < 5) {
		printf("width height frate port\n");
		exit(0);
	}

	int width = atoi(argv[1]);
	int height = atoi(argv[2]);
	int frate = atoi(argv[3]);
	int port = atoi(argv[4]);

	streamsock = waitForConnection(port);
	if (streamsock == -1) {
		exit(0);
	}

	int bufsize = width * height * 3;
	char *buffer = (char *)malloc(sizeof(char) * bufsize);
	printf("buffer size %d x %d x 3 = %d Byte\n", width, height, bufsize);

    double frameDelay = 1.0f / frate;
    double elapsed = 0;

    struct timeval tvs,tve;
/*
    DoubleBuffer dbuffer(width, height, 3);

    if ( pthread_create(&threadid, 0, send_thread, (void *)&dbuffer) != 0) {
        perror("pthread_create");
        exit(-1);
    }
    sleep(1);

    char *bufptr = (char *)dbuffer.getFrontBuffer();
    */

	while(1) {
        gettimeofday(&tvs, 0);


		//  wait for signal from the receiver
		// this is to synchronize send/recv operations
		// for some reason, in venom,
		recv(streamsock, buffer, 1024, MSG_WAITALL);
        printf("received 1KB signal from the receiver\n");
/*
        memset(bufptr, 0, dbuffer.bufSize());
        dbuffer.swapBuffer();
        bufptr = (char *)dbuffer.getFrontBuffer();
        */

		//
		// recv() must be called BEFORE the sender calls send()
		//
		int sent = sendFrame(streamsock, buffer, bufsize);
		//printf("sent %d byte\n", sent);

        gettimeofday(&tve, 0);

        elapsed = ((double)tve.tv_sec + (double)tve.tv_usec * 1e-6) - ((double)tvs.tv_sec + (double)tvs.tv_usec * 1e-6);
        if ( elapsed > 0 && frameDelay > elapsed ) {
            usleep(1e+6 * (frameDelay-elapsed));
            elapsed = 0;
        }
	}

	printf("Streaming loop finished. Goodbye\n");

	free(buffer);

	close(streamsock);
	close(serversocket);

	return 0;
}

void *send_thread(void *arg) {

    DoubleBuffer *dbuf = static_cast<DoubleBuffer *>(arg);

    int byteSentSofar = 0;
	int actualSent = 0;

    int bufsize = dbuf->bufSize();
    char *bufptr = (char *)dbuf->getBackBuffer();

    printf("thread acquired backbuffer\n");
	while(byteSentSofar < bufsize)
	{
        if (bufsize - byteSentSofar < BUF_LEN) {
            actualSent = send(streamsock, bufptr, bufsize-byteSentSofar, 0);
        }
        else {
            actualSent = send(streamsock, bufptr, BUF_LEN, 0);
        }

		if (actualSent == -1) {
			perror("send");
			break;
		}
		else if (actualSent == 0) {
			perror("send");
			break;
		}

        bufptr += actualSent;
		byteSentSofar += actualSent;
	}

    printf("thread releasing backbuffer\n");
    dbuf->releaseBackBuffer();
}

int sendFrame(int streamsock, char *buffer, int bufsize) {
	int byteSentSofar = 0;
	int actualSent = 0;
    char *bufptr = buffer;

	while(byteSentSofar < bufsize)
	{
        if (bufsize - byteSentSofar < BUF_LEN) {
            actualSent = send(streamsock, bufptr, bufsize-byteSentSofar, 0);
        }
        else {
            actualSent = send(streamsock, bufptr, BUF_LEN, 0);
        }

		if (actualSent == -1) {
			perror("send");
			return -1;
		}
		else if (actualSent == 0) {
			perror("send");
			return 0;
		}

        bufptr += actualSent;
		byteSentSofar += actualSent;
	}
	return byteSentSofar;
}


int waitForConnection(int port) {
	/* accept connection from sageStreamer */
	serversocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if ( serversocket == -1 ) {
		printf("%s() : couldn't create socket", __FUNCTION__);
		return -1;
	}

	// setsockopt
	int optval = 1;
	if ( setsockopt(serversocket, SOL_SOCKET, SO_REUSEADDR, &optval, (socklen_t)sizeof(optval)) != 0 ) {
		printf("%s() : setsockopt SO_REUSEADDR failed",  __FUNCTION__);
	}

	// bind to port
	struct sockaddr_in localAddr, clientAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(port);

	// bind
	if( bind(serversocket, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) != 0) {
		printf("%s() : bind error",  __FUNCTION__);
		return -1;
	}

	// put in listen mode
	listen(serversocket, 15);

	// accept
	/** accept will BLOCK **/
	//	qDebug() << "SN_SageStreamWidget::waitForPixelStreamerConn() : sageappid" << _sageAppId << "Before accept(). TCP port" << protocol+port << QTime::currentTime().toString("hh:mm:ss.zzz");

	memset(&clientAddr, 0, sizeof(clientAddr));
	int addrLen = sizeof(struct sockaddr_in);

	int streamsocket = -1;
	if ((streamsocket = accept(serversocket, (struct sockaddr *)&clientAddr, (socklen_t*)&addrLen)) == -1) {
		printf("%s() : accept error", __FUNCTION__);
		perror("accept");
		return -1;
	}
	else {
		printf("A receiver has connected !\n");
	}

	return streamsocket;
}





