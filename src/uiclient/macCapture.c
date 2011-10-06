//
// SAGEPointer capture code for Mac
// Author: Ratko Jagodic
// 
// Complile using the following command line:
// gcc -Wall -o macCapture macCapture.c -framework ApplicationServices 
//


#include <ApplicationServices/ApplicationServices.h>
#include <sys/time.h>

// globals
bool captured = false;
int screenWidth = 1024;
int screenHeight = 768;
bool ctrl = false;
int shiftCount = 0;
struct timeval tv_start;
double firstClickTime, lastMsgTime, firstEdgeTime, capturedTime;

int flagDiff;
CGEventFlags new, old, temp;
CFMachPortRef      eventTap;
CFRunLoopSourceRef runLoopSource;


// messages to SAGE (dim)
#define MOVE 1
#define CLICK 2
#define WHEEL 3
#define COLOR 4
#define DOUBLE_CLICK 5
#define HAS_DOUBLE_CLICK 10

// msgs to the python program
#define CAPTURED 11
#define RELEASED 12
#define QUIT 13
#define CHANGE_EDGE 14
#define DISABLE_CORNERS 15

// button IDs
#define LEFT 1
#define RIGHT 2
#define MIDDLE 3

// how long does a cursor have to hove at the edge of a screen before its captured/released?
#define CURSOR_HOVER_TIME 0.0  

// screen edges to use for pointer capture
#define LEFT_EDGE   1
#define RIGHT_EDGE  2
#define TOP_EDGE    3
#define BOTTOM_EDGE 4
int captureEdge = TOP_EDGE;
bool disableCorners = false;

// how quickly to double tap in order to capture/release the cursor
#define DOUBLE_CLICK_SPEED  0.3

#define HIDE_CURSOR_DELAY 0.1  //seconds



//-----------------------------------------------------------------------
//          TIME MEASUREMENT
//-----------------------------------------------------------------------

double getTime()  // in seconds... double 
{
    double timeStamp;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    timeStamp = (double)(tv.tv_sec-tv_start.tv_sec)+(double)(tv.tv_usec-tv_start.tv_usec)/1000000;

    return timeStamp;
}



//-----------------------------------------------------------------------
//           Carbon mouse and keyboard event capturing
//-----------------------------------------------------------------------



void sendMsgi(int msgId, int p1){
    printf("%d %d\n", msgId, p1);
    fflush(stdout);
}

void sendMsgii(int msgId, int p1, int p2){
    printf("%d %d %d\n", msgId, p1, p2);
    fflush(stdout);
}

void sendMsgff(int msgId, float p1, float p2){
    printf("%d %f %f\n", msgId, p1, p2);
    fflush(stdout);
}

void toggleCapture()
{
     if (captured) { 
	 captured = false;
	 //fprintf(stderr, "\n\nRELEASED!!!!\n");
	 printf("%d\n", RELEASED);
	 fflush(stdout);
	 CGDisplayShowCursor(kCGNullDirectDisplay);
     } 
     else {
	 //fprintf(stderr, "\n\n===> CAPTURED\n");
	 printf("%d\n", CAPTURED);
	 printf("%d\n", HAS_DOUBLE_CLICK);
	 fflush(stdout);
	 captured = true;
	 capturedTime = getTime();
	 //CGDisplayHideCursor(kCGNullDirectDisplay);
     }
}


// main callback for mouse and keyboard events
CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    CGEventRef retVal = event;
    bool doubleClick = (CGEventGetIntegerValueField(event, kCGMouseEventClickState) == 2);	
    double now = getTime();
    

    // The incoming mouse position.
    CGPoint location = CGEventGetLocation(event);
 
    if (type == kCGEventMouseMoved || type == kCGEventLeftMouseDragged || 
	type == kCGEventRightMouseDragged || type == kCGEventOtherMouseDragged) {
	
        // capture/release the pointer if its on the top of the screen for a while	
	bool onCaptureEdge = false;
	bool onReleaseEdge = false;
	CGPoint warpLocation;
	warpLocation.x = location.x;
	warpLocation.y = location.y;

	// adjust the normalized coord reported to SAGE
	float x = location.x/screenWidth;
	float y = 1 - location.y/screenHeight;

	// figure out edge-specific cursor coordinates
	int off = 70;

	bool cornerX = true; // allow corner
	bool cornerY = true;
	if (disableCorners) {
	    int cs = 40; // corner size
	    cornerX = (location.x >= cs && location.x <= screenWidth-cs);
	    cornerY = (location.y >= cs && location.y <= screenHeight-cs);
	}

	if (captureEdge == TOP_EDGE) {
	    if (location.y < 2 && !captured && cornerX) {
		warpLocation.y = screenHeight-off; // jump close to bottom
		onCaptureEdge = true;
	    }
	    else if (location.y > screenHeight-2 && captured) {
		warpLocation.y = 10.0; // jump to top
		onReleaseEdge = true;
	    }
	}	
	else if (captureEdge == BOTTOM_EDGE) {
	    if (location.y < 2 && captured ) {
		warpLocation.y = screenHeight-off; // jump close to bottom	
		onReleaseEdge = true;
	    }
	    else if (location.y > screenHeight-2 && !captured && cornerX ) {
		warpLocation.y = 10.0; // jump to top
		onCaptureEdge = true;
	    }
	}	
	else if (captureEdge == LEFT_EDGE) {
	    if (location.x < 2 && !captured && cornerY ) {
		warpLocation.x = screenWidth-off; // jump close to bottom
		onCaptureEdge = true;
	    }
	    else if (location.x > screenWidth-2 && captured ) {
		warpLocation.x = 10.0; // jump to top
		onReleaseEdge = true;
	    }
	}	
	else if (captureEdge == RIGHT_EDGE) {
	    if (location.x < 2 && captured )  {
		warpLocation.x = screenWidth-off; // jump close to bottom
		onReleaseEdge = true;
	    }
	    else if (location.x > screenWidth-2 && !captured && cornerY ) {
		warpLocation.x = 10.0; // jump to top
		onCaptureEdge = true;
	    }
	}	


	// determine what to do if the pointer is near the edge
	if (onCaptureEdge && !captured ) {  // CAPTURE
	    if (firstEdgeTime < 0)   // time was reset
		firstEdgeTime = now;

	    else if (now - firstEdgeTime > CURSOR_HOVER_TIME) {
		firstEdgeTime = -1.0;
		CGWarpMouseCursorPosition(warpLocation);  // warp the cursor to the bottom of the screen

		// adjust the normalized coord reported to SAGE
		x = warpLocation.x/screenWidth;
		y = 1 - warpLocation.y/screenHeight;

		toggleCapture();
	    }
	}
	else if (onReleaseEdge && captured) {  // RELEASE
	    if (firstEdgeTime < 0)   // time was reset
		firstEdgeTime = now;

	    else if (now - firstEdgeTime > CURSOR_HOVER_TIME) {
		firstEdgeTime = -1.0;
		CGWarpMouseCursorPosition(warpLocation);  // warp the cursor to the bottom of the screen
		
                // adjust the normalized coord reported to SAGE
		x = warpLocation.x/screenWidth;
		y = 1 - warpLocation.y/screenHeight;
		
		toggleCapture();  
	    }
	}
	else  // reset the timer
	    firstEdgeTime = -1.0;

	//fprintf(stderr, "\nPos: %.3f %.3f", x, y);

	if (captured) {
	    // dont send too frequently...
	    if (now - lastMsgTime >= (1.0/30.0)) { // 30 Hz
			lastMsgTime = now;

		// this sends normalized coordinate for old SAGE
		//sendMsgff(MOVE, x, y);

		// this sends event global position
			sendMsgii(MOVE, location.x, location.y);

	    }
	    retVal = NULL;
	}
    }
    else if(type == kCGEventLeftMouseDown) {
	if (captured) {
	    if(!doubleClick) {
		if (ctrl)
		    sendMsgii(CLICK, RIGHT, 1);
		else
		    sendMsgii(CLICK, LEFT, 1);
	    }
	    retVal = NULL;
	}
    }
    else if(type == kCGEventLeftMouseUp) {
	if (captured) {
	    if (doubleClick)
		sendMsgi(DOUBLE_CLICK, LEFT);
	    else {
		if (ctrl)
		    sendMsgii(CLICK, RIGHT, 0);
		else
		    sendMsgii(CLICK, LEFT, 0);
	    }
	    retVal = NULL;
	}
    }
    else if(type == kCGEventRightMouseDown) {
	if (captured) {
	    sendMsgii(CLICK, RIGHT, 1);
	    retVal = NULL;
	}
    }
    else if(type == kCGEventRightMouseUp) {
	if (captured) {
	    sendMsgii(CLICK, RIGHT, 0);
	    retVal = NULL;
	}
    }
    else if(type == kCGEventOtherMouseDown) {
	if (captured) {
	    sendMsgii(CLICK, MIDDLE, 1);
	    retVal = NULL;
	}
    }
    else if(type == kCGEventOtherMouseUp) {
	if (captured) {
	    sendMsgii(CLICK, MIDDLE, 0);
	    retVal = NULL;
	}
    }
    else if(type == kCGEventScrollWheel) {
	if (captured) {
	    int numSteps = CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1);
	    int n;
	    if (numSteps > 0) 
		n = -1 * (int)ceil((numSteps)/2.0); 
	    else 
		n = -1 * (int)floor((numSteps)/2.0); 
	    sendMsgi(WHEEL, n);
	    retVal = NULL;
	}
    }
    else if(type == kCGEventKeyDown) {
	int keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
	//fprintf(stderr, "\nEvent: %d", keycode);
	if ( (( ((CGEventGetFlags(event) & kCGEventFlagMaskAlphaShift)==kCGEventFlagMaskAlphaShift) || captured) && keycode==48) || (keycode == 113 || keycode == 105 || keycode == 107) ) {
	    if (captured) { 
		captured = false;
		//fprintf(stderr, "\n\nRELEASED!!!!\n");
		printf("%d\n", RELEASED);
		CGDisplayShowCursor(kCGNullDirectDisplay);
		fflush(stdout);
	    } 
	    else {
		//fprintf(stderr, "\n\n===> CAPTURED\n");
		printf("%d\n", CAPTURED);
		printf("%d\n", HAS_DOUBLE_CLICK);
		fflush(stdout);
		captured = true;
                retVal = NULL;
	    }
	}
    }
    
    else if(type == kCGEventFlagsChanged) {
	
	new = CGEventGetFlags(event); 
	flagDiff = (int)old - new;
	old = new;

	// check for CTRL key (for right click on laptops)
	if (abs(flagDiff) == kCGEventFlagMaskControl+1) {
	    if (captured) {
		if (abs(flagDiff) == kCGEventFlagMaskControl+1) {
		    if (flagDiff > 0)
			ctrl = false;
		    else
			ctrl = true;
		}
		retVal = NULL;
	    }
	}

	// check for SHIFT key
	else if (abs(flagDiff+2) == kCGEventFlagMaskShift || abs(flagDiff+4) == kCGEventFlagMaskShift) {
	    if (flagDiff < 0) {
		if ( now - firstClickTime >= DOUBLE_CLICK_SPEED ) {
		    shiftCount = 1;
		    firstClickTime = now;
		}
		else if(shiftCount == 0) {
		    shiftCount = 1;
		}
		else if(shiftCount == 1) {
		    toggleCapture();
		    firstClickTime = 0.0;  // reset the timer
		}
	    }
	    retVal = NULL;
	}
    }

    else if(type == kCGEventTapDisabledByTimeout) {
	//fprintf(stderr, "\n\nEVENT TAP DISABLED!!!!!!!!!\n\n");
	ctrl = false;
	old = CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState);
	CGEventTapEnable(eventTap, true);
	return event;	// NULL also seems to work here...
    }
    
    
    return retVal;
}


void quit()
{
    captured = false;
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CFRunLoopStop(CFRunLoopGetCurrent());
}


void timerCall(CFRunLoopTimerRef timer, void *info)
{
    char * res;
    char incomingMsg[10];  // from python side thru stdin
    int msgCode, msgVal;

    // check for the quit signal on stdin from the python side
    res = fgets(incomingMsg, 1000, stdin);

    if(feof(stdin))
	quit();
    else {  // actual message
	sscanf(incomingMsg, "%d %d\n", &msgCode, &msgVal);

	if (msgCode == QUIT)
	    quit();
	else if(msgCode == CHANGE_EDGE)
	    captureEdge = msgVal;
	else if(msgCode == DISABLE_CORNERS)
	    disableCorners = (bool)msgVal;
    }

    if (capturedTime > 0.0 && getTime()-capturedTime > HIDE_CURSOR_DELAY) {
	capturedTime = -1.0;
	CGDisplayHideCursor(kCGNullDirectDisplay);
    }
}


int main()
{
    CGEventMask        eventMask;
    CFRunLoopTimerRef  timer;
 
    // get the current flag status
    old = CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState);

    // set stdin to non-blocking... instead of using select
    int fd = fileno(stdin);
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);


    // The screen size of the primary display.
    CGRect screenBounds = CGDisplayBounds(CGMainDisplayID());
    screenWidth = (int)screenBounds.size.width;
    screenHeight = (int)screenBounds.size.height;
    
    // timing stuff
    gettimeofday(&tv_start,0);
    lastMsgTime = getTime();
    firstClickTime = getTime();
    firstEdgeTime = -1.0;
    capturedTime = -1.0;
        
    // enable showing/hiding of cursor... a hack really
    void CGSSetConnectionProperty(int, int, CFStringRef, CFBooleanRef);
    int _CGSDefaultConnection();
    CFStringRef propertyString;

    // Hack to make background cursor setting work
    propertyString = CFStringCreateWithCString(NULL, "SetsCursorInBackground", kCFStringEncodingUTF8);
    CGSSetConnectionProperty(_CGSDefaultConnection(), _CGSDefaultConnection(), propertyString, kCFBooleanTrue);
    CFRelease(propertyString);

    // Create an event tap. We are interested in mouse movements.
    eventMask = ((1 << kCGEventMouseMoved) | 
		 (1 << kCGEventLeftMouseDown) | 
		 (1 << kCGEventLeftMouseUp) |
		 (1 << kCGEventLeftMouseDragged) |
		 (1 << kCGEventRightMouseDown) | 
		 (1 << kCGEventRightMouseUp) |
		 (1 << kCGEventRightMouseDragged) |
		 (1 << kCGEventOtherMouseDown) |
		 (1 << kCGEventOtherMouseUp) |
		 (1 << kCGEventOtherMouseDragged) |
		 (1 << kCGEventScrollWheel) |
		 (1 << kCGEventKeyDown) |
		 (1 << kCGEventFlagsChanged));
    
    eventTap = CGEventTapCreate(
                   kCGSessionEventTap, kCGHeadInsertEventTap,
                   0, eventMask, myCGEventCallback, NULL);
    
    if (!eventTap) {
        fprintf(stderr, "failed to create event tap\n");
        exit(1);
    }

    // Create a run loop source.
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    
    // create a timer
    timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), 
				 0.01, //interval in seconds
				 0,
				 0,
				 &timerCall,
				 NULL);
    
    // Add to the current run loop.
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);

    // Enable the event tap.
    CGEventTapEnable(eventTap, true);

    // Set it all running.
    CFRunLoopRun();
    
    return 0;
}



