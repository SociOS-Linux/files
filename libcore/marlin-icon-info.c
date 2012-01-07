/* nautilus-icon-info.c
 * Copyright (C) 2007  Red Hat, Inc.,  Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

//#include <config.h>
#include <string.h>
#include "marlin-icon-info.h"
#include <gtk/gtk.h>
#include <gio/gio.h>

struct _MarlinIconInfo
{
    GObject         parent;

    guint64         last_use_time;
    GdkPixbuf       *pixbuf;

    /*gboolean        got_embedded_rect;
    GdkRectangle    embedded_rect;
    gint            n_attach_points;
    GdkPoint        *attach_points;*/
    char            *display_name;
    char            *icon_name;
};

struct _MarlinIconInfoClass
{
    GObjectClass parent_class;
};

static void schedule_reap_cache (void);

G_DEFINE_TYPE (MarlinIconInfo, marlin_icon_info, G_TYPE_OBJECT);

static void
marlin_icon_info_init (MarlinIconInfo *icon)
{
    icon->last_use_time = g_get_monotonic_time ();
    icon->pixbuf = NULL;
}

gboolean
marlin_icon_info_is_fallback (MarlinIconInfo  *icon)
{
    return icon->pixbuf == NULL;
}

static void
pixbuf_toggle_notify (gpointer      info,
                      GObject      *object,
                      gboolean      is_last_ref)
{
    MarlinIconInfo  *icon = info;

    /*g_warning ("%s %s %s ref_count %u", G_STRFUNC, icon->display_name, 
               icon->icon_name, G_OBJECT (icon->pixbuf)->ref_count);*/
    if (is_last_ref) {
        /*g_object_remove_toggle_ref (object,
                                    pixbuf_toggle_notify,
                                    info);*/
        icon->last_use_time = g_get_monotonic_time ();
        schedule_reap_cache ();
    }
}

static void
marlin_icon_info_finalize (GObject *object)
{
    MarlinIconInfo *icon;

    icon = MARLIN_ICON_INFO (object);

    if (icon->pixbuf) {
        g_warning ("%s %s %s ref_count %u", G_STRFUNC, icon->display_name, 
                   icon->icon_name, G_OBJECT (icon->pixbuf)->ref_count);

        /*g_object_remove_toggle_ref (G_OBJECT (icon->pixbuf),
                                    pixbuf_toggle_notify,
                                    icon);*/
        g_clear_object (&icon->pixbuf);
    }
    
    //g_free (icon->attach_points);
    g_free (icon->display_name);
    g_free (icon->icon_name);

    G_OBJECT_CLASS (marlin_icon_info_parent_class)->finalize (object);
}

static void
marlin_icon_info_class_init (MarlinIconInfoClass *icon_info_class)
{
    GObjectClass *gobject_class;

    gobject_class = (GObjectClass *) icon_info_class;

    gobject_class->finalize = marlin_icon_info_finalize;

}

MarlinIconInfo *
marlin_icon_info_new_for_pixbuf (GdkPixbuf *pixbuf)
{
    MarlinIconInfo *icon;

    icon = g_object_new (MARLIN_TYPE_ICON_INFO, NULL);
    icon->pixbuf = pixbuf;
    if (icon->pixbuf != NULL) {
        g_object_add_toggle_ref (G_OBJECT (icon->pixbuf),
                                 pixbuf_toggle_notify,
                                 icon);
        g_object_unref (icon->pixbuf);
    }

    return icon;
}

static MarlinIconInfo *
marlin_icon_info_new_for_icon_info (GtkIconInfo *icon_info)
{
    MarlinIconInfo *icon;
    /*GdkPoint *points;
    gint n_points;*/
    const char *filename;
    char *basename, *p;

    icon = g_object_new (MARLIN_TYPE_ICON_INFO, NULL);

    icon->pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
    g_object_add_toggle_ref (G_OBJECT (icon->pixbuf),
                             pixbuf_toggle_notify,
                             icon);
    g_object_unref (icon->pixbuf);

    /*icon->got_embedded_rect = gtk_icon_info_get_embedded_rect (icon_info,
                                                               &icon->embedded_rect);

    if (gtk_icon_info_get_attach_points (icon_info, &points, &n_points)) {
        icon->n_attach_points = n_points;
        icon->attach_points = points;
    }*/

    icon->display_name = g_strdup (gtk_icon_info_get_display_name (icon_info));

    filename = gtk_icon_info_get_filename (icon_info);
    if (filename != NULL) {
        basename = g_path_get_basename (filename);
        p = strrchr (basename, '.');
        if (p) {
            *p = 0;
        }
        icon->icon_name = basename;
    }

    return icon;
}


typedef struct  {
    GIcon *icon;
    int size;
} LoadableIconKey;

typedef struct {
    char *filename;
    int size;
} ThemedIconKey;

static GHashTable *loadable_icon_cache = NULL;
static GHashTable *themed_icon_cache = NULL;
static guint reap_cache_timeout = 0;

#define MICROSEC_PER_SEC ((guint64)1000000L)

static guint64 time_now;

static gboolean
reap_old_icon (gpointer  key,
               gpointer  value,
               gpointer  user_info)
{
    MarlinIconInfo *icon = value;
    gboolean *reapable_icons_left = user_info;

    //g_message ("%s %s %u %u\n", G_STRFUNC, icon->icon_name, time_now, icon->last_use_time);
    /* sole owner */
    if (icon->pixbuf && G_OBJECT (icon->pixbuf)->ref_count == 1) {
        if (time_now - icon->last_use_time > 30 * MICROSEC_PER_SEC) {
            /*g_warning ("DELETE %s %s %u ref_count %u\n", G_STRFUNC, icon->icon_name, 
                       time_now - icon->last_use_time,
                       G_OBJECT (icon->pixbuf)->ref_count);*/
            /* This went unused 30 secs ago. reap */
            return TRUE;
        } else {
            /* We can reap this soon */
            *reapable_icons_left = TRUE;
        }
    }

    return FALSE;
}

static gboolean
reap_cache (gpointer data)
{
    gboolean reapable_icons_left;

    reapable_icons_left = TRUE;

    time_now = g_get_monotonic_time ();

    if (loadable_icon_cache) {
        g_hash_table_foreach_remove (loadable_icon_cache,
                                     reap_old_icon,
                                     &reapable_icons_left);
    }

    if (themed_icon_cache) {
        g_hash_table_foreach_remove (themed_icon_cache,
                                     reap_old_icon,
                                     &reapable_icons_left);
    }

    if (reapable_icons_left) {
        return TRUE;
    } else {
        reap_cache_timeout = 0;
        return FALSE;
    }
}

static void
schedule_reap_cache (void)
{
    if (reap_cache_timeout == 0) {
        reap_cache_timeout = g_timeout_add_seconds_full (0, 5,
                                                         reap_cache,
                                                         NULL, NULL);
    }
}

void
marlin_icon_info_infos_caches (void)
{
    if (loadable_icon_cache) {
        g_warning (">>> %s loadable_icon_cache %u", G_STRFUNC, 
                   g_hash_table_size (loadable_icon_cache));
    }
    if (themed_icon_cache) {
        g_warning (">>> %s themed_icon_cache %u", G_STRFUNC, 
                   g_hash_table_size (themed_icon_cache));
    }
}

void
marlin_icon_info_clear_caches (void)
{
    if (loadable_icon_cache) {
        g_hash_table_remove_all (loadable_icon_cache);
    }

    if (themed_icon_cache) {
        g_hash_table_remove_all (themed_icon_cache);
    }
}

static guint
loadable_icon_key_hash (LoadableIconKey *key)
{
    return g_icon_hash (key->icon) ^ key->size;
}

static gboolean
loadable_icon_key_equal (const LoadableIconKey *a,
                         const LoadableIconKey *b)
{
    return a->size == b->size &&
        g_icon_equal (a->icon, b->icon);
}

static LoadableIconKey *
loadable_icon_key_new (GIcon *icon, int size)
{
    LoadableIconKey *key;

    key = g_slice_new (LoadableIconKey);
    key->icon = g_object_ref (icon);
    key->size = size;

    return key;
}

static void
loadable_icon_key_free (LoadableIconKey *key)
{
    g_object_unref (key->icon);
    g_slice_free (LoadableIconKey, key);
}

static guint
themed_icon_key_hash (ThemedIconKey *key)
{
    return g_str_hash (key->filename) ^ key->size;
}

static gboolean
themed_icon_key_equal (const ThemedIconKey *a,
                       const ThemedIconKey *b)
{
    return a->size == b->size &&
        g_str_equal (a->filename, b->filename);
}

static ThemedIconKey *
themed_icon_key_new (const char *filename, int size)
{
    ThemedIconKey *key;

    key = g_slice_new (ThemedIconKey);
    key->filename = g_strdup (filename);
    key->size = size;

    return key;
}

static void
themed_icon_key_free (ThemedIconKey *key)
{
    g_free (key->filename);
    g_slice_free (ThemedIconKey, key);
}

MarlinIconInfo *
marlin_icon_info_lookup (GIcon *icon, int size)
{
    MarlinIconInfo *icon_info;
    GdkPixbuf *pixbuf = NULL;

    if (G_IS_LOADABLE_ICON (icon)) {
        LoadableIconKey lookup_key;
        LoadableIconKey *key;
        GInputStream *stream;

        if (loadable_icon_cache == NULL) {
            loadable_icon_cache =
                g_hash_table_new_full ((GHashFunc)loadable_icon_key_hash,
                                       (GEqualFunc)loadable_icon_key_equal,
                                       (GDestroyNotify) loadable_icon_key_free,
                                       (GDestroyNotify) g_object_unref);
        }

        lookup_key.icon = icon;
        lookup_key.size = size;

        icon_info = g_hash_table_lookup (loadable_icon_cache, &lookup_key);
        if (icon_info) {
            //g_message ("CACHED %s stream %s\n", G_STRFUNC, g_icon_to_string (icon));
            return g_object_ref (icon_info);
        }

//#if 0
        stream = g_loadable_icon_load (G_LOADABLE_ICON (icon),
                                       size,
                                       NULL, NULL, NULL);
        if (stream) {
            //g_message ("%s stream %s\n", G_STRFUNC, g_icon_to_string (icon));

            /* sounds like from_stream_at scale is leaking ? */
            pixbuf = gdk_pixbuf_new_from_stream_at_scale (stream,
                                                          size, size, TRUE,
                                                          NULL, NULL);
            //pixbuf = eel_gdk_pixbuf_load_from_stream_at_size (stream, size);
            g_input_stream_close (stream, NULL, NULL);
            g_object_unref (stream);
        }
//#endif
        //pixbuf = gdk_pixbuf_new_from_file_at_size ("/usr/share/icons/hicolor/scalable/apps/marlin.svg", size, size, NULL);
        /*gchar *icon_path = g_icon_to_string (icon);
        pixbuf = gdk_pixbuf_new_from_file_at_size (icon_path, size, size, NULL);
        //pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
        g_free (icon_path);*/

        icon_info = marlin_icon_info_new_for_pixbuf (pixbuf);
        key = loadable_icon_key_new (icon, size);
        g_hash_table_insert (loadable_icon_cache, key, icon_info);

        return g_object_ref (icon_info);
    } else if (G_IS_THEMED_ICON (icon)) {
        const char * const *names;
        ThemedIconKey lookup_key;
        ThemedIconKey *key;
        GtkIconTheme *icon_theme;
        GtkIconInfo *gtkicon_info;
        const char *filename;

        if (themed_icon_cache == NULL) {
            themed_icon_cache =
                g_hash_table_new_full ((GHashFunc)themed_icon_key_hash,
                                       (GEqualFunc)themed_icon_key_equal,
                                       (GDestroyNotify) themed_icon_key_free,
                                       (GDestroyNotify) g_object_unref);
        }

        names = g_themed_icon_get_names (G_THEMED_ICON (icon));

        icon_theme = gtk_icon_theme_get_default ();
        gtkicon_info = gtk_icon_theme_choose_icon (icon_theme, (const char **)names, size, 0);

        if (gtkicon_info == NULL) {
            return marlin_icon_info_new_for_pixbuf (NULL);
        }

        filename = gtk_icon_info_get_filename (gtkicon_info);
        if (filename == NULL) {
			gtk_icon_info_free (gtkicon_info);
			return marlin_icon_info_new_for_pixbuf (NULL);
		}

        lookup_key.filename = (char *)filename;
        lookup_key.size = size;

        icon_info = g_hash_table_lookup (themed_icon_cache, &lookup_key);
        if (icon_info) {
            //g_message ("CACHED %s themed icon %s\n", G_STRFUNC, filename);
            gtk_icon_info_free (gtkicon_info);
            return g_object_ref (icon_info);
        }

        icon_info = marlin_icon_info_new_for_icon_info (gtkicon_info);
        //g_critical ("%s themed icon %s size %d\n", G_STRFUNC, gtk_icon_info_get_filename (gtkicon_info), size);

        key = themed_icon_key_new (filename, size);
        g_hash_table_insert (themed_icon_cache, key, icon_info);

        gtk_icon_info_free (gtkicon_info);

        return g_object_ref (icon_info);
    } else {
        GtkIconInfo *gtk_icon_info;

        //g_message ("%s ELSE ... %s", G_STRFUNC, g_icon_to_string (icon));
        gtk_icon_info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                                        icon,
                                                        size,
                                                        GTK_ICON_LOOKUP_GENERIC_FALLBACK);
        if (gtk_icon_info != NULL) {
            pixbuf = gtk_icon_info_load_icon (gtk_icon_info, NULL);
            gtk_icon_info_free (gtk_icon_info);
        }
        icon_info = marlin_icon_info_new_for_pixbuf (pixbuf);

        /*if (pixbuf != NULL)
            g_object_unref (pixbuf);*/
        /* FIXME remove the extra ref and cache the icon_info */
        g_object_ref (icon_info);

        return icon_info;
    }
}

MarlinIconInfo *
marlin_icon_info_lookup_from_name (const char *name, int size)
{
    GIcon *icon;
    MarlinIconInfo *info;

    icon = g_themed_icon_new (name);
    info = marlin_icon_info_lookup (icon, size);
    g_object_unref (icon);

    return info;
}

MarlinIconInfo *
marlin_icon_info_lookup_from_path (const char *path, int size)
{
    GFile *icon_file;
    GIcon *icon;
    MarlinIconInfo *info;

    icon_file = g_file_new_for_path (path);
    icon = g_file_icon_new (icon_file);
    info = marlin_icon_info_lookup (icon, size);
    g_object_unref (icon);
    g_object_unref (icon_file);
    
    return info;
}

MarlinIconInfo *
marlin_icon_info_get_generic_icon (int size)
{
    MarlinIconInfo *icon;

    GIcon *generic_icon = g_themed_icon_new ("text-x-generic");
    icon = marlin_icon_info_lookup (generic_icon, size);
    g_object_unref (generic_icon);

    return icon;
}

GdkPixbuf *
marlin_icon_info_get_pixbuf_nodefault (MarlinIconInfo  *icon)
{
    GdkPixbuf *res = NULL;

    if (icon->pixbuf != NULL) 
        res = g_object_ref (icon->pixbuf);

    /*if (icon->pixbuf)
        g_warning ("%s ref count %u", G_STRFUNC, G_OBJECT (icon->pixbuf)->ref_count);*/

    return res;
}

#if 0
GdkPixbuf *
marlin_icon_info_get_pixbuf (MarlinIconInfo *icon)
{
    GdkPixbuf *res;

    res = marlin_icon_info_get_pixbuf_nodefault (icon);
    if (res == NULL) {
        res = gdk_pixbuf_new_from_data (marlin_default_file_icon,
                                        GDK_COLORSPACE_RGB,
                                        TRUE,
                                        8,
                                        marlin_default_file_icon_width,
                                        marlin_default_file_icon_height,
                                        marlin_default_file_icon_width * 4, /* stride */
                                        NULL, /* don't destroy info */
                                        NULL);
    }
    return res;
}

GdkPixbuf *
marlin_icon_info_get_pixbuf_nodefault_at_size (MarlinIconInfo  *icon,
                                                 gsize              forced_size)
{
    GdkPixbuf *pixbuf, *scaled_pixbuf;
    int w, h, s;
    double scale;

    pixbuf = marlin_icon_info_get_pixbuf_nodefault (icon);

    if (pixbuf == NULL)
        return NULL;

    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);
    s = MAX (w, h);
    if (s == forced_size) {
        return pixbuf;
    }

    scale = (double)forced_size / s;
    scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                             w * scale, h * scale,
                                             GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);
    return scaled_pixbuf;
}
#endif

GdkPixbuf *
marlin_icon_info_get_pixbuf_at_size (MarlinIconInfo *icon, gsize forced_size)
{
    GdkPixbuf *pixbuf, *scaled_pixbuf;
    int w, h, s;
    double scale;

    //pixbuf = marlin_icon_info_get_pixbuf (icon);
    pixbuf = marlin_icon_info_get_pixbuf_nodefault (icon);
    if (pixbuf == NULL)
        return NULL;

    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);
    s = MAX (w, h);
    if (s == forced_size) {
        return pixbuf;
    }

    scale = (double)forced_size / s;
    scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                             w * scale, h * scale,
                                             GDK_INTERP_BILINEAR);
    g_object_unref (pixbuf);
    return scaled_pixbuf;
}

GdkPixbuf *
marlin_icon_info_get_pixbuf_force_size (MarlinIconInfo  *icon, gint size, gboolean force_size)
{
    if (force_size) {
        return marlin_icon_info_get_pixbuf_at_size (icon, size);
    } else {
        return marlin_icon_info_get_pixbuf_nodefault (icon);
    }
}

