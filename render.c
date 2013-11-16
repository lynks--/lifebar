#include "lifebar.h"

int render_divider(XftDraw *xft, int x, int d) {
	
}

XftColor *prepare_xft_colour(int r, int g, int b, int a) {
	XRenderColor xrc;
	xrc.red = r * 256;
	xrc.green = g * 256;
	xrc.blue = b * 256;
	xrc.alpha = a * 256;
	XftColor *ret = (XftColor *)malloc(sizeof(XftColor));
	XftColorAllocValue(d, DefaultVisual(d, 0), DefaultColorMap(d, 0),
					   &xrc, ret);
	return ret;
}
