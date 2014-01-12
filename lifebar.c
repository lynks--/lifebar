#include "lifebar.h"

struct config *conf;

int main(int argc, char *argv[]) {
	int i;

	// ========= do early setup =========

		//open the display
		Display *d = XOpenDisplay(NULL);
		if(!d) {
			fprintf(stderr, "%sfailed to get x display :(\n", BAD_MSG);
			return 1;
		}

	// ========= load the configuration =========

		conf = (struct config*)malloc(sizeof(struct config));

		//first we set the defaults to avoid unset values
		conf->position = BOTTOM;
		conf->depth = 20;
		strcpy(conf->datefmt, "%A %e %B %Y");
		strcpy(conf->timefmt, "%H:%M");
		conf->rpadding = 10;
		conf->lpadding = 10;
		conf->divpadding = 10;
		conf->divwidth = 1;
		conf->divstyle = GROOVE;
		strcpy(conf->ifone, "eth0");
		strcpy(conf->iftwo, "");
		strcpy(conf->fsone, "/home");
		strcpy(conf->fstwo, "");
		conf->alarm_increment_s = 300;
		conf->tintcol = prepare_colour(255, 255, 255, 60);
		conf->alarmtintcol = prepare_colour(255, 50, 50, 180);
		conf->keycol = prepare_colour(20, 20, 20, 255);
		conf->valcol = prepare_colour(50, 50, 50, 255);
		conf->timecol = prepare_colour(20, 20, 20, 255);
		conf->datecol = prepare_colour(20, 20, 20, 255);
		conf->alarmcol = prepare_colour(200, 20, 20, 255);
		conf->divcol = prepare_colour(50, 50, 70, 255);
		conf->viswscol = prepare_colour(0, 0, 0, 255);
		conf->inviswscol = prepare_colour(0, 0, 0, 100);
		conf->groove_dark = prepare_colour(10, 10, 10, 40);
		conf->groove_light = prepare_colour(250, 250, 250, 40);

		//now overwrite the defaults with any configured values
		char *confpath = malloc(1024);
		sprintf(confpath, "%s/.lifebarrc", getenv("HOME"));
		FILE *cf = fopen(confpath, "r");
		if(cf != NULL) {
			printf("%susing config file: '~/.lifebarrc'\n", GOOD_MSG);
			char line[1024];
			char key[128];
			char value[1024];
			unsigned int vstart = 0;
			while(fgets(line, 1024, cf) != NULL) {
				if(line[0] != '#' && line[0] != '\n') {
					int success = sscanf(line, " %s %n\" %[^\"]\"\n",
										 key, &vstart, value);
					if(success == 1) {
						//key was read ok, value was not quoted so we reread
						success = sscanf(line + vstart, "%s \n", value);
						if(success == 0) {
							fprintf(stderr,
									"%serror parsing config line: %s\n",
									BAD_MSG, line);
							continue;
						}
						else success = 2;
					}

					if(success == 2) {
						if(strcmp(key, "position") == 0) {
							if(strcmp(value, "top") == 0)
								conf->position = TOP;
							else if(strcmp(value, "bottom") == 0)
								conf->position = BOTTOM;
							else
								fprintf(stderr,
								  "%sbad value for config key 'position':%s\n",
								  BAD_MSG, value);
						}
						else if(strcmp(key, "depth") == 0) {
							int x = atoi(value);
							if(x > 0) conf->depth = x;
							else fprintf(stderr,
									"%sbad value for config key 'depth':%s\n",
									BAD_MSG, value);
						}
						else if(strcmp(key, "datefmt") == 0)
							strcpy(conf->datefmt, value);
						else if(strcmp(key, "timefmt") == 0)
							strcpy(conf->timefmt, value);
						else if(strcmp(key, "rpadding") == 0) {
							int x = atoi(value);
							if(x > 0) conf->rpadding = x;
							else fprintf(stderr,
								  "%sbad value for config key 'rpadding':%s\n",
								  BAD_MSG, value);
						}
						else if(strcmp(key, "lpadding") == 0) {
							int x = atoi(value);
							if(x > 0) conf->lpadding = x;
							else fprintf(stderr,
								  "%sbad value for config key 'lpadding':%s\n",
								  BAD_MSG, value);
						}
						else if(strcmp(key, "divpadding") == 0) {
							int x = atoi(value);
							if(x > 0) conf->divpadding = x;
							else fprintf(stderr,
								"%sbad value for config key 'divpadding':%s\n",
								BAD_MSG, value);
						}
						else if(strcmp(key, "divstyle") == 0) {
							if(strcmp(value, "line") == 0)
								conf->divstyle = LINE;
							else if(strcmp(value, "groove") == 0)
								conf->divstyle = GROOVE;
							else
								fprintf(stderr,
								  "%sbad value for config key 'divstyle':%s\n",
								  BAD_MSG, value);
						}
						else if(strcmp(key, "ifone") == 0)
							strcpy(conf->ifone, value);
						else if(strcmp(key, "iftwo") == 0)
							strcpy(conf->iftwo, value);
						else if(strcmp(key, "fsone") == 0)
							strcpy(conf->fsone, value);
						else if(strcmp(key, "fstwo") == 0)
							strcpy(conf->fstwo, value);
						else if(strcmp(key, "alarminc") == 0) {
							int x = atoi(value);
							if(x > 0) conf->alarm_increment_s = x;
							else fprintf(stderr,
								"%sbad value for config key 'alarminc':%s\n",
								BAD_MSG, value);
						}
						else if(strcmp(key, "tintcol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->tintcol = col;
						}
						else if(strcmp(key, "alarmtintcol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->alarmtintcol = col;
						}
						else if(strcmp(key, "keycol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->keycol = col;
						}
						else if(strcmp(key, "valcol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->valcol = col;
						}
						else if(strcmp(key, "timecol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->timecol = col;
						}
						else if(strcmp(key, "datecol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->datecol = col;
						}
						else if(strcmp(key, "alarmcol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->alarmcol = col;
						}
						else if(strcmp(key, "divcol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->divcol = col;
						}
						else if(strcmp(key, "viswscol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->viswscol = col;
						}
						else if(strcmp(key, "inviswscol") == 0) {
							struct colour *col = parse_config_colour(value);
							if(col != NULL) conf->inviswscol = col;
						}
					}
					else {
						fprintf(stderr, "%serror parsing config line: %s\n",
								BAD_MSG, line);
						continue;
					}
				}
			} //end config line iteration
		}
		else {
			//in the absence of a config file we use defaults
			fprintf(stderr,
					"%sno config file '~/.lifebarrc' found, using defaults\n",
				   BAD_MSG);
		}

	// ========= open the connection to i3wm via unix domain sockets =========

		//learn the i3wm ipc socket path
		char *i3_sockpath;
		get_i3_sockpath(&i3_sockpath);
		printf("%susing i3 ipc socket: %s\n", GOOD_MSG, i3_sockpath);

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
			fprintf(stderr, "%sFailed to read root background for ", BAD_MSG);
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
				printf("%sfound output: %s @ %dx%d\n", GOOD_MSG,
					   output_list->name, output_list->width,
					   output_list->height);
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
			ins->ws_layout = malloc(sizeof *ins->ws_layout);
			for(i = 0; i < MAX_WORKSPACES; i++)
				ins->ws_layout->ws_name[i][0] = '\0';
			ins->time_layout = malloc(sizeof *ins->time_layout);
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

			if(bgp != 0) {
				//grab the section of background image for this instance
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

				//we apply the tint manually
				for(i = 0; i < ins->output->width * conf->depth; i++) {
					//grab the background colour for this pixel
					unsigned int *c = ((unsigned int *)((*ins->bg).data)) + i;
					unsigned char *red = ((char *)c) + 2;
					unsigned char *green = ((char *)c) + 1;
					unsigned char *blue = ((char *)c) + 0;

					//calculate the difference from background to tint
					int red_d =
						(int)((conf->tintcol->red - (int)*red) /
						(255 / (double)conf->tintcol->alpha));
					int green_d =
						(int)((conf->tintcol->green - (int)*green) /
						(255 / (double)conf->tintcol->alpha));
					int blue_d =
						(int)((conf->tintcol->blue - (int)*blue) /
						(255 / (double)conf->tintcol->alpha));

					//and update the pixel
					if(((int)*red + red_d) > 255) *red = 255;
					else if(((int)*red + red_d) < 0) *red = 0;
					else *red += red_d;

					if(((int)*green + green_d) > 255) *green = 255;
					else if(((int)*green + green_d) < 0) *green = 0;
					else *green += green_d;

					if(((int)*blue + blue_d) > 255) *blue = 255;
					else if(((int)*blue + blue_d) < 0) *blue = 0;
					else *blue += blue_d;
				}
			}
			else ins->bg = NULL;

			//indicate we want to recieve mapnotify and mouse events
			XSelectInput(d, ins->w, StructureNotifyMask | ButtonPressMask);

			//map the window
			XMapWindow(d, ins->w);

			//create a graphics context used for xlib drawing tasks
			ins->gc = XCreateGC(d, ins->w, 0, NULL);

			//wait for the mapnotify event before proceeding
			while(1) {
				XEvent e;
				XNextEvent(d, &e);
				if(e.type == MapNotify) break;
			}

			//create the back buffer pixmap
			ins->bb = XCreatePixmap(d, ins->w, ins->output->width,
									conf->depth, 24);

			//create a cairo surface for the back buffer
			ins->cairo_s_bb = cairo_xlib_surface_create(d, ins->bb,
									XDefaultVisual(d, 0), ins->output->width,
									conf->depth);

			//and a cairo context from the surface
			ins->cairo = cairo_create(ins->cairo_s_bb);

			//continue iterating over active outputs
			output_list = output_list->next; 
		}

		//prepare the fonts
		//TODO make fonts configurable

		//measure the text y-coord
		uint32_t textheight = 0;
		cairo_text_extents_t extents;
		cairo_text_extents(instance_list->cairo, "ABCDEFG", &extents);
		textheight = conf->depth - ((conf->depth - extents.height) / 2);

		//detect acpi power sources
		int batt_count = count_acpi_batteries();
		struct batt_info *batteries[batt_count];
		for(i = 0; i < batt_count; i++)
			batteries[i] = malloc(sizeof *batteries[i]);

		//detect acpi thermal sources
		int thermal_count = count_acpi_thermal();
		struct thermal_info *thermals[thermal_count];
		for(i = 0; i < thermal_count; i++)
			thermals[i] = malloc(sizeof *thermals[i]);

	// ========= start the main loop =========

		int run = 1;
		int ifone_warned = 0; //warn once
		int iftwo_warned = 0;
		uint64_t start = time(NULL);
		uint64_t last_expensive = 0;	//last time expensive lookups were done
		uint64_t alarm_activate = 0;
		struct statvfs fsone;
		struct statvfs fstwo;
		struct i3_workspace *workspaces_list = NULL;
		struct ifaddrs *ifah = NULL;
		struct ifaddrs *ifap = NULL;
		struct ifaddrs *ifone = NULL;
		struct ifaddrs *iftwo = NULL;
		struct net_speed_info *ifone_speed = NULL;
		struct net_speed_info *iftwo_speed = NULL;
		struct net_speed_info *ifone_prev_speed = NULL;
		struct net_speed_info *iftwo_prev_speed = NULL;
		int fsone_alive = 0;
		int fstwo_alive = 0;
		char wanip[128];
		uint32_t alarm_s = 0; //time to alarm, in seconds
		while(run) {
			uint64_t frame_time = time(NULL);

			// ========= gather information for this iteration =========

				//we only perform expensive lookups once every EXPENSIVE_TIME
				if(last_expensive + EXPENSIVE_TIME <= frame_time) {
					last_expensive = frame_time;

					//decrement the alarm
					if(alarm_s > EXPENSIVE_TIME) 
						alarm_s -= EXPENSIVE_TIME;
					else if(alarm_s > 0) {
						//alarm timer has finised
						alarm_activate = frame_time;
						alarm_s = 0;
					}

					//lookup interface addresses
					freeifaddrs(ifah);
					if(getifaddrs(&ifah)) perror("getifaddrs");
					ifap = ifah;
					while(1) {
						if(strcmp(ifap->ifa_name, conf->ifone) == 0) {
							if(ifone == NULL)
								ifone = ifap; //first encounter
							else if(ifap->ifa_addr->sa_family == AF_INET)
								ifone = ifap; //ipv4 overwrites any
							else if(ifap->ifa_addr->sa_family == AF_INET6 &&
										ifone->ifa_addr->sa_family != AF_INET)
								ifone = ifap; //ipv6 overwrites any but ipv4
						}
						else if(strcmp(ifap->ifa_name, conf->iftwo) == 0) {
							if(iftwo == NULL)
								iftwo = ifap; //first encounter
							else if(ifap->ifa_addr->sa_family == AF_INET)
								iftwo = ifap; //ipv4 overwrites any
							else if(ifap->ifa_addr->sa_family == AF_INET6 &&
										iftwo->ifa_addr->sa_family != AF_INET)
								iftwo = ifap; //ipv6 overwrites any but ipv4
						}

						if(ifap->ifa_next == NULL) {
							if(ifone == NULL && !ifone_warned &&
											strlen(conf->ifone) > 0) {
								fprintf(stderr,
										"%sfailed to find interface '%s'\n",
										BAD_MSG, conf->ifone);
								ifone_warned = 1;
							}
							if(iftwo == NULL && !iftwo_warned &&
											strlen(conf->iftwo) > 0) {
								fprintf(stderr,
										"%sfailed to find interface '%s'\n",
										BAD_MSG, conf->iftwo);
								iftwo_warned = 1;
							}
							break;
						}

						ifap = ifap->ifa_next;
					}

					//read interface speed
					if(ifone != NULL) {
						//NOTE: rather than malloc and free, we should just
						//iterate between two pointers here

						free(ifone_prev_speed);
						ifone_prev_speed = ifone_speed;

						ifone_speed = malloc(sizeof *ifone_speed);
						read_net_speed(conf->ifone, ifone_speed);
					}
					if(iftwo != NULL) {
						free(iftwo_prev_speed);
						iftwo_prev_speed = iftwo_speed;

						iftwo_speed = malloc(sizeof *iftwo_speed);
						read_net_speed(conf->iftwo, iftwo_speed);
					}

					//lookup hdd capacity information
					fsone_alive = statvfs(conf->fsone, &fsone);
					fstwo_alive = statvfs(conf->fstwo, &fstwo);

					//read battery information
					for(i = 0; i < batt_count; i++)
						read_acpi_battery(i, batteries[i]);

					//read thermal information
					for(i = 0; i < thermal_count; i++)
						read_acpi_thermal(i, thermals[i]);

					//lookup external/wan ip
					//TODO curl this
					/*
					struct sockaddr_in sock;
					int sock_h = socket(AF_INET, SOCK_STREAM, 0);
					sock.sin_family = AF_INET;
					//we query ipecho.net/plain using http
					sock.sin_addr.s_addr = inet_addr("146.255.36.1");
					sock.sin_port = htons(80);
					if(connect(sock_h, (struct sockaddr *)&sock,
						sizeof(struct sockaddr)) == -1) perror("wan sock");
					else {
						send(sock_h,
						"GET /plain HTTP/1.1\r\nHost: ipecho.net\r\nConnection: close\r\n\r\n"
					}
					*/

				} //end expensive lookups

				//now cheap or time sensitive lookups

				//query i3 for workspace information
				free_workspaces_list(workspaces_list);
				workspaces_list = get_i3_workspaces(i3_sock);

				//check the event buffer for incoming events
				int mouse_clicked = 0;
				int mouse_x = 0;
				int mouse_y = 0;
				int mouse_button = 0;
				while(XPending(d)) {
					XEvent e;
					XNextEvent(d, &e);
					if(e.type == ButtonPress) {
						XButtonEvent *be = (XButtonEvent *)&e;
						mouse_clicked = 1;
						mouse_x = be->x_root;
						mouse_y = be->y_root;
						mouse_button = be->button;
					}
				}

			// ========= iterate over each instance, drawing it =========

				struct instance *ins = instance_list;
				while(ins != NULL) {

					// ========= handle instance-specific events =========

						if(mouse_clicked) {
							//check if this instance has been clicked on
							if(mouse_x > ins->output->x &&
							   mouse_x <
									(ins->output->x + ins->output->width)) {

								//iterate over workspace layout list
								for(i = 0; i < MAX_WORKSPACES; i++) {

									//zero length name indicates the list
									//terminator
									if(!strlen(ins->ws_layout->ws_name[i]))
										break;

									//is the click event inside this workspace
									//name area
									if(mouse_x < ins->output->x +
												ins->ws_layout->x_max[i]) {

										if(mouse_button == 1) {
											//left click switches workspace
											char i3_payload[256];
											sprintf(i3_payload,
												"workspace %s", 
												ins->ws_layout->ws_name[i]);
											char *res;
											i3_ipc_send(&res, i3_sock, COMMAND,
														i3_payload);
											free_ipc_result(res);
										}
										break;
									}
								}

								//handle mouse wheel on time
								if(mouse_x >
										ins->output->x +
										ins->time_layout->x_min &&
								   mouse_x <
										ins->output->x +
										ins->time_layout->x_max) {

									if(mouse_button == 4) {
										//mw up
										alarm_s += conf->alarm_increment_s;
									}
									else if(mouse_button == 5) {
										//mw_down
										if(alarm_s < conf->alarm_increment_s)
											alarm_s = 0;
										else
											alarm_s -= conf->alarm_increment_s;
									}
								}

							}
						}

					// ========= draw the background =========

						//if we have got a background pixmap, we use it
						//otherwise we use tint colour with an alpha of 255
						if(ins->bg != NULL)
							XPutImage(d, ins->bb, ins->gc, ins->bg, 0, 0, 0, 0,
									  ins->output->width, conf->depth);
						else {
							set_cairo_source_colour(ins->cairo, conf->tintcol);
							cairo_paint_with_alpha(ins->cairo, 1.0);
						}
						
						//we overlay the alarm flash if required
						if(frame_time < alarm_activate + 7) {
							if((frame_time - alarm_activate + 1) % 2) {
								set_cairo_source_colour(ins->cairo,
														conf->alarmtintcol);
								cairo_paint(ins->cairo);
							}
						}

					// ========= left side =========

						uint32_t tlpadding = conf->lpadding;

						//workspaces
						struct i3_workspace *ws_head = workspaces_list;
						int ws_index = 0;
						while(ws_head != NULL) {
							//is this workspace on my output?
							if(!strcmp(ws_head->output, ins->output->name)) {
								//draw the text
								tlpadding +=
									render_workspace(ins->cairo, tlpadding,
													textheight, ws_head, LEFT);

								//store this x max for future click events
								strcpy(ins->ws_layout->ws_name[ws_index],
									   ws_head->name);
								ins->ws_layout->x_max[ws_index] =
									tlpadding + conf->divpadding + 1;
								ws_index++;

								//divider
								tlpadding += render_divider(ins->cairo,
															tlpadding, LEFT);
							}
							ws_head = ws_head->next;
						}
						//cap the workspace layout for this frame
						ins->ws_layout->ws_name[ws_index][0] = '\0';

					// ========= right side =========

						uint32_t trpadding = conf->rpadding;

						//time
						ins->time_layout->x_max =
							ins->output->width - trpadding;
						trpadding += render_time(ins->cairo,
										ins->output->width - trpadding,
										textheight, RIGHT);

						//alarm
						if(alarm_s > 0) {
							//divider
							trpadding += render_divider(ins->cairo,
										ins->output->width - trpadding, RIGHT);

							trpadding += render_alarm(ins->cairo, alarm_s,
										ins->output->width - trpadding,
										textheight, RIGHT);
						}
						ins->time_layout->x_min =
							ins->output->width - trpadding;

						//divider
						trpadding += render_divider(ins->cairo,
										ins->output->width - trpadding, RIGHT);

						//date
						trpadding += render_date(ins->cairo,
										ins->output->width - trpadding,
										textheight, RIGHT);

						//divider
						trpadding += render_divider(ins->cairo,
										ins->output->width - trpadding, RIGHT);

						//ifone
						if(ifone != NULL && ifone_prev_speed != NULL) {
							trpadding += render_interface(ins->cairo,
											ins->output->width - trpadding,
											textheight, ifone,
											ifone_prev_speed, ifone_speed, 
											RIGHT);

							//divider
							trpadding += render_divider(ins->cairo,
										ins->output->width - trpadding, RIGHT);
						}

						//iftwo
						if(iftwo != NULL && iftwo_prev_speed != NULL) {
							trpadding += render_interface(ins->cairo,
											ins->output->width - trpadding,
											textheight, iftwo,
											iftwo_prev_speed, iftwo_speed,
											RIGHT);

							//divider
							trpadding += render_divider(ins->cairo,
										ins->output->width - trpadding, RIGHT);
						}

						//fsone
						if(fsone_alive == 0) {
							trpadding += render_filesystem(ins->cairo,
									ins->output->width - trpadding,
									textheight, &fsone, conf->fsone, RIGHT);
							//divider
							trpadding += render_divider(ins->cairo,
									ins->output->width - trpadding, RIGHT);
						}

						//fstwo
						if(fstwo_alive == 0) {
							trpadding += render_filesystem(ins->cairo,
									ins->output->width - trpadding,
									textheight, &fstwo, conf->fstwo, RIGHT);
							//divider
							trpadding += render_divider(ins->cairo,
									ins->output->width - trpadding, RIGHT);
						}

						//batteries
						for(i = 0; i < batt_count; i++) {
							trpadding += render_battery(ins->cairo,
									ins->output->width - trpadding,
									textheight, batteries[i], RIGHT);
							//divider
							trpadding += render_divider(ins->cairo,
									ins->output->width - trpadding, RIGHT);
						}

						//thermal
						for(i = 0; i < thermal_count; i++) {
							trpadding += render_thermal(ins->cairo,
									ins->output->width - trpadding,
									textheight, thermals[i], RIGHT);
							//divider
							trpadding += render_divider(ins->cairo,
									ins->output->width - trpadding, RIGHT);
						}

					// ========= finish this frame =========

						//draw the back buffer and send to the screen
						XCopyArea(d, ins->bb, ins->w, ins->gc, 0, 0,
							ins->output->width, conf->depth, 0, 0);
						XFlush(d);

						//next instance
						ins = ins->next;

				} //end instance iteration

				//control fps
				usleep(50000);

		} //end main loop

}
