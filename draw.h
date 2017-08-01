#include <xcb/xcb.h>
#include <cairo-xcb.h>

xcb_connection_t *c;
xcb_visualtype_t *visual;
xcb_colormap_t colormap;
uint8_t depth;

typedef struct {
	uint8_t r, g, b, a;
} rgba_t;


typedef struct win_data {
	int screen_num;
	rgba_t bg;
	cairo_t *cr;
	xcb_window_t w;
	xcb_rectangle_t win_dim;

} win_data;

