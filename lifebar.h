#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>

//i3 ipc message types
#define COMMAND 0
#define GET_WORKSPACES 1
#define SUBSCRIBE 2
#define GET_OUTPUTS 3
#define GET_TREE 4
#define GET_MASKS 5
#define GET_BAR_CONFIG 6
#define GET_VERSION 7

//config.position options
#define TOP 0
#define BOTTOM 1

//render function lean directions
#define LEFT 0
#define RIGHT 1

//divider line styles
#define LINE 0
#define GROOVE 1

struct i3_output {
	char name[16];
	char active[16];
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	struct i3_output *next;
};

struct i3_workspace {
	char name[64];
	char visible[16];
	char focused[16];
	char urgent[16];
	char output[16];
	struct i3_workspace *next;
};

struct instance {
	Window w;
	XImage *bg;	//background
	Pixmap bb;	//backbuffer
	GC gc;		//graphics context
	XftDraw *xft;
	struct i3_output *output;
	struct instance *next;
};

struct config {
	uint32_t position;		//TOP or BOTTOM
	uint32_t depth;			//bar depth, eg 20
	int32_t tint;			//transparency tint -100 -> 100
	char datefmt[64];		//date strftime format string
	char timefmt[64];		//time strftime format string
	uint32_t rpadding;		//padding on right screen edge
	uint32_t lpadding;		//padding on left screen edge
	uint32_t divpadding;	//padding either side of divider lines
	uint32_t divwidth;		//divider line width
	uint32_t divstyle;		//divider line style (LINE or GROOVE)
	char ifone[32];			//interface name, eg eth0
	char iftwo[32];			//interface name, eg eth0
	char fsone[32];			//fs location, eg /home
	char fstwo[32];			//fs location, eg /home
	XftColor *tintcol;		//transparency tint colour
	XftColor *maincol;		//main text colour
	XftColor *timecol;		//time text colour
	XftColor *divcol;		//divider line colour for line divider style
	XftColor *viswscol;		//text colour for visible workspace
	XftColor *inviswscol;	//text colour for invisible workspace
};

extern struct config *conf;

int render_divider(XftDraw *, int, int);

XftColor *prepare_xft_colour(Display *, int, int, int, int);

int is_key_label(char *);

void handle_value_label(struct i3_output *, char *, char *);

void debug_i3_output(struct i3_output *);

void i3_ipc_send(char **, int, int, char *);

void free_ipc_result(char *);

struct i3_output *get_i3_outputs();

struct i3_workspace *get_i3_workspaces();

void get_i3_sockpath(char **);
