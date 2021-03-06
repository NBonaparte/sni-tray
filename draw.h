#include <xcb/xcb.h>
#include <cairo-xcb.h>
#include <gio/gio.h>
#include "libgwater/xcb/libgwater-xcb.h"

xcb_connection_t *c;
xcb_window_t w;
xcb_visualtype_t *visual;
xcb_colormap_t colormap;
uint8_t depth;
int screen_num;

xcb_rectangle_t win_dim;
cairo_t *cr;
cairo_surface_t *surface;

typedef struct {
	uint8_t r, g, b, a;
} rgba_t;

rgba_t bg;

enum click_type {
	PRIMARY = 1,
	SECONDARY,
	CONTEXT,
	UNUSED,
	SCROLL
};
gboolean callback(xcb_generic_event_t *event, gpointer user_data);
void draw_tray();
void init_window();
