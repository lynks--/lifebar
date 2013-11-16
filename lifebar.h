#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>
#include <sys/socket.h>
#include <sys/types.h>
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

extern int i3_sock;

struct i3_output {
	char name[16];
	char active[16];
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	struct i3_output *next;
};

struct config {
	uint32_t position;
	uint32_t depth;
	int32_t tint;
	char datefmt[64];
	char timefmt[64];
	uint32_t rpadding;
	uint32_t divpadding;
	uint32_t divwidth;
	char ifone[32];
};

int render_divider(XftDraw *, int, int);

int is_key_label(char *);

void handle_value_label(struct i3_output *, char *, char *);

void debug_i3_output(struct i3_output *);

void i3_ipc_send(char **, int, char *);

void free_ipc_result(char *);

struct i3_output *get_i3_outputs();

void get_i3_sockpath(char **) {
