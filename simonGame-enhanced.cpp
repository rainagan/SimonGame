/* used source code from /cs349-code/x/:
openwindow.cpp, animation.cpp, button.cpp, displaylist.cpp, xeyesballdb.cpp, 
doublebuffer1.cpp, drawing.cpp, eventloop.cpp */
/* This enhanced version is exactly the same as my basic version */
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h> // needed for sleep
#include <sys/time.h>
#include "simon.h"

using namespace std;

// get microseconds
unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

int FPS = 60;
int x_cord;
int y_cord;

// Display* display;
// Window window; 
struct XInfo {
	Display* display;
	int		 screen;
	Window	 window;
	GC       gc[3];

	Pixmap pixmap;
	int width;
	int height;
};

class Displayable {
public:
	virtual void paint(XInfo& xinfo) = 0;

	int x;
	int y;
	bool isIn;
	string s;
};

class Text : public Displayable {
public:
	virtual void paint(XInfo& xinfo) {
		XDrawImageString( xinfo.display, xinfo.window, xinfo.gc[0],
			this->x, this->y, this->s.c_str(), this->s.length() );
	}
	Text(int x, int y, string s): x(x), y(y), s(s)  {}

	int x;
	int y;
  string s; // string to show

};

class Botton : public Displayable {
public:
	virtual void paint(XInfo& xinfo) {
		// GC gc = XCreateGC(xinfo.display, xinfo.window, 0, 0);
		int screen = DefaultScreen( xinfo.display );
		XSetForeground(xinfo.display, xinfo.gc[1], BlackPixel(xinfo.display, screen));
		XSetBackground(xinfo.display, xinfo.gc[1], WhitePixel(xinfo.display, screen));
		XSetFillStyle(xinfo.display,  xinfo.gc[1], FillSolid);
		if (press) {
			XFillArc(xinfo.display, xinfo.window, xinfo.gc[1],
				x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
		} else {
			if (isIn) {
				XSetLineAttributes(xinfo.display, xinfo.gc[1], 4, LineSolid, CapButt, JoinRound);

			} else {
				XSetLineAttributes(xinfo.display, xinfo.gc[1], 1, LineSolid, CapButt, JoinRound);

			}
		}
		XDrawArc(xinfo.display, xinfo.window, xinfo.gc[1],
			x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
		XSetForeground(xinfo.display, xinfo.gc[1], WhitePixel(xinfo.display, screen));
	}
	Botton(int _x, int _y, int _d){
		x = _x;
		y = _y;
		// toggleEvent = _toggleEvent;
		isIn = false;
		d = _d;
		press = false;
	}
	int x;
	int y;
	int d;
	bool isIn;
	bool press;

	void toggle() {
		isIn = !isIn;
	}
};

// pressed button
class PBotton : public Displayable {
public:
	virtual void paint(XInfo& xinfo) {
		int screen = DefaultScreen( xinfo.display );
		XSetForeground(xinfo.display, xinfo.gc[1], BlackPixel(xinfo.display, screen));
		XSetBackground(xinfo.display, xinfo.gc[1], WhitePixel(xinfo.display, screen));
		XSetFillStyle(xinfo.display,  xinfo.gc[1], FillSolid);
		XFillArc(xinfo.display, xinfo.window, xinfo.gc[1],
			x - 50, y - 50, 100, 100, 0, 360 * 64);

		XSetForeground(xinfo.display, xinfo.gc[2], WhitePixel(xinfo.display, screen));
		XSetBackground(xinfo.display, xinfo.gc[2], BlackPixel(xinfo.display, screen));
		XSetLineAttributes(xinfo.display, xinfo.gc[2], 3, LineSolid, CapButt, JoinRound); 
		XDrawArc(xinfo.display, xinfo.window, xinfo.gc[2],
			x - (d / 2), y - (d / 2), d, d, 0, 360 * 64);
		XSetForeground(xinfo.display, xinfo.gc[2], WhitePixel(xinfo.display, screen));
	}
	PBotton(int _x, int _y, int _d){
		x = _x;
		y = _y;
		d = _d;
	}
	int x;
	int y;
	int d;
	bool isIn;
	bool press;
};

vector<Displayable *> dList; 
Text *score = new Text(50, 50, "0");
Text *msg = new Text(50, 100, "Press SPACE to play.");
// button list
vector<Botton *> bList;
// text list
vector <Text *> tList;
PBotton *pb = new PBotton(0,0,100);
// pressed button list
vector <PBotton *> pbList;

void error( string str ) {
	cerr << str << endl;
	exit(0);
}

void initX(int argc, char* argv[], XInfo& xinfo) {
	XSizeHints hints;
	unsigned long white, black;
	XSetWindowAttributes att;

	xinfo.display = XOpenDisplay( "" );
	if ( !xinfo.display )	{
		error( "Can't open display." );
	}

	xinfo.screen = DefaultScreen( xinfo.display );

	white = XWhitePixel( xinfo.display, xinfo.screen );
	black = XBlackPixel( xinfo.display, xinfo.screen );

	hints.x = 100;
	hints.y = 100;
	hints.width = 800;
	hints.height = 400;
	hints.flags = PPosition | PSize;

	xinfo.window = XCreateSimpleWindow( 
		xinfo.display,				// display where window appears
		DefaultRootWindow(xinfo.display), // window's parent in window tree
		hints.x, hints.y,			// upper left corner location
		hints.width, hints.height,	// size of the window
		5,						// width of window's border
		black,						// window border colour
		white );	

	    // extra window properties like a window title
	XSetStandardProperties(
		xinfo.display,		// display containing the window
		xinfo.window,		// window whose properties are set
		"Simon Game",		// window's title
		"Simon Game",			// icon's title
		None,				// pixmap for the icon
		argv, argc,			// applications command line args
		&hints );

	int i=0;
	xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	XSetForeground(xinfo.display, xinfo.gc[i], BlackPixel(xinfo.display, xinfo.screen));
	XSetBackground(xinfo.display, xinfo.gc[i], WhitePixel(xinfo.display, xinfo.screen));
	XFontStruct * font;
	font = XLoadQueryFont (xinfo.display, "12x24");
	XSetFont (xinfo.display, xinfo.gc[i], font->fid);

	i=1;
	xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	XSetForeground(xinfo.display, xinfo.gc[i], BlackPixel(xinfo.display, xinfo.screen));
	XSetBackground(xinfo.display, xinfo.gc[i], WhitePixel(xinfo.display, xinfo.screen));

	i=2;
	xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
	XSetForeground(xinfo.display, xinfo.gc[i], WhitePixel(xinfo.display, xinfo.screen));
	XSetBackground(xinfo.display, xinfo.gc[i], BlackPixel(xinfo.display, xinfo.screen));

	int depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
	xinfo.pixmap = XCreatePixmap(xinfo.display, xinfo.window, hints.width, hints.height, depth);
	xinfo.width = hints.width;
	xinfo.height = hints.height;

	att.backing_store = WhenMapped;
	XChangeWindowAttributes( xinfo.display, xinfo.window, CWBackingStore, &att );

	XSelectInput(xinfo.display, xinfo.window, 
		ButtonPressMask | KeyPressMask | PointerMotionMask 
		| EnterWindowMask | LeaveWindowMask
		| StructureNotifyMask   // for resize events
		);

	XSetWindowBackgroundPixmap(xinfo.display, xinfo.window, None);

	XMapRaised( xinfo.display, xinfo.window );

	XFlush(xinfo.display);
	sleep(1);
}

// repaint only PBotton
void b_repaint( vector<PBotton*> dList, XInfo& xinfo) {
	vector<PBotton*>::const_iterator begin = dList.begin();
	vector<PBotton*>::const_iterator end = dList.end();

	XClearWindow(xinfo.display, xinfo.window);
	// XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], 
	// 	0, 0, xinfo.width, xinfo.height);
	// XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[0], 
	// 	0, 0, xinfo.width, xinfo.height,  // region of pixmap to copy
	// 	0, 0);
	while ( begin != end ) {
		PBotton* d = *begin;
		d->paint(xinfo);
		begin++;
	}
	XFlush(xinfo.display);
}

// repaint dList
void repaint( vector<Displayable*> dList, XInfo& xinfo) {
	vector<Displayable*>::const_iterator begin = dList.begin();
	vector<Displayable*>::const_iterator end = dList.end();

	XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[1], 
		0, 0, xinfo.width, xinfo.height);
	XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[1], 
		0, 0, xinfo.width, xinfo.height,  // region of pixmap to copy
		0, 0);

	while ( begin != end ) {
		Displayable* d = *begin;
		d->paint(xinfo);
		begin++;
	}
	XFlush(xinfo.display);
}

// used for init and resize
void draw_bottons(XInfo &xinfo,int n) {
	int i;
	stringstream ss;
	string s_i;

	dList.push_back(score);
	dList.push_back(msg);
	XClearWindow(xinfo.display, xinfo.window);
	int x = (xinfo.width-n*100)/(n+1);
	for (i=1; i<=n; ++i) {
		bList[i-1]->x = (i-1)*100+x*i+50;
		bList[i-1]->y = xinfo.height/2;
		dList.push_back(bList[i-1]);
		tList[i-1]->x = (i-1)*100+x*i+45;
		tList[i-1]->y = xinfo.height/2+10;
		dList.push_back(tList[i-1]);
	}
}

void handleResize(XInfo &xinfo, XEvent &event, int n) {
	XConfigureEvent xce = event.xconfigure;
	if (xce.width != xinfo.width || xce.height != xinfo.height) {
		XFreePixmap(xinfo.display, xinfo.pixmap);
		int depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
		xinfo.pixmap = XCreatePixmap(xinfo.display, xinfo.window, xce.width, xce.height, depth);
		xinfo.width = xce.width;
		xinfo.height = xce.height;
		draw_bottons(xinfo, n);
	}
}

// mouse cursor enter button
void handleEnterMotion(XInfo &xinfo, XEvent &event, int n) {
	if (sqrt(pow(event.xbutton.x-bList[0]->x,2)+pow(event.xbutton.y-bList[0]->y,2))<=50) {
		bList[0]->isIn = true;
		dList.push_back(bList[0]);
	} else if (n>1 && (sqrt(pow(event.xbutton.x-bList[1]->x,2)+pow(event.xbutton.y-bList[1]->y,2))<=50)) {
		bList[1]->isIn = true;
		dList.push_back(bList[1]);
	} else if (n>2 && (sqrt(pow(event.xbutton.x-bList[2]->x,2)+pow(event.xbutton.y-bList[2]->y,2))<=50)) {
		bList[2]->isIn = true;
		dList.push_back(bList[2]);
	} else if (n>3 && (sqrt(pow(event.xbutton.x-bList[3]->x,2)+pow(event.xbutton.y-bList[3]->y,2))<=50)) {
		bList[3]->isIn = true;
		dList.push_back(bList[3]);
	} else if (n>4 && (sqrt(pow(event.xbutton.x-bList[4]->x,2)+pow(event.xbutton.y-bList[4]->y,2))<=50)) {
		bList[4]->isIn = true;
		dList.push_back(bList[4]);
	} else if (n>5 && (sqrt(pow(event.xbutton.x-bList[5]->x,2)+pow(event.xbutton.y-bList[5]->y,2))<=50)) {
		bList[5]->isIn = true;
		dList.push_back(bList[5]);
	}
}

// mouse cursor leave button
void handleLeaveMotion(XInfo &xinfo, XEvent &event, int n) {
	if (sqrt(pow(event.xbutton.x-bList[0]->x,2)+pow(event.xbutton.y-bList[0]->y,2))>50) {
		bList[0]->isIn = false;
		bList[0]->press = false;
	} 
	if (n>1 && (sqrt(pow(event.xbutton.x-bList[1]->x,2)+pow(event.xbutton.y-bList[1]->y,2))>50)) {
		bList[1]->isIn = false;
		bList[1]->press = false;
	} 
	if (n>2 && (sqrt(pow(event.xbutton.x-bList[2]->x,2)+pow(event.xbutton.y-bList[2]->y,2))>50)) {
		bList[2]->isIn = false;
		bList[2]->press = false;
	} 
	if (n>3 && (sqrt(pow(event.xbutton.x-bList[3]->x,2)+pow(event.xbutton.y-bList[3]->y,2))>50)) {
		bList[3]->isIn = false;
		bList[3]->press = false;
	} 
	if (n>4 && (sqrt(pow(event.xbutton.x-bList[4]->x,2)+pow(event.xbutton.y-bList[4]->y,2))>50)) {
		bList[4]->isIn = false;
		bList[4]->press = false;
	} 
	if (n>5 && (sqrt(pow(event.xbutton.x-bList[5]->x,2)+pow(event.xbutton.y-bList[5]->y,2))>50)) {
		bList[5]->isIn = false;
		bList[5]->press = false;
	}
}

// press button
void handlePress(XInfo &xinfo, XEvent &event, int n) {
	int d = 90,i;
	if (sqrt(pow(event.xbutton.x-bList[0]->x,2)+pow(event.xbutton.y-bList[0]->y,2))<=50) {
		pb->x = bList[0]->x;
		pb->y = bList[0]->y;
		pbList.push_back(pb);
		b_repaint(pbList,xinfo);
	} else if (n>1 && (sqrt(pow(event.xbutton.x-bList[1]->x,2)+pow(event.xbutton.y-bList[1]->y,2))<=50)) {
		pb->x = bList[1]->x;
		pb->y = bList[1]->y;
		pbList.push_back(pb);
		b_repaint(pbList,xinfo);
	} else if (n>2 && (sqrt(pow(event.xbutton.x-bList[2]->x,2)+pow(event.xbutton.y-bList[2]->y,2))<=50)) {
		pb->x = bList[2]->x;
		pb->y = bList[2]->y;
		pbList.push_back(pb);
		b_repaint(pbList,xinfo);
	} else if (n>3 && (sqrt(pow(event.xbutton.x-bList[3]->x,2)+pow(event.xbutton.y-bList[3]->y,2))<=50)) {
		pb->x = bList[3]->x;
		pb->y = bList[3]->y;
		pbList.push_back(pb);
		b_repaint(pbList,xinfo);
	} else if (n>4 && (sqrt(pow(event.xbutton.x-bList[4]->x,2)+pow(event.xbutton.y-bList[4]->y,2))<=50)) {
		pb->x = bList[3]->x;
		pb->y = bList[4]->y;
	} else if (n>5 && (sqrt(pow(event.xbutton.x-bList[5]->x,2)+pow(event.xbutton.y-bList[5]->y,2))<=50)) {
		pb->x = bList[5]->x;
		pb->y = bList[5]->y;
		pbList.push_back(pb);
		b_repaint(pbList,xinfo);
	}
}

void eventLoop(XInfo &xinfo, int n) {
	XEvent event;
	unsigned long lastRepaint = 0;
	unsigned long end;
	KeySym key;
	char text[10];
	int i, new_score;
	vector<int> n_button;
	vector<int> e_button;
	stringstream ss;
	string s_score;


	Simon simon = Simon(n, true);
	cout << "Playing with " << simon.getNumButtons() << " buttons." << endl;

	while (true) {
		if (XPending(xinfo.display) > 0) {
			XNextEvent(xinfo.display, &event);
			switch (event.type) {
				case ButtonPress:
				if (sqrt(pow(event.xbutton.x-bList[0]->x,2)+pow(event.xbutton.y-bList[0]->y,2))<=50) {
					bList[0]->press = true;
					if (simon.getState() == Simon::HUMAN) e_button.push_back(0);
				} else if (n>1 && (sqrt(pow(event.xbutton.x-bList[1]->x,2)+pow(event.xbutton.y-bList[1]->y,2))<=50)) {
					bList[1]->press = true;
					if (simon.getState() == Simon::HUMAN) e_button.push_back(1);
				} else if (n>2 && (sqrt(pow(event.xbutton.x-bList[2]->x,2)+pow(event.xbutton.y-bList[2]->y,2))<=50)) {
					bList[2]->press = true;
					if (simon.getState() == Simon::HUMAN) e_button.push_back(2);
				} else if (n>3 && (sqrt(pow(event.xbutton.x-bList[3]->x,2)+pow(event.xbutton.y-bList[3]->y,2))<=50)) {
					bList[3]->press = true;
					if (simon.getState() == Simon::HUMAN) e_button.push_back(3);
				} else if (n>4 && (sqrt(pow(event.xbutton.x-bList[4]->x,2)+pow(event.xbutton.y-bList[4]->y,2))<=50)) {
					bList[4]->press = true;
					if (simon.getState() == Simon::HUMAN) e_button.push_back(4);
				} else if (n>5 && (sqrt(pow(event.xbutton.x-bList[5]->x,2)+pow(event.xbutton.y-bList[5]->y,2))<=50)) {
					bList[5]->press = true;
					if (simon.getState() == Simon::HUMAN) e_button.push_back(5);
				}
				break;
				case KeyPress: 
				i = XLookupString((XKeyEvent *)&event,text,
				10,&key,NULL);					// pointer to a composeStatus structure (unused)
				if (i == 1) {
					// printf("Got key press -- %c\n", text[0]);
					if (text[0] == 'q') {
						error("Terminating normally.");
					} else if (text[0] == ' ') {
						msg->s = "Watch what I do ...";
						dList.push_back(msg);
						repaint(dList,xinfo);
						simon.newRound();
						while (simon.getState() == Simon::COMPUTER) {
							int temp = simon.nextButton();
							n_button.push_back(temp);
							pb->x = bList[temp]->x;
							pb->y = bList[temp]->y;
							pbList.push_back(pb);
							b_repaint(pbList,xinfo);
							sleep(1);
						}
						msg->s = "Now it's your turn";
					}
				}
				break;
				case ConfigureNotify:
				handleResize(xinfo, event, n);
				break;
				case MotionNotify:
				handleEnterMotion(xinfo, event, n);
				handleLeaveMotion(xinfo,event,n);
				break;
			}
		}
		end = now();
		if (end - lastRepaint > 1000000 / FPS) { 
			XClearWindow(xinfo.display, xinfo.window);
			if (e_button.size() == n_button.size() && simon.getState()!= Simon::START) {
				for (int j=0; j<e_button.size(); ++j) {
					if (!simon.verifyButton(e_button[j])) {
						msg->s = "You lose! Press SPACE to play again";
						score->s = "1";
						e_button.clear();
						n_button.clear();
						simon.newRound();
					}
				}
				if (simon.getState()== Simon::WIN) {
					msg->s = "You won! Press SPACE to continue";
					new_score = simon.getScore();
					ss << new_score;
					s_score = ss.str();
					ss.str(string());
					score->s = s_score;
					e_button.clear();
					n_button.clear();
				}
			}
			repaint(dList,xinfo);
			lastRepaint = now();
		}
		if (XPending(xinfo.display) == 0) {
			usleep(1000000 / FPS - (end - lastRepaint));
		}
	} 
}

int main ( int argc, char* argv[] ) {
	int n = 4, i, x;
	if (argc > 1) {
		n = atoi(argv[1]);
	}
	n = max(1, min(n, 6));

	XInfo xinfo;
	initX(argc, argv, xinfo);

	stringstream ss;
	string s_score, s_i;
	ss.str(string());

	vector<Displayable *> dList; 
	dList.push_back(score);
	dList.push_back(msg);

	x = (800-n*100)/(n+1);
	for (i=1; i<=n; ++i) {
		bList.push_back(new Botton((i-1)*100+x*i+50,400/2,100));
		ss << i;
		s_i = ss.str();
		ss.str(string());
		tList.push_back(new Text((i-1)*100+x*i+45,400/2+10,s_i));
	}
	draw_bottons(xinfo,n);
	repaint(dList,xinfo);

	eventLoop(xinfo,n);
}