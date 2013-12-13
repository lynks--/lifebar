/*
 * Lifebar - i3wm workspace status and tool bar.
 *
 * Copyright (C) 2013 lynks@adranos.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * acpi.c is based heavily on the work of joey@kitenet.net
 *
*/

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <net/if.h>
#include <cairo.h>
#include <cairo-xlib.h>
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

//output message types
#define BAD_MSG "[ \x1b[31m:(\x1b[0m ] "
#define GOOD_MSG "[ \x1b[92m:)\x1b[0m ] "

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
	XImage *bg;						//background
	Pixmap bb;						//backbuffer pixmap
	cairo_surface_t *cairo_s_bb;	//cairo surface for the backbuffer
	cairo_t *cairo;					//cairo context for the backbuffer
	GC gc;							//graphics context
//	XftDraw *xft;
	struct i3_output *output;
	struct instance *next;
};

struct colour {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
};

struct config {
	uint32_t position;				//TOP or BOTTOM
	uint32_t depth;					//bar depth, eg 20
	char datefmt[64];				//date strftime format string
	char timefmt[64];				//time strftime format string
	uint32_t rpadding;				//padding on right screen edge
	uint32_t lpadding;				//padding on left screen edge
	uint32_t divpadding;			//padding either side of divider lines
	uint32_t divwidth;				//divider line width
	uint32_t divstyle;				//divider line style (LINE or GROOVE)
	char ifone[32];					//interface name, eg eth0
	char iftwo[32];					//interface name, eg eth0
	char fsone[32];					//fs location, eg /home
	char fstwo[32];					//fs location, eg /home
	struct colour *tintcol;			//transparency tint colour
	struct colour *keycol;			//main key text colour
	struct colour *valcol;			//main value text colour
	struct colour *timecol;			//time text colour
	struct colour *datecol;			//date text colour
	struct colour *alarmcol;		//alarm text colour
	struct colour *divcol;			//divider line colour for line divider style
	struct colour *viswscol;		//text colour for visible workspace
	struct colour *inviswscol;		//text colour for invisible workspace
	struct colour *groove_light;	//light side of groove overlay
	struct colour *groove_dark;		//dark side of groove overlay
};

extern struct config *conf;

int render_divider(cairo_t *, int, int);

struct colour *prepare_colour(int, int, int, int);

void set_cairo_source_colour(cairo_t *, struct colour*);

int is_key_label(char *);

void handle_value_label(struct i3_output *, char *, char *);

void debug_i3_output(struct i3_output *);

void i3_ipc_send(char **, int, int, char *);

void free_ipc_result(char *);

void free_workspaces_list(struct i3_workspace *);

struct i3_output *get_i3_outputs();

struct i3_workspace *get_i3_workspaces();

void get_i3_sockpath(char **);

struct colour *parse_config_colour(char *);
