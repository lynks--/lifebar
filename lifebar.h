/*
 * Lifebar - i3wm workspace status and tool bar.
 *
 * Copyright (c) 2013 lynks@adranos.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v3 as published
 * by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <curl/curl.h>

//i3 ipc message types
#define COMMAND 0
#define GET_WORKSPACES 1
#define SUBSCRIBE 2
#define GET_OUTPUTS 3
#define GET_TREE 4
#define GET_MASKS 5
#define GET_BAR_CONFIG 6
#define GET_VERSION 7

//i3 header length
#define I3_HEADER_LENGTH 14

//config.position options
#define TOP 0
#define BOTTOM 1

//render function lean directions
#define LEFT 0
#define RIGHT 1

//divider line styles
#define LINE 0
#define GROOVE 1

//ws wrapping
#define WSWRAP_ON 0
#define WSWRAP_OFF 1

//output message types
#define BAD_MSG "[ \x1b[31m:(\x1b[0m ] "
#define GOOD_MSG "[ \x1b[92m:)\x1b[0m ] "

//battery state
#define CHARGING 0
#define DISCHARGING 1
#define FULL 2
#define EMPTY 3
#define UNKNOWN 4

//max workspaces tracked for click events
#define MAX_WORKSPACES 128
#define MAX_WS_NAME_LENGTH 64

//how often in seconds do we perform the expensive lookups
//NOTE: currently this is used to decrement alarm, so it should stay at 1
#define EXPENSIVE_TIME 1
//ping our external ip every x seconds, 300 = 5 minutes
#define EXTERNAL_IP_TIME 300
//how many readings (1 per second) do we average net speed over
#define NET_SPEED_AVERAGE 3

// the valid configuration options for the config key 'modules' which does
// not have any suffix.
static char **valid_static = (char *[]){"fsone", "fstwo", "extip", "ifone", 
				"iftwo", "date", "time", NULL};

// the valid configuration options for the config key 'modules' which does
// have a suffix like bat0 or therm1.
static char **valid_suffix = (char *[]){"bat", "therm", NULL};

struct batt_info {
	uint32_t index;					//battery number, as in BAT0
	uint32_t percent;				//how full the battery is 0-100 inc
	uint32_t status;				//CHARGING DISCHARGING FULL EMPTY
	uint32_t time;					//how long until FULL/EMPTY in seconds
};

struct thermal_info {
	uint32_t index;					//thermal zone index
	uint32_t temp_c;				//temp in c
};

struct net_speed_info {
	uint64_t down_bytes;			//these are just current totals, we have to
	uint64_t up_bytes;				//compare to previous readings for speed
};

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

struct ws_layout {
	char ws_name[MAX_WORKSPACES][MAX_WS_NAME_LENGTH];
	uint32_t x_max[MAX_WORKSPACES];
	char active[MAX_WORKSPACES];
};

struct time_layout {
	uint32_t x_min;
	uint32_t x_max;
};

struct module {
	char name[64];
	struct module *next;
};

struct instance {
	Window w;
	XImage *bg;						//background
	Pixmap bb;						//backbuffer pixmap
	cairo_surface_t *cairo_s_bb;	//cairo surface for the backbuffer
	cairo_t *cairo;					//cairo context for the backbuffer
	GC gc;							//graphics context
	uint32_t max_wsx;				//max x coord for mouse wheel ws switching
	struct i3_output *output;		//output info for this instance
	struct ws_layout *ws_layout;	//store positions of ws dividers for clicks
	struct time_layout *time_layout;//store position of time for mousewheel
	struct instance *next;
};

struct colour {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
};

struct curl_writedata {
	char *buffer;
	size_t size;
};

struct config {
	uint32_t position;				//TOP or BOTTOM
	uint32_t depth;					//bar depth, eg 20
	char datefmt[64];				//date strftime format string
	char timefmt[64];				//time strftime format string
	uint32_t rpadding;				//padding on right screen edge
	uint32_t lpadding;				//padding on left screen edge
	uint32_t kvpadding;				//padding between key and value
	uint32_t divpadding;			//padding either side of divider lines
	uint32_t divwidth;				//divider line width
	uint32_t divstyle;				//divider line style (LINE or GROOVE)
	uint32_t wswrap;				//whether wsmousewheel wraps WSWRAP_ON/OFF
	char ifone[32];					//interface name, eg eth0
	char iftwo[32];					//interface name, eg eth0
	char fsone[32];					//fs location, eg /home
	char fstwo[32];					//fs location, eg /home
	uint32_t alarm_increment_s;		//alarm increment in seconds eg 300
	struct colour *tintcol;			//transparency tint colour
	struct colour *alarmtintcol;	//background tint for alarm flash
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
	cairo_font_face_t *keyfont;		//main key text font
	double keyfontsize;				//main key text size
	cairo_font_face_t *valfont;		//main value text font
	double valfontsize;				//main value text size
	cairo_font_face_t *datefont;	//date text font
	double datefontsize;			//date text size
	cairo_font_face_t *timefont;	//time text font
	double timefontsize;			//time text size
	cairo_font_face_t *wsfont;		//workspace text font
	double wsfontsize;				//workspace text size
	struct module *modules;			//the modules in appropriate order
	uint32_t batt_alarm;			//batt percentage for alarm
	uint32_t external;				//whether or not to make external ip calls
};

extern struct config *conf;

void check_module_list(struct module *);

int valid_module_static(char *, char **);

int valid_module_suffix(char *, char **);

int get_module_suffix(char *, char **);

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

void parse_config_font(const char *, char *);

int count_acpi_batteries();

void read_acpi_battery(int, struct batt_info *);

int count_acpi_thermal();

void read_acpi_thermal(int, struct thermal_info *);

int render_divider(cairo_t *, int, int);

int render_workspace(cairo_t *, int, int, struct i3_workspace *, int);

int render_time(cairo_t *, int, int, int);

int render_alarm(cairo_t *, uint32_t, int, int, int);

int render_date(cairo_t *, int, int, int);

int render_interface(cairo_t *, int, int, struct ifaddrs *,
			struct net_speed_info[], uint32_t, int);

int render_filesystem(cairo_t *, int, int, struct statvfs *, char *, int);

int render_battery(cairo_t *, int, int, struct batt_info *, int);

int render_thermal(cairo_t *, int, int, struct thermal_info *, int);

int render_keyvalue(cairo_t *, int, int, char *, char *, int);

void read_net_speed(char *, struct net_speed_info *);

size_t curl_writeback(void *, size_t, size_t, void *);
