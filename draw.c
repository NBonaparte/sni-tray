#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_aux.h>
#include <cairo-xcb.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <signal.h>
#include <unistd.h>
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

xcb_visualtype_t* visual_type(xcb_screen_t* screen, int match_depth) {
	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
	if (depth_iter.data) {
		for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
			if (match_depth == 0 || match_depth == depth_iter.data->depth) {
				for (xcb_visualtype_iterator_t it = xcb_depth_visuals_iterator(depth_iter.data); it.rem; xcb_visualtype_next(&it)) {
					return it.data;
				}
			}
		}
		if (match_depth > 0) {
			return visual_type(screen, 0);
		}
	}
	return NULL;
}

void mon_select(xcb_screen_t *s, xcb_rectangle_t *mon_dim, char *mon_name) {
	xcb_randr_get_screen_resources_current_reply_t *r = xcb_randr_get_screen_resources_current_reply(c,
		xcb_randr_get_screen_resources_current(c, s->root), NULL);
	if(!r)
		errx(1, "Failed to get screen resources");

	int mon_total = xcb_randr_get_screen_resources_current_outputs_length(r);
	xcb_randr_output_t *o = xcb_randr_get_screen_resources_current_outputs(r);

	if(mon_name != NULL) {
	for(int i = 0; i < mon_total; i++) {
		xcb_randr_get_output_info_reply_t *out = xcb_randr_get_output_info_reply(c,
			xcb_randr_get_output_info(c, o[i], XCB_CURRENT_TIME), NULL);

		if(out == NULL || out->crtc == XCB_NONE || out->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
			continue;

		xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(c,
			xcb_randr_get_crtc_info(c, out->crtc, XCB_CURRENT_TIME), NULL);
		if(!crtc) {
			free(r);
			errx(1, "Failed to get crtc info");
		}

		if(!strcmp(mon_name, (char *) xcb_randr_get_output_info_name(out))) {
			*mon_dim = (xcb_rectangle_t){crtc->x, crtc->y, crtc->width, crtc->height};
			break;
		}
		free(crtc);
		free(out);
	}
	}
	if(!mon_dim->width || !mon_dim->height) {
		warnx("Using primary monitor as fallback");
		// get primary output as fallback
		xcb_randr_get_output_primary_reply_t *primary = xcb_randr_get_output_primary_reply(c,
			xcb_randr_get_output_primary(c, s->root), NULL);
		xcb_randr_get_output_info_reply_t *pri = xcb_randr_get_output_info_reply(c,
			xcb_randr_get_output_info(c, primary->output, XCB_CURRENT_TIME), NULL);
		xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(c,
			xcb_randr_get_crtc_info(c, pri->crtc, XCB_CURRENT_TIME), NULL);
		*mon_dim = (xcb_rectangle_t){crtc->x, crtc->y, crtc->width, crtc->height};
		free(crtc);
		free(pri);
		free(primary);
	}

	printf("%d %d %d %d\n", mon_dim->x, mon_dim->y, mon_dim->width, mon_dim->height);
	free(r);
}

xcb_window_t main_win_init(xcb_screen_t *s, xcb_rectangle_t *dim) {
	xcb_window_t w = xcb_generate_id(c);

	depth = XCB_COPY_FROM_PARENT;
	visual = visual_type(s, 32);
	colormap = s->default_colormap;
	if(visual != NULL) {
		depth = xcb_aux_get_depth_of_visual(s, visual->visual_id);
		colormap = xcb_generate_id(c);
		if(xcb_request_check(c, xcb_create_colormap_checked(c, XCB_COLORMAP_ALLOC_NONE, colormap, s->root, visual->visual_id)) != NULL)
			errx(1, "aiodojf");
		printf("colormap: %d %d\n", colormap, s->default_colormap);
	}
	else
		;
		// TODO: switch to one version of visualtype function
		//visual = get_visualtype(s);

	printf("depth: %d\n", depth);
	//IMPORTANT: NEED TO DEFINE BACK AND BORDER PIXELS
	uint32_t mask[] = {s->black_pixel, s->black_pixel, 1, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS, colormap};

	if(xcb_request_check(c, xcb_create_window_checked(c, depth, w, s->root,
		dim->x, dim->y, dim->width, dim->height, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT, visual->visual_id,
		XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP, mask))!= NULL) {
		errx(1, "dank");
	}
	char *title = "SNI Tray";
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, w, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
	//conf_win(s, w, dim);
	xcb_map_window(c, w);
	return w;
}
static cairo_surface_t * draw_surface_from_pixbuf(GdkPixbuf *buf) {
    int width = gdk_pixbuf_get_width(buf);
    int height = gdk_pixbuf_get_height(buf);
    int pix_stride = gdk_pixbuf_get_rowstride(buf);
    guchar *pixels = gdk_pixbuf_get_pixels(buf);
    int channels = gdk_pixbuf_get_n_channels(buf);
    cairo_surface_t *surface;
    int cairo_stride;
    unsigned char *cairo_pixels;


    cairo_format_t format = CAIRO_FORMAT_ARGB32;
    if (channels == 3)
        format = CAIRO_FORMAT_RGB24;

    surface = cairo_image_surface_create(format, width, height);
    cairo_surface_flush(surface);
    cairo_stride = cairo_image_surface_get_stride(surface);
    cairo_pixels = cairo_image_surface_get_data(surface);

    for (int y = 0; y < height; y++) {
        guchar *row = pixels;
        uint32_t *cairo = (uint32_t *) cairo_pixels;
        for (int x = 0; x < width; x++) {
            if (channels == 3) {
                uint8_t r = *row++;
                uint8_t g = *row++;
                uint8_t b = *row++;
                *cairo++ = (r << 16) | (g << 8) | b;
            }
            else {
                uint8_t r = *row++;
                uint8_t g = *row++;
                uint8_t b = *row++;
                uint8_t a = *row++;
                double alpha = a / 255.0;
                r = r * alpha;
                g = g * alpha;
                b = b * alpha;
                *cairo++ = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
        pixels += pix_stride;
        cairo_pixels += cairo_stride;
    }

    cairo_surface_mark_dirty(surface);
    return surface;
}


cairo_surface_t * image_to_surface(char *path) {
	GError *err = NULL;
	cairo_surface_t *ret = NULL;
	GdkPixbuf *gbuf = gdk_pixbuf_new_from_file(path, &err);
	if(!gbuf) {
		printf("error\n");
		return NULL;
	}
	ret = draw_surface_from_pixbuf(gbuf);
	g_object_unref(gbuf);
	return ret;
}
/*
cairo_surface_t *draw_pixmap(Pixmap *px) {
	cairo_surface_t *ret = NULL;
	//TODO check cairo format
	ret = cairo_image_surface_create_for_data(px->pixmap, CAIRO_FORMAT_ARGB32, px->width, px->height,
			cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, px->width));
	return ret;
}
*/
void cairo_reset_surface(cairo_t *cr, rgba_t *bg) {
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, bg->r/255.0, bg->g/255.0, bg->b/255.0, bg->a/255.0);
	cairo_paint(cr);
	cairo_restore(cr);

}
void draw_image(cairo_t *dest, char *path) {
	cairo_surface_t *kek = image_to_surface(path);
	//cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(dest, kek, 0, 0);
	cairo_paint(dest);
	cairo_surface_destroy(kek);

}
void init_window(win_data *data) {
	c = xcb_connect(NULL, &(data->screen_num));
	xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	uint32_t vals[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
	xcb_change_window_attributes(c, s->root, XCB_CW_EVENT_MASK, vals);
	xcb_rectangle_t mon_dim = {0,0,0,0};
	mon_select(s, &mon_dim, "HDMI3");
	data->win_dim = (xcb_rectangle_t){mon_dim.x, mon_dim.y, 24, 24};

	xcb_window_t w = main_win_init(s, &(data->win_dim));

	/*
	xcb_pixmap_t buf_pix = xcb_generate_id(c);
	if(xcb_request_check(c, xcb_create_pixmap_checked(c, depth, buf_pix, w, win_dim.width, win_dim.height)))
		errx(1, "Failed to create pixmap");


	xcb_visualtype_t *visual_tray = xcb_aux_find_visual_by_id(s, s->root_visual);
	uint8_t depth = xcb_aux_get_depth_of_visual(s, visual->visual_id);
	cairo_surface_t *buf = cairo_xcb_surface_create(c, buf_pix, visual, win_dim.width, win_dim.height);
*/
	cairo_surface_t *surface = cairo_xcb_surface_create(c, w, visual, data->win_dim.width, data->win_dim.height);
	//cairo_t *cr_buf = cairo_create(buf);
	data->cr = cairo_create(surface);

	xcb_pixmap_t pixmap = xcb_generate_id(c);
	xcb_gcontext_t gc = xcb_generate_id(c);
	rgba_t bg = {0x00,0x00,0x00,0xee};
	cairo_reset_surface(data->cr, &bg);

	draw_image(data->cr, "/home/michael/.icons/Flattr Dark/status/24/nm-signal-50.svg");
}
int main() {
	win_data data;
	init_window(&data);

	xcb_generic_event_t *event;
	while((event = xcb_wait_for_event(c))) {
		xcb_flush(c);
	}
}