#include "gdbus.h"
#define HAS_SUFFIX(name) (g_str_has_suffix(name, ".svg") || g_str_has_suffix(name, ".png") || \
		g_str_has_suffix(name, ".xpm"))

/* Icon Lookup Algorithm
 * (see https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html for details)
 */
//TODO get IconThemePath
//TODO Add scales?
//
static gchar *find_icon_helper(gchar *icon, gint size, gchar *theme);
static gchar *lookup_icon(GKeyFile *kf, gchar *name, gint size, gchar *theme_path);
//this will be for /usr/share/pixmaps
static gchar *lookup_fallback_icon(gchar *name);
static gboolean dir_match_size(GKeyFile *kf, gchar *subdir, gint icon_size);
static gint dir_size_dist(GKeyFile *kf, gchar *subdir, gint icon_size);
static gchar *get_theme_location(const gchar *theme);

//TODO or have global theme in struct?
gchar *find_icon(gchar *icon, gint size, gchar *theme) {
	gchar *filename = find_icon_helper(icon, size, theme);
	if(filename != NULL)
		return filename;

	filename = find_icon_helper(icon, size, "hicolor");
	if(filename != NULL)
		return filename;

	return lookup_fallback_icon(icon);
}

static gchar *find_icon_helper(gchar *icon, gint size, gchar *theme) {
	GError *err = NULL;
	GKeyFile *kf = g_key_file_new();
	g_key_file_set_list_separator(kf, ',');
	gchar *theme_path = get_theme_location(theme), *filename = NULL;
	gchar **parents = NULL;
	if(theme_path == NULL) {
		fprintf(stderr, "Error finding theme %s\n", theme);
		return NULL;
	}
	gchar *theme_index = g_build_filename(theme_path, "index.theme", NULL);
	if(g_key_file_load_from_file(kf, theme_index, G_KEY_FILE_NONE, &err)) {
		filename = lookup_icon(kf, icon, size, theme_path);
		if(filename != NULL) {
			goto cleanup;
		}
		if((parents = g_key_file_get_string_list(kf, "Icon Theme", "Inherits", NULL, NULL)) != NULL) {
			for(int i = 0; parents[i] != NULL; i++) {
				filename = find_icon_helper(icon, size, parents[i]);
				if(filename != NULL) {
					goto cleanup;
				}
			}
		}
	}
	else {
		fprintf(stderr, "Error loading %s: %s\n", theme_path, err->message);
		g_error_free(err);
	}
cleanup:
	g_free(theme_index);
	g_free(theme_path);
	g_key_file_free(kf);
	g_strfreev(parents);
	return filename;
}

static gchar *lookup_icon(GKeyFile *kf, gchar *icon, gint size, gchar *theme_path) {
	GError *err = NULL;
	GDir *dir;
	const gchar *filename;
	gchar *enddir, *closest = NULL, *name;
	gchar **dirs;
	gpointer dot;
	gint min_size = G_MAXINT;
	if((dirs = g_key_file_get_string_list(kf, "Icon Theme", "Directories", NULL, &err)) != NULL) {
		for(int i = 0; dirs[i] != NULL; i++) {
			enddir = g_build_filename(theme_path, dirs[i], NULL);
			if((dir = g_dir_open(enddir, 0, NULL)) != NULL) {
				while((filename = g_dir_read_name(dir)) != NULL) {
					if((dot = g_strrstr_len(filename, -1, ".")) != NULL) {
						name = g_strndup(filename, dot - (gpointer)filename);
						if((g_strcmp0(name, icon) == 0) &&
								dir_match_size(kf, dirs[i], size) &&
								HAS_SUFFIX(filename)) {
							g_strfreev(dirs);
							//printf("%s\n", filename);
							g_free(name);
							closest = g_build_filename(enddir, filename, NULL);
							g_dir_close(dir);
							g_free(enddir);
							return closest;
						}
						g_free(name);
					}
				}
				g_dir_close(dir);
			}
			g_free(enddir);
		}
		for(int i = 0; dirs[i] != NULL; i++) {
			enddir = g_build_filename(theme_path, dirs[i], NULL);
			if((dir = g_dir_open(enddir, 0, NULL)) != NULL) {
				while((filename = g_dir_read_name(dir)) != NULL) {
					if((dot = g_strrstr_len(filename, -1, ".")) != NULL) {
						name = g_strndup(filename, dot - (gpointer)filename);
						if((g_strcmp0(name, icon) == 0) &&
								(dir_size_dist(kf, dirs[i], size) < min_size) &&
								HAS_SUFFIX(filename)) {
							g_free(closest);
							closest = g_build_filename(enddir, filename, NULL);
							min_size = dir_size_dist(kf, dirs[i], size);
						}
						g_free(name);
					}
				}
				g_dir_close(dir);
			}
			g_free(enddir);
		}
	}
	else {
		fprintf(stderr, "Error loading index.theme: %s\n", err->message);
		g_error_free(err);
	}
	g_strfreev(dirs);
	if(closest != NULL)
		return closest;
	return NULL;
}

static gchar *lookup_fallback_icon(gchar *icon) {
	GDir *dir;
	const gchar *filename;
	gpointer dot;
	gchar *name;
	if((dir = g_dir_open("/usr/share/pixmaps", 0, NULL)) != NULL) {
		while((filename = g_dir_read_name(dir)) != NULL) {
			if((dot = g_strrstr_len(filename, -1, ".")) != NULL) {
				name = g_strndup(filename, dot - (gpointer)filename);
				if((g_strcmp0(name, icon) == 0) && HAS_SUFFIX(filename)) {
					g_free(name);
					g_dir_close(dir);
					return g_build_filename("/usr/share/pixmaps", filename, NULL);
				}
				g_free(name);
			}
		}
		g_dir_close(dir);
	}
	return NULL;
}

static gboolean dir_match_size(GKeyFile *kf, gchar *subdir, gint icon_size) {
	gchar *type = NULL;
	gint size, min, max, thresh;
	if((type = g_key_file_get_string(kf, subdir, "Type", NULL)) == NULL)
		type = "Threshold";
	if((size = g_key_file_get_integer(kf, subdir, "Size", NULL)) == 0) {
		g_free(type);
		//no size specified, we can't do anything
		return FALSE;
	}
	if(g_strcmp0(type, "Fixed") == 0) {
		g_free(type);
		return size == icon_size;
	}
	else if(g_strcmp0(type, "Scaled") == 0) {
		g_free(type);
		if((min = g_key_file_get_integer(kf, subdir, "MinSize", NULL)) == 0)
			min = size;
		if((max = g_key_file_get_integer(kf, subdir, "MaxSize", NULL)) == 0)
			max = size;
		return (min <= icon_size) && (icon_size <= max);
	}
	else if(g_strcmp0(type, "Threshold") == 0) {
		g_free(type);
		if((thresh = g_key_file_get_integer(kf, subdir, "Threshold", NULL)) == 0)
			thresh = 2;
		return (size - thresh <= icon_size) && (icon_size <= size + thresh);
	}
	g_free(type);
	return FALSE;
}

static gint dir_size_dist(GKeyFile *kf, gchar *subdir, gint icon_size) {
	gchar *type = NULL;
	gint size, min, max, thresh;
	if((type = g_key_file_get_string(kf, subdir, "Type", NULL)) == NULL)
		type = "Threshold";
	if((size = g_key_file_get_integer(kf, subdir, "Size", NULL)) == 0) {
		g_free(type);
		return G_MAXINT;
	}
	if(g_strcmp0(type, "Fixed") == 0) {
		g_free(type);
		return ABS(size - icon_size);
	}
	else {
		if((min = g_key_file_get_integer(kf, subdir, "MinSize", NULL)) == 0)
			min = size;
		if((max = g_key_file_get_integer(kf, subdir, "MaxSize", NULL)) == 0)
			max = size;
		if(g_strcmp0(type, "Scaled") == 0) {
			g_free(type);
			if(icon_size < min)
				return min - icon_size;
			if(icon_size > max)
				return icon_size - max;
			return 0;
		}
		else if(g_strcmp0(type, "Threshold") == 0) {
			g_free(type);
			if((thresh = g_key_file_get_integer(kf, subdir, "Threshold", NULL)) == 0)
				thresh = 2;
			if(icon_size < (size - thresh))
				return min - icon_size;
			if(icon_size > (size + thresh))
				return icon_size - max;
			return 0;
		}
		g_free(type);
		return G_MAXINT;
	}
}

//free return value manually
static gchar *get_theme_location(const gchar *theme) {
	const gchar *home_dir = g_get_home_dir();
	const gchar *const *data_dirs = g_get_system_data_dirs();
	gchar *home_icons = g_build_filename(home_dir, ".icons", theme, NULL), *data_theme = NULL;
	if(g_file_test(home_icons, G_FILE_TEST_IS_DIR)) {
		return home_icons;
	}
	for(int i = 0; data_dirs[i] != NULL; i++) {
		data_theme = g_build_filename(data_dirs[i], "icons", theme, NULL);
		if(g_file_test(data_theme, G_FILE_TEST_IS_DIR)) {
			g_free(home_icons);
			return data_theme;
		}
		g_free(data_theme);
	}
	g_free(home_icons);
	return NULL;
}

static gchar *lookup_value_keyfile(gchar *loc, const gchar *group, const gchar *key) {
	GKeyFile *kf = g_key_file_new();
	GError *err1, *err2;
	gchar *ret = NULL;
	if(g_key_file_load_from_file(kf, loc, G_KEY_FILE_NONE, &err1)) {
		if((ret = g_key_file_get_string(kf, group, key, &err2)) != NULL) {
			g_key_file_free(kf);
			return ret;
		}
		else {
			fprintf(stderr, "Error getting value of %s in %s: %s\n", key, loc, err2->message);
			g_error_free(err2);
		}
	}
	else {
			fprintf(stderr, "Error loading file %s: %s\n", loc, err1->message);
			g_error_free(err1);
	}
	g_key_file_free(kf);
	return NULL;
}
static gchar *lookup_value_rc(const gchar *loc, const gchar *key) {
	GFile *file = g_file_new_for_path(loc);
	GError *err;
	GFileInputStream *file_in = g_file_read(file, NULL, &err);
	GDataInputStream *data = NULL;
	char *buf, *ret;
	gsize len;
	if (file_in == NULL) {
		g_printerr("Error accessing file %s: %s\n", loc, err->message);
		g_error_free(err);
	}
	data = g_data_input_stream_new((GInputStream *) file_in);
	while((buf = g_data_input_stream_read_line(data, &len, NULL, NULL))) {
		if(g_str_has_prefix(buf, key)) {
			const gchar *beg = g_strstr_len(buf, -1, "\"");
			gpointer end = g_strrstr_len(buf, -1, "\"");
			if((beg != NULL) && (end != NULL) && (beg != end))
				ret = g_strndup(beg + 1, end - (gpointer) beg - 1);
		}
		g_free(buf);
	}
	g_object_unref(data);
	g_object_unref(file_in);
	g_object_unref(file);
	return ret;
}

//manual way to get icon themes because we can't use gtk_icon_theme_get_default
//and gtkrc-2.0 isn't a .ini/keyfile :/
gchar * get_icon_theme() {
	const gchar *home_dir = g_get_home_dir(), *config_dir = g_get_user_config_dir();
	gchar *gtkrc_2 = g_build_filename(home_dir, ".gtkrc-2.0", NULL),
		*gtkrc_3 = g_build_filename(config_dir, "gtk-3.0", "settings.ini", NULL),
		*icon_theme = NULL;

	if(g_file_test(gtkrc_3, G_FILE_TEST_IS_REGULAR)) {
		icon_theme = lookup_value_keyfile(gtkrc_3, "Settings", "gtk-icon-theme-name");
		if(icon_theme != NULL) {
			goto cleanup;
		}
	}
	if(g_file_test(gtkrc_2, G_FILE_TEST_IS_REGULAR)) {
		icon_theme = lookup_value_rc(gtkrc_2, "gtk-icon-theme-name");
		if(icon_theme != NULL) {
			goto cleanup;
		}
	}
cleanup:
	g_free(gtkrc_2);
	g_free(gtkrc_3);
	return icon_theme;
}
/*
int main(int argc, char **argv) {
	gchar *dank = get_icon_theme();
	printf("Theme: %s\n", dank);
	if(argc > 2) {
		gchar *loc = find_icon(argv[1], g_ascii_strtoll(argv[2], NULL, 10), dank);
		if(loc)
			printf("%s\n", loc);
		else
			printf("Could not find\n");
		g_free(loc);
	}
	g_free(dank);
	return 0;
}*/
