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

int render_interface(cairo_t *cairo, int x, int y, struct ifaddrs *a, int d) {
	char if_string[256];
	if_string[0] = '\0';

	//if this interface is up
	if(a->ifa_flags & IFF_UP) {
		//if this is an ipv4 or ipv6 address
		if((a->ifa_addr->sa_family == AF_INET) ||
		   (a->ifa_addr->sa_family == AF_INET6)) {
			struct sockaddr_in *addr = (struct sockaddr_in *)a->ifa_addr;
			char readable[256];
			inet_ntop(addr->sin_family, &(addr->sin_addr), readable, 256);
		
			sprintf(if_string, "%s %s", conf->ifone, readable);
		}
		else sprintf(if_string, "%s down", conf->ifone);
	}
	else sprintf(if_string, "%s down", conf->ifone);

	if(strlen(if_string) > 0) {
		//TODO set down color above
		set_cairo_source_colour(cairo, conf->keycol);

		cairo_text_extents_t extents;
		cairo_text_extents(cairo, if_string, &extents);

		int render_x = x;
		if(d == RIGHT) render_x = x - extents.width;

		cairo_move_to(cairo, render_x, y);
		cairo_show_text(cairo, if_string);
		return extents.width + 1;
	}
	return 10;
}

int render_filesystem(cairo_t *cairo, int x, int y,
					  struct statvfs *fs, char *path, int d) {

	unsigned long long fs_cap =
		(unsigned long long)fs->f_bsize * fs->f_blocks;
	unsigned long long fs_free =
		(unsigned long long)fs->f_bsize * fs->f_bavail;

	char str[64];
	int giga = 1024 * 1024 * 1024;
	sprintf(str, "%s %.1fGiB", path, (double)fs_free / giga);

	set_cairo_source_colour(cairo, conf->keycol);
	cairo_text_extents_t extents;
	cairo_text_extents(cairo, str, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;
	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, str);

	return extents.width;
}

int render_workspace(cairo_t *cairo, int x, int y,
					 struct i3_workspace *ws, int d) {

	//set colour based on visibility
	struct colour *c = conf->inviswscol;
	if(strcmp(ws->visible, "true") == 0) c = conf->viswscol;
	set_cairo_source_colour(cairo, c);

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

int render_battery(cairo_t *cairo, int x, int y,
					 struct batt_info *batt, int d) {

	char batt_string[64];

	switch(batt->status) {
		case CHARGING:
			sprintf(batt_string, "batt%d charging %d%%",
					batt->index, batt->percent);
			break;
		case DISCHARGING:
			sprintf(batt_string, "batt%d discharging %d%%",
					batt->index, batt->percent);
			break;
		case FULL:
			sprintf(batt_string, "batt%d full", batt->index);
			break;
		case UNKNOWN:
			sprintf(batt_string, "batt%d status unknown", batt->index);
	}

	cairo_text_extents_t extents;
	cairo_text_extents(cairo, batt_string, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;

	//draw the text
	set_cairo_source_colour(cairo, conf->keycol);
	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, batt_string);

	//update the padding
	return extents.width + 2;
}

int render_time(cairo_t *cairo, int x, int y, int d) {
	time_t rawnow = time(NULL);
	struct tm *now = localtime(&rawnow);
	char time_string[128];

	strftime(time_string, 128, conf->timefmt, now);

	set_cairo_source_colour(cairo, conf->keycol);

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

	set_cairo_source_colour(cairo, conf->keycol);

	cairo_text_extents_t extents;
	cairo_text_extents(cairo, date_string, &extents);

	int render_x = x;
	if(d == RIGHT) render_x = x - extents.width;

	cairo_move_to(cairo, render_x, y);
	cairo_show_text(cairo, date_string);

	return extents.width - 1;
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
	int success = sscanf(value, " %d , %d , %d , %d ", &r, &g, &b, &a);

	if(success == 4) return prepare_colour(r, g, b, a);

	fprintf(stderr, "%sbad formatting of config colour: '%s'\n",
			BAD_MSG, value);
	return NULL;
}

void set_cairo_source_colour(cairo_t *cairo, struct colour *col) {
	//cairo uses 0.0->1.0 for 0->255
	double r = col->red / (double)255;
	double g = col->green / (double)255;
	double b = col->blue / (double)255;
	double a = col->alpha / (double)255;
	cairo_set_source_rgba(cairo, r, g, b, a);
}
