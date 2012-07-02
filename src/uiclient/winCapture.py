import time, pyHook, pythoncom#, win32api
import traceback as tb
from threading import Thread
from multiprocessing import Process
import multiprocessing
import ctypes
import sys
import socket

user32dll = ctypes.windll.user32


# messages to SAGE (dim)
MOVE = 1
CLICK = 2
WHEEL = 3
COLOR = 4
HAS_DOUBLE_CLICK = 10


# button IDs
LEFT=1
RIGHT=2
MIDDLE = 3  


# msgs to the python program
CAPTURED = 11
RELEASED = 12
QUIT = 13

# for double-SHIFT click to capture mouse
DOUBLE_CLICK_SPEED = 0.5  # seconds


# how long does a cursor have to hove at the edge of a screen before its captured/released?
CURSOR_HOVER_TIME = 0.0  

# screen edges to use for pointer capture
LEFT_EDGE   = 1
RIGHT_EDGE  = 2
TOP_EDGE    = 3
BOTTOM_EDGE = 4


class WinCapture:

    def __init__(self, sockfd, w, h, sharedDoRun, sharedCaptureEdge, sharedDisableCorners):
        #Process.__init__(self)
        self.screenWidth = int(w)    # local display resolution
        self.screenHeight = int(h)
		#print "WinCapture init", self.screenWidth, self.screenHeight
        self.SHARED_DO_RUN = sharedDoRun  # if the main program is running
        self.SHARED_CAPTURE_EDGE = sharedCaptureEdge
        self.SHARED_DISABLE_CORNERS = sharedDisableCorners
        self.sock = sockfd

        # timing variables
        self.lastMsgTime = time.time()
        self.firstClickTime = time.time()
        self.firstEdgeTime = -1.0

        # some state variables
        self.captured = False
        self.hm = None
        self.capsHeld = False
        self.tabHeld = False
        self.shiftHeld = False
        #self.shiftCount = 0

    def sendMsg(self, *data):
        msg = ""  
        for d in data:   
            msg += " "+str(d)
 #       self.guiPipe.send(msg.strip())
        #print msg.strip()
        #sys.stdout.write(msg.strip())
        #sys.stdout.flush()
        self.sock.sendall(msg.strip() + "\n")

    def toggleCapture(self):
        if self.captured: 
            self.captured = False
            #win32api.SetCursor(self.noCursor)
            #user32dll.ShowCursor(1)
            #print "RELEASED!!!!"
            self.sendMsg(RELEASED)
        else:
            self.captured = True
            self.capsHeld = self.shiftHeld = self.tabHeld = False
            #user32dll.ShowCursor(0)
            #win32api.SetCursor(self.nocursor)
            #print "=======> CAPTURED!"
            self.sendMsg(CAPTURED)
            


    def onMouseEvent(self, event):
        eventType = event.MessageName
        #print "Mouse event: ", eventType, event.Position
        retVal = True

        now = time.time()

        try:    
            location = event.Position
            if eventType == "mouse move":

                # mouse coordinates in Windows coordinate system...absolute
                mx,my = location[0], location[1]
                
                # for some reason values could be negative so keep it in bounds...
                if mx > self.screenWidth: mx = self.screenWidth
                elif mx < 0: mx = 0
                if my > self.screenHeight: my = self.screenHeight
                elif my < 0: my = 0

                # normalized coord reported to SAGE
                #x = float(mx/float(self.screenWidth))
                #y = 1-float(my/float(self.screenHeight))

		# instead send absolute coord to SAGENextPointer
		x = mx
		y = my
                #print "Pos: %d, %d" % (mx, my)

                # where to warp the cursor to after capture/release
                wx, wy = float(mx), float(my)

                # capture/release the pointer if its on the top of the screen for a while	
                onCaptureEdge = False
                onReleaseEdge = False
                
                # figure out edge-specific cursor coordinates
                off = 70.0

                cornerX = True  # allow corner
                cornerY = True
                if self.SHARED_DISABLE_CORNERS.value == 1:
                    cs = 40   # corner size
                    cornerX =  (mx >= cs and mx <= self.screenWidth-cs)
                    cornerY =  (my >= cs and my <= self.screenHeight-cs)
                    

                if  self.SHARED_CAPTURE_EDGE.value == TOP_EDGE: 
                    if  my < 2 and not self.captured and cornerX:
                        wy = self.screenHeight-off # jump close to bottom
                        onCaptureEdge = True

                    elif  my > self.screenHeight-2 and self.captured:
                        wy = 10.0 # jump to top
                        onReleaseEdge = True


                elif  self.SHARED_CAPTURE_EDGE.value == BOTTOM_EDGE: 
                    if  my < 2 and self.captured:  
                        wy = self.screenHeight-off # jump close to bottom	
                        onReleaseEdge = True

                    elif  my > self.screenHeight-2 and not self.captured and cornerX:  
                        wy = 10.0 # jump to top
                        onCaptureEdge = True


                elif  self.SHARED_CAPTURE_EDGE.value == LEFT_EDGE: 
                    if  mx < 2 and not self.captured and cornerY:  
                        wx = self.screenWidth-off # jump close to bottom
                        onCaptureEdge = True

                    elif  mx > self.screenWidth-2 and self.captured:  
                        wx = 10.0 # jump to top
                        onReleaseEdge = True


                elif  self.SHARED_CAPTURE_EDGE.value == RIGHT_EDGE: 
                    if  mx < 2 and self.captured:   
                        wx = self.screenWidth-off # jump close to bottom
                        onReleaseEdge = True

                    elif  mx > self.screenWidth-2 and not self.captured and cornerY:  
                        wx = 10.0 # jump to top
                        onCaptureEdge = True



                # determine what to do if the pointer is near the edge
                if  onCaptureEdge and not self.captured:   # CAPTURE
                    if  self.firstEdgeTime < 0:   # time was reset
                        self.firstEdgeTime = now

                    elif  now - self.firstEdgeTime > CURSOR_HOVER_TIME: 
                        self.firstEdgeTime = -1.0
                        user32dll.SetCursorPos(int(wx), int(wy))  # warp the cursor to the other edge of the screen

                        # adjust the normalized coord reported to SAGE
                        x = float(wx/self.screenWidth)
                        y = 1 - float(wy/self.screenHeight)

                        self.toggleCapture()
                        retVal = False


                elif  onReleaseEdge and self.captured:   # RELEASE
                    if  self.firstEdgeTime < 0:   # time was reset
                        self.firstEdgeTime = now

                    elif  now - self.firstEdgeTime > CURSOR_HOVER_TIME: 
                        self.firstEdgeTime = -1.0
                        user32dll.SetCursorPos(int(wx), int(wy)) # warp the cursor to the other edge of the screen

                        # adjust the normalized coord reported to SAGE
                        x = float(wx/self.screenWidth)
                        y = 1 - float(wy/self.screenHeight)

                        self.toggleCapture()  
                        retVal = False


                else:  # reset the timer
                    self.firstEdgeTime = -1.0

                #print "\nPos: %.3f, %.3f" % (wx, wy)

                # finally, send the message to SAGE
                if self.captured:
                    if now - self.lastMsgTime >= 1/50.0:
                        self.lastMsgTime = now
                        self.sendMsg(MOVE, x, y)

                    #win32api.SetCursor(self.nocursor)
                    #self.prevCursor = win32api.SetCursor(None)
                    #win32api.SetCursorPos((int(wx), int(wy)))
                    #retVal = False


            elif eventType == "mouse left down":
                if self.captured: 
                    self.sendMsg(CLICK, LEFT, 1)
                    retVal = False

            elif eventType == "mouse left up":
                if self.captured: 
                    self.sendMsg(CLICK, LEFT, 0)
                    retVal = False

            elif eventType == "mouse right down":
                if self.captured: 
                    self.sendMsg(CLICK, RIGHT, 1)
                    retVal = False

            elif eventType == "mouse right up":
                if self.captured: 
                    self.sendMsg(CLICK, RIGHT, 0)
                    retVal = False

            elif eventType == "mouse wheel":
                numSteps = event.Wheel
                if self.captured: 
                    self.sendMsg(WHEEL, numSteps*4)
                    retVal = False

            elif eventType == "mouse middle down":
                if self.captured: 
                    self.sendMsg(CLICK, MIDDLE, 1)
                    retVal = False

            elif eventType == "mouse middle up":
                if self.captured: 
                    self.sendMsg(CLICK, MIDDLE, 0)
                    retVal = False


        except: 
            tb.print_exc()
  
        return retVal



    def onKeyboardEvent(self, event):
        eventType = event.MessageName
        key = event.Key
        retVal = True

        #print "Key event:", key

        try:
            if eventType == "key down":
                if key == "Capital": 
                    self.capsHeld = True
                    if self.captured: retVal = False

                elif key == "Lshift":
                    self.shiftHeld = True
                    if self.captured: retVal = False

                    # allow double shift to capture/release the pointer
                    """
                    if time.time() - self.firstClickTime >= DOUBLE_CLICK_SPEED:
                        self.shiftCount = 1
                        self.firstClickTime = time.time()
                    elif self.shiftCount == 0:
                        self.shiftCount = 1
                    elif self.shiftCount == 1:
                        self.toggleCapture()
                        self.firstClickTime = 0.0
                    """
                elif key == "Tab":
                    self.tabHeld = True
                    if self.captured: retVal = False

                if self.tabHeld or self.shiftHeld and self.capsHeld:
                    self.toggleCapture()
                    retVal = False


            elif eventType == "key up":
                if key == "Capital": 
                    self.capsHeld = False
                    if self.captured: retVal = False

                elif key == "Lshift":
                    self.shiftHeld = False
                    if self.captured: retVal = False

                elif key == "Tab":
                    self.tabHeld = False
                    if self.captured: retVal = False

        except: 
            tb.print_exc()

        return retVal               


    def stopHooks(self):
        if self.hm:
            self.hm.UnhookMouse()
            self.hm.UnhookKeyboard()


    def startHooks(self):
        self.hm = pyHook.HookManager()
        self.hm.SubscribeMouseAll(self.onMouseEvent)
        #self.hm.SubscribeKeyAll(self.onKeyboardEvent)
        self.hm.HookMouse()
        self.hm.HookKeyboard()


    def restart(self):
        self.stopHooks()
        self.startHooks()


    ####   main starting point   #####
    def run(self):
        self.startHooks()
        while self.SHARED_DO_RUN.value == 1:
            pythoncom.PumpWaitingMessages() # wait forever receiving event from Windogs
            time.sleep(0.01)
        self.sendMsg(QUIT)
        self.stopHooks()

#def readFromStdin():
	#while 1:
	    #msg = raw_input()
	    #items = msg.split(' ')
	    #code = int(items[0])
	    #value = int(items[1])


def main():
    global wCapture, SHARED_DO_RUN, SHARED_CAPTURE_EDGE, SCREENSHOT_DIR, SCREENSHOT_NAME, SHARED_DISABLE_CORNERS

    # get display height and width
    width = sys.argv[1]
    height = sys.argv[2]
    shareEdge = int(sys.argv[3])

    # shared variable for closing the winCapture
    SHARED_DO_RUN = multiprocessing.Value("i", 1)
    SHARED_CAPTURE_EDGE = multiprocessing.Value("i", shareEdge)
    SHARED_DISABLE_CORNERS = multiprocessing.Value("i", 1)

	# connect to QTcpServer
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
    try:
        sock.connect( ("127.0.0.1", 44556) )
        print "Connected !"
    except socket.error:
        print "Failed to connect"
        tb.print_exc()

    # start the winCapture in another process... send it the shared DO_RUN 
    # variable and the pipe so it can talk to the GUI
    wCapture = WinCapture(sock, width, height, SHARED_DO_RUN, SHARED_CAPTURE_EDGE, SHARED_DISABLE_CORNERS)
    #wCapture.start()
    wCapture.run()

    socket.close()

    # start the thread that will read from the child process through a pipe
    #t = Thread(target=readFromStdin)
    #t.start()


if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()

