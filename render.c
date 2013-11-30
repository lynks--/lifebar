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
			cairo_rectangle(cairo, x - conf->divpadding, 0, 2, conf->depth);
			cairo_fill(cairo);
			set_cairo_source_colour(cairo, conf->groove_dark);
			cairo_rectangle(cairo, x - (conf->divpadding + 1), 0, 2, conf->depth);
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
		}
	}
	return 10;
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
	double r = col->red / (double)255;
	double g = col->green / (double)255;
	double b = col->blue / (double)255;
	double a = col->alpha / (double)255;
	cairo_set_source_rgba(cairo, r, g, b, a);
}
