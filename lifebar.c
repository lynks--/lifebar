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

struct i3_output {
	char name[16];
	char active[16];
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	struct i3_output *next;
};

int i3_sock;

int is_key_label(char *c) {
	if(strcmp(c, "name") == 0) return 1;
	if(strcmp(c, "active") == 0) return 1;
	if(strcmp(c, "x") == 0) return 1;
	if(strcmp(c, "y") == 0) return 1;
	if(strcmp(c, "width") == 0) return 1;
	if(strcmp(c, "height") == 0) return 1;
	return 0;
}

void handle_value_label(struct i3_output* o, char *key, char *value) {
	printf("handle value label: key=%s, value=%s\n");
	if(strlen(key) > 0) {
		if(strcmp(key, "name") == 0) strcpy(o->name, value);
		else if(strcmp(key, "active") == 0) strcpy(o->active, value);
		else if(strcmp(key, "x") == 0) o->x = atoi(value);
		else if(strcmp(key, "y") == 0) o->y = atoi(value);
		else if(strcmp(key, "width") == 0) o->width = atoi(value);
		else if(strcmp(key, "height") == 0) o->height = atoi(value);
	}
}

void debug_i3_output(struct i3_output *head) {
	while(head != NULL) {
		printf("=========\nname=%s\nactive=%s\nx=%d\ny=%d\nwidth=%d\nheight=%d\n",
				head->name, head->active, head->x, head->y, head->width, head->height);
		head = head->next;
	}
}

void i3_ipc_send(char **ret, int type, char *payload) {
	//we pack the buffer and send it
	int i;
	int plen = strlen(payload);
	char buf[plen + 14];
	strcpy(buf, "i3-ipc");
	*((int *)(buf +  6)) = plen;
	*((int *)(buf + 10)) = type;
	strcpy(buf + 14, payload);
	write(i3_sock, buf, plen + 14);

	//and read the response
	char *recv_buf = malloc(4096);
	ssize_t t = recv(i3_sock, recv_buf, 4096, 0);
	if(t == -1) {
		perror("recv");
		recv_buf[0] = '\0';
	}
	else recv_buf[t] = '\0';
	*ret = recv_buf + 14; //skip over the header
}

void free_ipc_result(char *c) {
	//this function exists because we have to free the actual malloced
	//result, which included the 14 byte header we skipped over
	free(c - 14);
}

struct i3_output *get_i3_outputs() {
	//query i3 for the current outputs and build a linked list
	//note: this function returns a list in reverse order to the json array
	struct i3_output *head = NULL;

	char *output_json;
	i3_ipc_send(&output_json, GET_OUTPUTS, "");

	char key[64];
	char label[64];
	char *labelptr = label;

	int inside = 0;
	int i;
	for(i = 0; i < strlen(output_json); i++) {
		char c = *(output_json + i);
		char d = *(output_json + i + 1);
		if(c == '\"') {
			if(inside == 0) inside = 1;
			else {
				inside = 0;
				*labelptr = '\0';
				if(strcmp(label, "name") == 0) {
					//we are entering a new output block
					//we create a new struct pointing to the current head
					struct i3_output o;
					printf("o=%08x\n", &o);
					o.next = head;
					printf("o.next=%08x\n", o.next);
					head = &o;
					printf("NEW HEAD\n");
					printf("head=%08x, head.next=%08x\n", head, head->next);
					debug_i3_output(head);
					printf("+++++\n");

				}

				if(is_key_label(label)) strcpy(key, label);
				else {
					handle_value_label(head, key, label);
					key[0] = '\0';
				}

				labelptr = label; //reset the label pointer
			}
		}
		else if(c == ':' && d != '\"' && d != '{')
			inside = 2; //inside a non-"-delimited value
		else if((c == ',' || c == '}') && inside == 2) {
			inside = 0;
			*labelptr = '\0';
			handle_value_label(head, key, label);
			key[0] = '\0';
			labelptr = label;
		}
		else {
			if(inside != 0) {
				*labelptr = c;
				labelptr++;
			}
		}
	}
	free_ipc_result(output_json);
	return head;
}

void get_i3_sock(char **ret) {
	int pid, err;
	int pipefd[2];
	FILE *output;
	char buf[512];

	//set up the pipe
	if(pipe(pipefd) < 0) {
		perror("pipe");
		exit(1);
	}

	if((pid = fork()) == 0) {
		//we are in the child

		//duplicate writing end of pipe to child stdout
		dup2(pipefd[1], STDOUT_FILENO);

		//close both of my ends of the pipe?
		close(pipefd[0]);
		close(pipefd[1]);

		execl("/usr/bin/i3", "i3", "--get-socketpath", NULL);
		perror("execl");
	}

	//we are in the parent
	
	//close writing end of pipe
	close(pipefd[1]);

	//open the reading end of the pipe
	output = fdopen(pipefd[0], "r");

	fgets(buf, sizeof buf, output);
	int len = strlen(buf);
	//remove the trailing newline
	buf[len - 1] = '\0';
	*ret = malloc(len);
	strcpy(*ret, buf);
}



int main(int argc, char **argv) {
	int i;

	// ========= load the configuration =========

		struct {
			uint32_t position;
			uint32_t depth;
			uint32_t width;
			uint32_t height;
			int32_t tint;
			char *datefmt;
			char *timefmt;
			uint32_t rpadding;
			uint32_t divpadding;
			uint32_t divwidth;
			char *ifone;
		} config;

		//TODO externalise config
		config.position = BOTTOM;
		config.width = 1920; //TODO read this from the root window PER SCREEN
		config.height = 1080; //TODO ^^
		config.depth = 20;
		config.tint = 40; //-60-60 ish, basic whitewash
		config.datefmt = "%A %e %B %Y";
		config.timefmt = "%H:%M";
		config.rpadding = 10;
		config.divpadding = 10;
		config.divwidth = 1;
		config.ifone = "eth0";

	// ========= open the connection to i3wm via unix domain sockets =========

		//learn the i3wm ipc socket path
		char *i3_sockpath;
		get_i3_sock(&i3_sockpath);
		printf("i3 ipc socket: %s\n", i3_sockpath);

		//use the socket path to create a unix sockaddr struct
		struct sockaddr_un i3_sockaddr;
		i3_sockaddr.sun_family = AF_UNIX;
		strcpy(i3_sockaddr.sun_path, i3_sockpath);
		free(i3_sockpath);

		//create and connect the socket
		i3_sock = socket(AF_UNIX, SOCK_STREAM, 0);
		int addrlen = strlen(i3_sockaddr.sun_path)
						+ sizeof(i3_sockaddr.sun_family);
		if(i3_sock == -1) {
			perror("socket");
			return 1;
		}
		if(connect(i3_sock, (struct sockaddr*)&i3_sockaddr, addrlen) == -1) {
			perror("connect");
			return 1;
		}

	// ========= do all the xlib, gfx and xft setup  =========

		//open the display
		Display *d = XOpenDisplay(NULL);
		if(!d) {
			fprintf(stderr, "failed to get x display :(\n");
			return 1;
		}

		//query i3 for the output list
		struct i3_output *output_list = get_i3_outputs();

		//create the window screen
		Window w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0,
									   config.width, config.depth, 0,
									   0, 0);

		//set this window type to dock so i3wm knows its a toolbar
		Atom a_win_type = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
		Atom a_dock = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK", False);
		XChangeProperty(d, w, a_win_type, XA_ATOM, 32,
						PropModeReplace, (unsigned char*)&a_dock, 1);

		//set this window position to top/bottom from config
		//NOTE: 64 bit arch requires these to be longs, despite the '32'
		//		element size provided in XChangeProperty below, go figure.
		struct {
			long left;
			long right;
			long top;
			long bottom;
			long left_start_y;
			long left_end_y;
			long right_start_y;
			long right_end_y;
			long top_start_x;
			long top_end_x;
			long bottom_start_x;
			long bottom_end_x;
		} strut_partial;
		memset(&strut_partial, 0, sizeof(strut_partial));
		switch(config.position) {
			case TOP:
				strut_partial.top = config.depth;
				strut_partial.top_start_x = 0;
				strut_partial.top_end_x = config.width;
				break;
			case BOTTOM:
				strut_partial.bottom = config.depth;
				strut_partial.bottom_start_x = 0;
				strut_partial.bottom_end_x = config.width;
				break;
		}
		Atom a_win_strut = XInternAtom(d, "_NET_WM_STRUT_PARTIAL", False);
		Atom a_win_cardinal = XInternAtom(d, "CARDINAL", False);
		XChangeProperty(d, w, a_win_strut, a_win_cardinal, 32,
						PropModeReplace, (unsigned char*)&strut_partial, 12);

		//grab the root pixmap for pseudo transparency
		XImage *bg = NULL;
		Atom a_root_pmap = XInternAtom(d, "_XROOTPMAP_ID", False);
		Atom actual_type_r;
		int actual_format_r;
		unsigned long nitems_r;
		unsigned long bytes_after_r;
		unsigned char *prop_r = NULL;
		XGetWindowProperty(d, DefaultRootWindow(d), a_root_pmap, 0, 1, False,
						   XA_PIXMAP, &actual_type_r, &actual_format_r,
						   &nitems_r, &bytes_after_r, &prop_r);
		if(actual_format_r == 0 || prop_r == NULL) {
			fprintf(stderr, "Failed to read root background for ");
			fprintf(stderr, "transparency. Ensure your root window has the ");
			fprintf(stderr, "_XROOTPMAP_ID property.\n");
		}
		else {
			Pixmap bgp = *((Pixmap *)prop_r);
			if(config.position == TOP) 
				bg = XGetImage(d, bgp, 0, 0, config.width, config.depth,
							   XAllPlanes(), ZPixmap);
			else
				bg = XGetImage(d, bgp, 0, config.height - config.depth,
							   config.width, config.depth, XAllPlanes(),
							   ZPixmap);
			//and apply the tint
			for(i = 0; i < config.width * config.depth; i++) {
				unsigned int *c = ((unsigned int *)((*bg).data)) + i;

				int tint = config.tint;
				if(i < config.width) tint *= 2; //top line

				unsigned char *red = ((char *)c) + 2;
				unsigned char *green = ((char *)c) + 1;
				unsigned char *blue = ((char *)c) + 0;

				if(((int)*red + tint) > 255) *red = 255;
				else if(((int)*red + tint) < 0) *red = 0;
				else *red += tint;

				if(((int)*green + tint) > 255) *green = 255;
				else if(((int)*green + tint) < 0) *green = 0;
				else *green += tint;

				if(((int)*blue + tint) > 255) *blue = 255;
				else if(((int)*blue + tint) < 0) *blue = 0;
				else *blue += tint;
			}
		}

		//indicate we want to recieve mapnotify events
		XSelectInput(d, w, StructureNotifyMask);

		//map the window
		XMapWindow(d, w);

		//create a graphics context
		GC gc = XCreateGC(d, w, 0, NULL);

		//wait for the mapnotify event
		while(True) {
			XEvent e;
			XNextEvent(d, &e);
			if(e.type == MapNotify) break;
		}

	// ========= prepare our rendering resources =========

		XRenderColor xrc;

		//background colour
		xrc.red = 200 * 256;
		xrc.green = 200 * 256;
		xrc.blue = 200 * 256;
		xrc.alpha = 255 * 256;
		XftColor bg_colour;
		XftColorAllocValue(d, DefaultVisual(d, 0), DefaultColormap(d, 0),
						   &xrc, &bg_colour);

		//time colour
		xrc.red = 20 * 256;
		xrc.green = 20 * 256;
		xrc.blue = 30 * 256;
		xrc.alpha = 255 * 256;
		XftColor time_colour;
		XftColorAllocValue(d, DefaultVisual(d, 0), DefaultColormap(d, 0),
						   &xrc, &time_colour);

		//date colour
		xrc.red = 20 * 256;
		xrc.green = 20 * 256;
		xrc.blue = 40 * 256;
		xrc.alpha = 255 * 256;
		XftColor date_colour;
		XftColorAllocValue(d, DefaultVisual(d, 0), DefaultColormap(d, 0),
						   &xrc, &date_colour);

		//divider colour
		xrc.red = 50 * 256;
		xrc.green = 50 * 256;
		xrc.blue = 255 * 256;
		xrc.alpha = 255 * 256;
		XftColor divider_colour;
		XftColorAllocValue(d, DefaultVisual(d, 0), DefaultColormap(d, 0),
						   &xrc, &divider_colour);

		//just to avoid calling it so regularly
		Visual *v = DefaultVisual(d, 0);

		//back buffer
		Pixmap bb = XCreatePixmap(d, w, config.width, config.depth, 24);

		//create the xft context
		XftDraw *xft = XftDrawCreate(d, bb, v, DefaultColormap(d,0));
		XftFont *time_font = XftFontOpen(d, 0,
										 XFT_FAMILY, XftTypeString, "sans",
										 XFT_STYLE, XftTypeString, "bold",
										 XFT_SIZE, XftTypeDouble, 8.0,
										 NULL);
		XftFont *date_font = XftFontOpen(d, 0,
										 XFT_FAMILY, XftTypeString, "sans",
										 XFT_SIZE, XftTypeDouble, 8.0,
										 NULL);

	// ========= start the main loop =========

		int run = 1;
		while(run) {
			//lookup interface addresses
			struct ifaddrs *ifap = NULL;
			struct ifaddrs *ifone = NULL;
			if(getifaddrs(&ifap)) perror("getifaddrs");
			else {
				while(1) {
					if((strcmp(ifap->ifa_name, config.ifone) == 0) &&
					   ((ifap->ifa_addr->sa_family == AF_INET) ||
					    (ifap->ifa_addr == NULL))) { //this is not working for "down" interface
						ifone = ifap;
						break;
					}
					else {
						if(ifap->ifa_next == NULL) {
							fprintf(stderr, "failed to find interface \"%s\"",
									config.ifone);
							break;
						}
						else {
							ifap = ifap->ifa_next;
							continue;
						}
					}
				}
			}


			//draw the tinted background
			if(bg != NULL)
				XPutImage(d, bb, gc, bg, 0, 0, 0, 0,
						  config.width, config.depth);
			else
				XftDrawRect(xft, &bg_colour, 0, 0,
							config.width, config.depth);

			uint32_t trpadding = config.rpadding;
			uint32_t textheight = 0;

			//time
			time_t rawnow = time(NULL);
			struct tm *now = localtime(&rawnow);
			char time_string[128];
			strftime(time_string, 128, config.timefmt, now);
			XGlyphInfo gi;
			XftTextExtents8(d, time_font, time_string,
							strlen(time_string), &gi);
			textheight = config.depth - (config.depth - gi.height) / 2;
			XftDrawString8(xft, &time_colour, time_font,
						   config.width - (gi.width + trpadding), textheight,
						   (XftChar8 *)time_string, strlen(time_string));
			trpadding += gi.width;

			//divider
			XftDrawRect(xft, &divider_colour,
						config.width - (trpadding + config.divpadding), 3,
						config.divwidth, 14);
			trpadding += (config.divpadding * 2) + config.divwidth;

			//date
			char date_string[256];
			strftime(date_string, 256, config.datefmt, now);
			XftTextExtents8(d, date_font, date_string,
							strlen(date_string), &gi);
			XftDrawString8(xft, &date_colour, date_font,
						   config.width - (gi.width + trpadding), textheight, 
						   (XftChar8 *)date_string, strlen(date_string));
			trpadding += gi.width;

			//divider
			XftDrawRect(xft, &divider_colour,
						config.width - (trpadding + config.divpadding), 3,
						config.divwidth, 14);
			trpadding += (config.divpadding * 2) + config.divwidth;

			//ifone
			if(ifone != NULL) {
				char ifone_string[256];
				ifone_string[0] = '\0';

				//if this is an ipv4 or ipv6 address
				if((ifone->ifa_addr->sa_family == AF_INET) ||
				   (ifone->ifa_addr->sa_family == AF_INET6)) {
					struct sockaddr_in *addr =
						(struct sockaddr_in *)ifone->ifa_addr;
					char readable_addr[256];
					inet_ntop(addr->sin_family, &(addr->sin_addr),
							  readable_addr, 256);
				
					sprintf(ifone_string, "%s: %s", config.ifone,
							readable_addr);
				}
				else sprintf(ifone_string, "%s: down", config.ifone);

				if(strlen(ifone_string) > 0) {
					XftTextExtents8(d, date_font, ifone_string,
									strlen(ifone_string), &gi);
					XftDrawString8(xft, &date_colour, date_font,
								   config.width - (gi.width + trpadding), textheight, 
								   (XftChar8 *)ifone_string, strlen(ifone_string));
					trpadding += gi.width;
				}
			}


			//draw the back buffer and send to the screen
			XCopyArea(d, bb, w, gc, 0, 0, config.width, config.depth, 0, 0);
			XFlush(d);

			//control fps
			usleep(500000);
		}

}
