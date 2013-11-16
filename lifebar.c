#include "lifebar.h"

struct config *conf;

int main(int argc, char **argv) {
	int i;

	// ========= load the conf->ration =========

		struct config *conf = (struct config*)malloc(sizeof(struct config));
		conf->position = BOTTOM;
		conf->depth = 20;
		conf->tint = 60;
		strcpy(conf->datefmt, "%A %e %B %Y");
		strcpy(conf->timefmt, "%H:%M");
		conf->rpadding = 10;
		conf->divpadding = 10;
		conf->divwidth = 1;
		conf->divstyle = LINE;
		strcpy(conf->ifone, "eth0");
		conf->tintcol = prepare_xft_colour(255, 255, 255, 255); //TODO use this
		conf->maincol = prepare_xft_colour(40, 40, 40, 255);
		conf->timecol = prepare_xft_colour(40, 40, 40, 255);
		conf->divcol = prepare_xft_colour(200, 50, 50, 255);

	// ========= open the connection to i3wm via unix domain sockets =========

		//learn the i3wm ipc socket path
		char *i3_sockpath;
		get_i3_sockpath(&i3_sockpath);
		printf("i3 ipc socket: %s\n", i3_sockpath);

		//use the socket path to create a unix sockaddr struct
		struct sockaddr_un i3_sockaddr;
		i3_sockaddr.sun_family = AF_UNIX;
		strcpy(i3_sockaddr.sun_path, i3_sockpath);
		free(i3_sockpath);

		//create and connect the socket
		int i3_sock = socket(AF_UNIX, SOCK_STREAM, 0);
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
		struct i3_output *output_list = get_i3_outputs(i3_sock);
		//for now, we just iterate until we find the first active output
		//TODO rewrite for multiple outputs
		struct i3_output *active = output_list;
		while(active->next != NULL) {
			if(strcmp(active->active, "true") == 0) {
				printf("found output: %s @ %dx%d\n", active->name,
					   active->width, active->height);
				break;
			}
			active = active->next;
		}
		char *out_name = active->name;
		int width = active->width;
		int height = active->height;

		//create the window screen
		Window w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0,
									   width, conf->depth, 0,
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
		switch(conf->position) {
			case TOP:
				strut_partial.top = conf->depth;
				strut_partial.top_start_x = 0;
				strut_partial.top_end_x = width;
				break;
			case BOTTOM:
				strut_partial.bottom = conf->depth;
				strut_partial.bottom_start_x = 0;
				strut_partial.bottom_end_x = width;
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
			if(conf->position == TOP) 
				bg = XGetImage(d, bgp, 0, 0, width, conf->depth,
							   XAllPlanes(), ZPixmap);
			else
				bg = XGetImage(d, bgp, 0, height - conf->depth,
							   width, conf->depth, XAllPlanes(),
							   ZPixmap);
			//and apply the tint
			for(i = 0; i < width * conf->depth; i++) {
				unsigned int *c = ((unsigned int *)((*bg).data)) + i;

				int tint = conf->tint;
				if(i < width) tint *= 2; //top line

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
		Pixmap bb = XCreatePixmap(d, w, width, conf->depth, 24);

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
					if((strcmp(ifap->ifa_name, conf->ifone) == 0) &&
					   ((ifap->ifa_addr->sa_family == AF_INET) ||
					    (ifap->ifa_addr == NULL))) { //this is not working for "down" interface
						ifone = ifap;
						break;
					}
					else {
						if(ifap->ifa_next == NULL) {
							fprintf(stderr, "failed to find interface \"%s\"",
									conf->ifone);
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
						  width, conf->depth);
			else
				XftDrawRect(xft, &bg_colour, 0, 0,
							width, conf->depth);

			uint32_t trpadding = conf->rpadding;
			uint32_t textheight = 0;

			//time
			time_t rawnow = time(NULL);
			struct tm *now = localtime(&rawnow);
			char time_string[128];
			strftime(time_string, 128, conf->timefmt, now);
			XGlyphInfo gi;
			XftTextExtents8(d, time_font, time_string,
							strlen(time_string), &gi);
			textheight = conf->depth - (conf->depth - gi.height) / 2;
			XftDrawString8(xft, &time_colour, time_font,
						   width - (gi.width + trpadding), textheight,
						   (XftChar8 *)time_string, strlen(time_string));
			trpadding += gi.width;

			//divider
			trpadding += render_divider(xft, width - trpadding, LEFT);

			//date
			char date_string[256];
			strftime(date_string, 256, conf->datefmt, now);
			XftTextExtents8(d, date_font, date_string,
							strlen(date_string), &gi);
			XftDrawString8(xft, &date_colour, date_font,
						   width - (gi.width + trpadding), textheight, 
						   (XftChar8 *)date_string, strlen(date_string));
			trpadding += gi.width;

			//divider
			XftDrawRect(xft, &divider_colour,
						width - (trpadding + conf->divpadding), 3, 1, 14);
			trpadding += (conf->divpadding * 2) + 1;

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
				
					sprintf(ifone_string, "%s: %s", conf->ifone,
							readable_addr);
				}
				else sprintf(ifone_string, "%s: down", conf->ifone);

				if(strlen(ifone_string) > 0) {
					XftTextExtents8(d, date_font, ifone_string,
									strlen(ifone_string), &gi);
					XftDrawString8(xft, &date_colour, date_font,
								   width - (gi.width + trpadding), textheight, 
								   (XftChar8 *)ifone_string,
								   strlen(ifone_string));
					trpadding += gi.width;
				}
			}


			//draw the back buffer and send to the screen
			XCopyArea(d, bb, w, gc, 0, 0, width, conf->depth, 0, 0);
			XFlush(d);

			//control fps
			usleep(500000);
		}

}
