#include "lifebar.h"

int render_divider(XftDraw *xft, int x, int d) {
	if(d == RIGHT) {
		if(conf->divstyle == LINE) {
			XftDrawRect(xft, conf->divcol,
						x - (conf->divpadding + conf->divwidth), 3,
						conf->divwidth, 14);
			return (conf->divpadding * 2) + conf->divwidth + 1;
		}
	}
	else if(d == LEFT) {
		if(conf->divstyle == LINE) {
			XftDrawRect(xft, conf->divcol,
						x + conf->divpadding, 3,
						conf->divwidth, 14);
			return (conf->divpadding * 2) + conf->divwidth + 1;
		}
	}
}

XftColor *prepare_xft_colour(Display *d, int r, int g, int b, int a) {
	XRenderColor xrc;
	xrc.red = r * 256;
	xrc.green = g * 256;
	xrc.blue = b * 256;
	xrc.alpha = a * 256;
	XftColor *ret = (XftColor *)malloc(sizeof(XftColor));
	XftColorAllocValue(d, DefaultVisual(d, 0), DefaultColormap(d, 0),
					   &xrc, ret);
	return ret;
}
