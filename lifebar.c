#include "lifebar.h"

struct config *conf;

int main(int argc, char **argv) {
	int i;

	// ========= do early setup =========

		//open the display
		Display *d = XOpenDisplay(NULL);
		if(!d) {
			fprintf(stderr, "failed to get x display :(\n");
			return 1;
		}

	// ========= load the configuration =========

		conf = (struct config*)malloc(sizeof(struct config));
		conf->position = BOTTOM;
		conf->depth = 20;
		conf->tint = 60;
		strcpy(conf->datefmt, "%A %e %B %Y");
		strcpy(conf->timefmt, "%H:%M");
		conf->rpadding = 10;
		conf->lpadding = 10;
		conf->divpadding = 10;
		conf->divwidth = 1;
		conf->divstyle = LINE;
		strcpy(conf->ifone, "eth0");
		strcpy(conf->iftwo, "");
		strcpy(conf->fsone, "/");
		strcpy(conf->fstwo, "/home");
		conf->tintcol = prepare_xft_colour(d, 250, 250, 255, 255); //TODO use
		conf->maincol = prepare_xft_colour(d, 20, 20, 20, 255);
		conf->timecol = prepare_xft_colour(d, 20, 20, 20, 255);
		conf->divcol = prepare_xft_colour(d, 50, 50, 70, 255);
		conf->viswscol = prepare_xft_colour(d, 0, 0, 0, 255);
		conf->inviswscol = prepare_xft_colour(d, 60, 60, 60, 255);

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

		//grab the root pixmap for pseudo transparency
		Pixmap bgp = 0;
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
			//this pixmap is our entire background image, across all outputs
			bgp = *((Pixmap *)prop_r);
		}

		//query i3 for the output list
		struct i3_output *output_list = get_i3_outputs(i3_sock);
		struct i3_output *output_temp = NULL;
		struct i3_output *active_head = NULL;
		while(output_list != NULL) {
			if(strcmp(output_list->active, "true") == 0) {
				printf("found output: %s @ %dx%d\n", output_list->name,
					   output_list->width, output_list->height);
				output_temp = active_head;
				active_head = output_list;
				output_list = output_list->next;
				active_head->next = output_temp;
			}
			else {
				output_temp = output_list;
				output_list = output_list->next;
				//inactive outputs are not remembered
				free(output_temp);
			}
		}
		output_list = active_head;

		//we now iterate over each output, creating and preparing a window
		//and environment for each. the result of this process is a linked
		//list of instances pointed to by `instance_list`
		struct instance *instance_list = NULL;
		struct instance *instance_tail = NULL;
		while(output_list != NULL) {
			//allocate this instance
			struct instance *ins = malloc(sizeof *ins);
			ins->next = instance_list;
			ins->output = output_list;
			instance_list = ins;

			//create the window
			ins->w = XCreateSimpleWindow(d, DefaultRootWindow(d),
										 ins->output->x, ins->output->y,
										 ins->output->width, conf->depth,
										 0, 0, 0);

			//set this window type to dock so i3wm knows its a toolbar
			Atom a_win_type = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
			Atom a_dock = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK", False);
			XChangeProperty(d, ins->w, a_win_type, XA_ATOM, 32,
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
					strut_partial.top_start_x = ins->output->x;
					strut_partial.top_end_x = ins->output->x +
						ins->output->width;
					break;
				case BOTTOM:
					strut_partial.bottom = conf->depth;
					strut_partial.bottom_start_x = ins->output->x;
					strut_partial.bottom_end_x = ins->output->x +
						ins->output->width;
					break;
			}
			Atom a_win_strut = XInternAtom(d, "_NET_WM_STRUT_PARTIAL", False);
			Atom a_win_cardinal = XInternAtom(d, "CARDINAL", False);
			XChangeProperty(d, ins->w, a_win_strut, a_win_cardinal, 32,
							PropModeReplace, (unsigned char*)&strut_partial,
							12);

			//grab the section of background image for this instance
			if(bgp != 0) {
				if(conf->position == TOP) 
					ins->bg = XGetImage(d, bgp, ins->output->x, ins->output->y,
										ins->output->width, conf->depth,
										XAllPlanes(), ZPixmap);
				else
					ins->bg = XGetImage(d, bgp, ins->output->x,
										ins->output->y +
											ins->output->height - conf->depth,
										ins->output->width, conf->depth,
										XAllPlanes(), ZPixmap);

				//and apply the tint
				for(i = 0; i < ins->output->width * conf->depth; i++) {
					unsigned int *c = ((unsigned int *)((*ins->bg).data)) + i;

					int tint = conf->tint;
					//if(i < width) tint *= 2; //top line

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
			else ins->bg = NULL;

			//indicate we want to recieve mapnotify and mouse events
			XSelectInput(d, ins->w, StructureNotifyMask | ButtonPressMask);

			//map the window
			XMapWindow(d, ins->w);

			//create a graphics context
			ins->gc = XCreateGC(d, ins->w, 0, NULL);

			//wait for the mapnotify event before proceeding
			while(1) {
				XEvent e;
				XNextEvent(d, &e);
				if(e.type == MapNotify) break;
			}

			//just to avoid calling it so regularly
			Visual *v = DefaultVisual(d, 0);

			//back buffer
			ins->bb = XCreatePixmap(d, ins->w, ins->output->width,
									conf->depth, 24);

			//create the xft context
			ins->xft = XftDrawCreate(d, ins->bb, v, DefaultColormap(d, 0));

			//continue iterating over active outputs
			output_list = output_list->next; 
		}

		XftFont *time_font = XftFontOpen(d, 0,
										 XFT_FAMILY, XftTypeString, "sans",
										 XFT_STYLE, XftTypeString, "bold",
										 XFT_SIZE, XftTypeDouble, 8.0,
										 NULL);
		XftFont *main_font = XftFontOpen(d, 0,
										 XFT_FAMILY, XftTypeString, "sans",
										 XFT_SIZE, XftTypeDouble, 8.0,
										 NULL);

		uint32_t textheight = 0;
			XGlyphInfo gi;
			XftTextExtents8(d, main_font, "E", 1, &gi);
			textheight = conf->depth - (conf->depth - gi.height) / 2;

	// ========= start the main loop =========

		int run = 1;
		while(run) {

			// ========= gather information for this iteration =========

				//lookup interface addresses
				struct ifaddrs *ifap = NULL;
				struct ifaddrs *ifone = NULL;
				if(getifaddrs(&ifap)) perror("getifaddrs");
				else {
					while(1) {
						if((strcmp(ifap->ifa_name, conf->ifone) == 0) &&
						   ((ifap->ifa_addr->sa_family == AF_INET) ||
							(ifap->ifa_addr == NULL))) { //TODO fix for 'down'
							ifone = ifap;
							break;
						}
						else {
							if(ifap->ifa_next == NULL) {
								fprintf(stderr,
										"failed to find interface \"%s\"",
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

				//query i3 for workspace information
				struct i3_workspace *workspaces_list =
					get_i3_workspaces(i3_sock);
				struct i3_workspace *ws_head;

				//check the event buffer for incoming events
				int mouse_clicked = 0;
				int mouse_x = 0;
				int mouse_y = 0;
				while(XPending(d)) {
					XEvent e;
					XNextEvent(d, &e);
					if(e.type == ButtonPress) {
						XButtonEvent *be = (XButtonEvent *)&e;
						mouse_clicked = 1;
						mouse_x = be->x_root;
						mouse_y = be->y_root;
					}
				}

				//lookup hdd capacity information
				struct statvfs fs;
				unsigned long long fsone_cap = 0;
				unsigned long long fsone_free = 0;
				unsigned long long fstwo_cap = 0;
				unsigned long long fstwo_free = 0;
				if(statvfs(conf->fsone, &fs) != -1) {
					fsone_cap = (unsigned long long)fs.f_bsize * fs.f_bfree;
					fsone_free = (unsigned long long)fs.f_bsize * fs.f_bfree;
				}
				if(statvfs(conf->fstwo, &fs) != -1) {
					fstwo_cap = (unsigned long long)fs.f_bsize * fs.f_bfree;
					fstwo_free = (unsigned long long)fs.f_bsize * fs.f_bfree;
				}

			// ========= iterate over each instance, drawing it =========

				struct instance *ins = instance_list;
				while(ins != NULL) {

					// ========= handle instance-specific events =========

						if(mouse_clicked) {
							//check if this instance has been clicked on
							/*
							if(mouse_x > ins->output->x &&
							   mouse_x < (ins->output->x + ins->output->width)
							*/
						}

					// ========= draw the tinted background =========

						if(ins->bg != NULL)
							XPutImage(d, ins->bb, ins->gc, ins->bg, 0, 0, 0, 0,
									  ins->output->width, conf->depth);
						else
							XftDrawRect(ins->xft, conf->tintcol, 0, 0,
										ins->output->width, conf->depth);

					// ========= left side =========

						uint32_t tlpadding = conf->lpadding;

						//workspaces
						ws_head = workspaces_list;
						while(ws_head != NULL) {
							//is this workspace on my output?
							if(strcmp(ws_head->output,
									  ins->output->name) == 0) {
								XftColor *c;
								if(strcmp(ws_head->visible, "true") == 0)
									c = conf->viswscol;
								else
									c = conf->inviswscol;
								XftDrawString8(ins->xft, c, main_font,
											   tlpadding, textheight,
											   (XftChar8 *)ws_head->name,
											   strlen(ws_head->name));
								tlpadding += 10;
								tlpadding += render_divider(ins->xft,
															tlpadding, LEFT);
							}
							ws_head = ws_head->next;
						}

					// ========= right side =========

						uint32_t trpadding = conf->rpadding;

						//time
						time_t rawnow = time(NULL);
						struct tm *now = localtime(&rawnow);
						char time_string[128];
						strftime(time_string, 128, conf->timefmt, now);
						XftTextExtents8(d, time_font, time_string,
										strlen(time_string), &gi);
						XftDrawString8(ins->xft, conf->timecol, time_font,
									   ins->output->width -
									   (gi.width + trpadding), textheight,
									   (XftChar8 *)time_string,
									   strlen(time_string));
						trpadding += gi.width - 1;

						//divider
						trpadding += render_divider(ins->xft,
										ins->output->width - trpadding, RIGHT);

						//date
						char date_string[256];
						strftime(date_string, 256, conf->datefmt, now);
						XftTextExtents8(d, main_font, date_string,
										strlen(date_string), &gi);
						XftDrawString8(ins->xft, conf->maincol, main_font,
									   ins->output->width -
									   (gi.width + trpadding), textheight, 
									   (XftChar8 *)date_string,
									   strlen(date_string));
						trpadding += gi.width;

						//divider
						trpadding += render_divider(ins->xft,
										ins->output->width - trpadding, RIGHT);

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
							
								sprintf(ifone_string, "%s  %s", conf->ifone,
										readable_addr);
							}
							else sprintf(ifone_string, "%s  down",
										conf->ifone);

							if(strlen(ifone_string) > 0) {
								XftTextExtents8(d, main_font, ifone_string,
												strlen(ifone_string), &gi);
								XftDrawString8(ins->xft, conf->maincol,
											   main_font,
											   ins->output->width -
											   (gi.width + trpadding),
											   textheight, 
											   (XftChar8 *)ifone_string,
											   strlen(ifone_string));
								trpadding += gi.width;

								//divider
								trpadding += render_divider(ins->xft,
										ins->output->width - trpadding, RIGHT);
							}
						}

						//fsone
						if(fsone_cap) {
							char str[64];
							sprintf(str, "%s  %.1fGiB", conf->fsone,
									(double)fsone_free / (1024 * 1024 * 1024));
							XftTextExtents8(d, main_font, str,
											strlen(str), &gi);
							XftDrawString8(ins->xft, conf->maincol, main_font,
										   ins->output->width -
										   (gi.width + trpadding), textheight, 
										   (XftChar8 *)str,
										   strlen(str));
							trpadding += gi.width;

							//divider
							trpadding += render_divider(ins->xft,
										ins->output->width - trpadding, RIGHT);
						}

						//fstwo
						if(fstwo_cap) {
							char str[64];
							sprintf(str, "%s  %.1fGiB", conf->fstwo,
									(double)fstwo_free / (1024 * 1024 * 1024));
							XftTextExtents8(d, main_font, str,
											strlen(str), &gi);
							XftDrawString8(ins->xft, conf->maincol, main_font,
										   ins->output->width -
										   (gi.width + trpadding), textheight, 
										   (XftChar8 *)str,
										   strlen(str));
							trpadding += gi.width;

							//divider
							trpadding += render_divider(ins->xft,
										ins->output->width - trpadding, RIGHT);
						}

					// ========= finish this frame =========

						//draw the back buffer and send to the screen
						XCopyArea(d, ins->bb, ins->w, ins->gc, 0, 0,
							ins->output->width, conf->depth, 0, 0);
						XFlush(d);

						//next instance
						ins = ins->next;
				}

				//control fps
				usleep(50000);

		}

}
