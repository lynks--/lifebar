#include "lifebar.h"

int render_divider(cairo_t *cairo, int x, int d) {
	if(d == RIGHT) {
		if(conf->divstyle == LINE) {
			set_cairo_source_colour(cairo, conf->divcol);
			cairo_rectangle(cairo,
							x - (conf->divpadding + conf->divwidth), 3,
							conf->divwidth, conf->depth - 6);
			cairo_fill(cairo);
			return (conf->divpadding * 2) + conf->divwidth + 1;
		}
		else if(conf->divstyle == GROOVE) {
			set_cairo_source_colour(cairo, conf->groove_light);
			cairo_rectangle(cairo, x - conf->divpadding, 0,
								   1, conf->depth);
			cairo_fill(cairo);
			set_cairo_source_colour(cairo, conf->groove_dark);
			cairo_rectangle(cairo, x - (conf->divpadding + 1), 0,
								   1, conf->depth);
			cairo_fill(cairo);
			return (conf->divpadding * 2) + 2;
		}
	}
	else if(d == LEFT) {
		if(conf->divstyle == LINE) {
			set_cairo_source_colour(cairo, conf->divcol);
			cairo_rectangle(cairo,
							x + conf->divpadding, 3,
							conf->divwidth, conf->depth - 6);
			cairo_fill(cairo);
			return (conf->divpadding * 2) + conf->divwidth + 1;
		}
		else if(conf->divstyle == GROOVE) {
			set_cairo_source_colour(cairo, conf->groove_light);
			cairo_rectangle(cairo, x + conf->divpadding + 1, 0,
								   1, conf->depth);
			cairo_fill(cairo);
			set_cairo_source_colour(cairo, conf->groove_dark);
			cairo_rectangle(cairo, x + conf->divpadding, 0,
								   1, conf->depth);
			cairo_fill(cairo);
			return (conf->divpadding * 2) + 2;
		}
	}
	return 10;
}

int render_interface(cairo_t *c, int x, int y, struct ifaddrs *a,
			struct net_speed_info *p_spd, struct net_speed_info *spd, int d) {

	char k_string[32];
	char v_string[64];
	sprintf(k_string, "%s", a->ifa_name);

	//if this interface is up
	if(a->ifa_flags & IFF_UP) {

		//if this is an ipv4 or ipv6 address
		if((a->ifa_addr->sa_family == AF_INET) ||
		   (a->ifa_addr->sa_family == AF_INET6)) {

			//parse the address to readable string
			struct sockaddr_in *addr = (struct sockaddr_in *)a->ifa_addr;
			char readable[256];
			inet_ntop(addr->sin_family, &(addr->sin_addr), readable, 256);

			//calculate speed since last tick
			float down = (float)(spd->down_bytes - p_spd->down_bytes) /
									(float)(EXPENSIVE_TIME * 1024);
			float up = (float)(spd->up_bytes - p_spd->up_bytes) /
									(float)(EXPENSIVE_TIME * 1024);

			//build the string
			sprintf(v_string, "%s [\xe2\x86\x93 %.2fKiB/s] [\xe2\x86\x91 %.2fKiB/s]",
					 readable, down, up);
		}
		else sprintf(v_string, "down");
	}
	else sprintf(v_string, "down");

	return render_keyvalue(c, x, y, k_string, v_string, d);
}

int render_filesystem(cairo_t *c, int x, int y, struct statvfs *fs,
														char *path, int d) {

	unsigned long long fs_cap =
		(unsigned long long)fs->f_bsize * fs->f_blocks;
	unsigned long long fs_free =
		(unsigned long long)fs->f_bsize * fs->f_bavail;

	char k_string[32];
	char v_string[32];
	int giga = 1024 * 1024 * 1024;

	sprintf(k_string, "%s", path);
	sprintf(v_string, "%.1fGiB", (double)fs_free / giga);

	return render_keyvalue(c, x, y, k_string, v_string, d);
}

int render_workspace(cairo_t *cairo, int x, int y,
					 struct i3_workspace *ws, int d) {

	//set colour based on visibility
	struct colour *c = conf->inviswscol;
	if(strcmp(ws->visible, "true") == 0) c = conf->viswscol;
	set_cairo_source_colour(cairo, c);

	//set font
	cairo_set_font_face(cairo, conf->wsfont);
	cairo_set_font_size(cairo, conf->wsfontsize);

	cairo_text_extents_t extents;
	cairo_text_extents(cairo, ws->name, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;

	//draw the text
	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, ws->name);

	//update the padding
	return extents.width + 2;
}

int render_battery(cairo_t *c, int x, int y, struct batt_info *b, int d) {
	char k_string[32];
	char v_string[32];

	sprintf(k_string, "batt-%d", b->index);

	switch(b->status) {
		case CHARGING:
			sprintf(v_string, "charging %d%%", b->percent);
			break;
		case DISCHARGING:
			sprintf(v_string, "discharging %d%%", b->percent);
			break;
		case FULL:
			sprintf(v_string, "full", b->index);
			break;
		case UNKNOWN:
			sprintf(v_string, "status unknown");
	}

	return render_keyvalue(c, x, y, k_string, v_string, d);
}

int render_thermal(cairo_t *c, int x, int y, struct thermal_info *t, int d) {
	char k_string[32];
	char v_string[32];
	sprintf(k_string, "thermal-%d", t->index);
	sprintf(v_string, "%d\xc2\xb0" "C", t->temp_c);

	return render_keyvalue(c, x, y, k_string, v_string, d);
}

int render_time(cairo_t *cairo, int x, int y, int d) {
	time_t rawnow = time(NULL);
	struct tm *now = localtime(&rawnow);
	char time_string[128];

	strftime(time_string, 128, conf->timefmt, now);

	set_cairo_source_colour(cairo, conf->timecol);
	cairo_set_font_face(cairo, conf->timefont);
	cairo_set_font_size(cairo, conf->timefontsize);

	cairo_text_extents_t extents;
	cairo_text_extents(cairo, time_string, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;

	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, time_string);

	return extents.width - 1;
}

int render_alarm(cairo_t *cairo, uint32_t alarm_s, int x, int y, int d) {
	struct tm *atm = malloc(sizeof *atm);
	memset(atm, 0, sizeof *atm);
	char *format = malloc(32);
	sprintf(format, "%s", "%M:%S");
	char time_string[128];

	while(alarm_s >= 3600) {
		atm->tm_hour++;
		alarm_s -= 3600;
		sprintf(format, "%s", "%H:%M:%S");
	}
	while(alarm_s >= 60) {
		atm->tm_min++;
		alarm_s -= 60;
	}
	atm->tm_sec = alarm_s;


	strftime(time_string, 128, format, atm);
	free(atm);
	free(format);

	set_cairo_source_colour(cairo, conf->alarmcol);
	cairo_set_font_face(cairo, conf->timefont);
	cairo_set_font_size(cairo, conf->timefontsize);

	cairo_text_extents_t extents;
	cairo_text_extents(cairo, time_string, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;

	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, time_string);

	return extents.width - 1;
}

int render_date(cairo_t *cairo, int x, int y, int d) {
	time_t rawnow = time(NULL);
	struct tm *now = localtime(&rawnow);
	char date_string[256];

	strftime(date_string, 256, conf->datefmt, now);

	set_cairo_source_colour(cairo, conf->datecol);
	cairo_set_font_face(cairo, conf->datefont);
	cairo_set_font_size(cairo, conf->datefontsize);

	cairo_text_extents_t extents;
	cairo_text_extents(cairo, date_string, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;

	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, date_string);

	return extents.width - 1;
}

int render_keyvalue(cairo_t *cairo, int x, int y, char *k, char *v , int d) {
	uint32_t render_x;
	cairo_text_extents_t k_extents;
	cairo_text_extents_t v_extents;

	//we must set the fonts to measure extents
	cairo_set_font_face(cairo, conf->keyfont);
	cairo_set_font_size(cairo, conf->keyfontsize);
	cairo_text_extents(cairo, k, &k_extents);
	cairo_set_font_face(cairo, conf->valfont);
	cairo_set_font_size(cairo, conf->valfontsize);
	cairo_text_extents(cairo, v, &v_extents);

	//render key
	set_cairo_source_colour(cairo, conf->keycol);
	cairo_set_font_face(cairo, conf->keyfont);
	cairo_set_font_size(cairo, conf->keyfontsize);
	render_x = x;
	if(d == RIGHT)
		render_x = x - (k_extents.width + v_extents.width + conf->kvpadding);
	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, k);

	//render value
	set_cairo_source_colour(cairo, conf->valcol);
	cairo_set_font_face(cairo, conf->valfont);
	cairo_set_font_size(cairo, conf->valfontsize);
	render_x = x + k_extents.width + conf->kvpadding;
	if(d == RIGHT)
		render_x = x - v_extents.width;
	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, v);

	return k_extents.width + conf->kvpadding + v_extents.width;
}

struct colour *prepare_colour(int r, int g, int b, int a) {
	struct colour *c = malloc(sizeof c);
	c->red = r;
	c->green = g;
	c->blue = b;
	c->alpha = a;
	return c;
}

struct colour *parse_config_colour(char *value) {
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 0;
	int success = sscanf(value, " %3d , %3d , %3d , %3d ", &r, &g, &b, &a);

	if(success == 4) return prepare_colour(r, g, b, a);

	fprintf(stderr, "%sbad formatting of config colour: '%s'\n",
			BAD_MSG, value);
	return NULL;
}

void parse_config_font(const char *confkey, char *value) {
	char face[64];
	char weight[8];
	int size;
	int success = sscanf(value, " %64[^:] : %8[^:] : %2u",
								face, weight, &size);

	cairo_font_weight_t weight_t = CAIRO_FONT_WEIGHT_NORMAL;
	if(strcmp(weight, "bold") == 0) weight_t = CAIRO_FONT_WEIGHT_BOLD;

	if(success == 3) {

		//build the font
		cairo_font_face_t *font =
			cairo_toy_font_face_create(face,
									   CAIRO_FONT_SLANT_NORMAL,
									   weight_t);

		//TODO replace this mess with enums and config array
		if(strcmp(confkey, "keyfont") == 0) {
			conf->keyfont = font;
			conf->keyfontsize = (double)size;
		}
		else if(strcmp(confkey, "valfont") == 0) {
			conf->valfont = font;
			conf->valfontsize = (double)size;
		}
		else if(strcmp(confkey, "datefont") == 0) {
			conf->datefont = font;
			conf->datefontsize = (double)size;
		}
		else if(strcmp(confkey, "timefont") == 0) {
			conf->timefont = font;
			conf->timefontsize = (double)size;
		}
		else if(strcmp(confkey, "wsfont") == 0) {
			conf->wsfont = font;
			conf->wsfontsize = (double)size;
		}
	}
	else fprintf(stderr, "%sbad formatting of config font: '%s'\n",
				 BAD_MSG, value);
}

void set_cairo_source_colour(cairo_t *cairo, struct colour *col) {
	//cairo uses 0.0->1.0 for 0->255
	double r = col->red / (double)255;
	double g = col->green / (double)255;
	double b = col->blue / (double)255;
	double a = col->alpha / (double)255;
	cairo_set_source_rgba(cairo, r, g, b, a);
}
