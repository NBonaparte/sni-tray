#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//for getpid()
#include <sys/types.h>
#include <unistd.h>
//struct to hold all pixmap data
//TODO replace gint32 with int32_t? (from stdint.h)
typedef struct Pixmap {
	gint32 width;
	gint32 height;
	GBytes *pixmap;
} Pixmap;
//struct to hold all properties for item
typedef struct ItemData {
	GDBusProxy *proxy;
	gchar *dbus_name;
	gchar *category;
	gchar *id;
	gchar *title;
	gchar *status;
	guint32 win_id;
	gchar *icon_name;
	gchar *icon_path; //NOT theme_path
	gchar *theme_path;
	Pixmap *icon_pixmap;
	gchar *overlay_name;
	gchar *att_name;
	Pixmap *att_pixmap;
	gchar *movie_name;
	//TODO tooltip icon name, icon pixmap, title, description
	gboolean ismenu;
	GVariant *obj_path;
} ItemData;

//use g_list_length(list) to find number of elements
GList *list;


gchar *find_icon(gchar *icon, gint size, gchar *theme);
gchar *get_icon_theme();
