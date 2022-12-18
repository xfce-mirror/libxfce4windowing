/*
 * Copyright (c) 2022 Brian Tarricone <brian@tarricone.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#ifdef ENABLE_X11
#include <libwnck/libwnck.h>
#endif

#include "libxfce4windowing-private.h"

static gboolean inited = FALSE;
#ifdef ENABLE_X11
static gint wnck_default_icon_size = WNCK_DEFAULT_ICON_SIZE;
#endif

void
_libxfce4windowing_init(void) {
    if (!inited) {
        inited = TRUE;
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    }
}

#ifdef ENABLE_X11
GdkPixbuf *
_xfw_wnck_object_get_icon(GObject *wnck_object, const gchar *icon_name, gint size, gint scale, XfwGetIconFunc get_icon, XfwGetIconFunc get_mini_icon) {
    GdkPixbuf *icon = NULL;

    g_return_val_if_fail(WNCK_IS_WINDOW(wnck_object) || WNCK_IS_CLASS_GROUP(wnck_object), NULL);

    if (size * scale > wnck_default_icon_size) {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        wnck_set_default_icon_size(size * scale);
G_GNUC_END_IGNORE_DEPRECATIONS
        wnck_default_icon_size = size * scale;
    }

    if (icon_name != NULL) {
        icon = gtk_icon_theme_load_icon_for_scale(gtk_icon_theme_get_default(),
                                                  icon_name,
                                                  size,
                                                  scale,
                                                  GTK_ICON_LOOKUP_FORCE_SIZE,
                                                  NULL);
    }

    if (icon == NULL) {
        icon = get_icon(wnck_object);
        if (icon != NULL) {
            g_object_ref(icon);
        }
    }

    if (icon == NULL) {
        icon = get_mini_icon(wnck_object);
        if (icon != NULL) {
            g_object_ref(icon);
        }
    }

    if (icon != NULL) {
        gint full_size = size * scale;
        gint width = gdk_pixbuf_get_width(icon);
        gint height = gdk_pixbuf_get_height(icon);

        if (width > full_size || height > full_size || (width < full_size && height < full_size)) {
            GdkPixbuf *icon_scaled;
            gdouble aspect = (gdouble)width / (gdouble)height;
            gint new_width, new_height;

            if (width == height) {
                new_width = new_height = full_size;
            } else if (width > height) {
                new_width = full_size;
                new_height = full_size / aspect;
            } else {
                new_width = full_size / aspect;
                new_height = full_size;
            }

            icon_scaled = gdk_pixbuf_scale_simple(icon, new_width, new_height, GDK_INTERP_BILINEAR);
            g_object_unref(icon);
            icon = icon_scaled;
        }
    }

    return icon;
}
#endif

/**
 * _xfw_g_desktop_app_info_get:
 * @app_id: an application ID
 *
 * Attempts to find a #GDesktopAppInfo instance for the provided application
 * ID.
 *
 * Return value: (nullable) (transfer full): a #GDesktopAppInfo instance,
 * with the reference owned by the caller, or %NULL.
 **/
GDesktopAppInfo *
_xfw_g_desktop_app_info_get(const gchar *app_id) {
    GDesktopAppInfo *app_info;
    gchar *desktop_id;

    desktop_id = g_strdup_printf("%s.desktop", app_id);
    app_info = g_desktop_app_info_new(desktop_id);
    g_free(desktop_id);
    if (app_info == NULL) {
        gchar ***desktop_ids = g_desktop_app_info_search(app_id);
        if (desktop_ids[0] != NULL) {
            app_info = g_desktop_app_info_new(desktop_ids[0][0]);
        }
        for (gchar ***p = desktop_ids; *p != NULL; p++) {
            g_strfreev(*p);
        }
        g_free(desktop_ids);
    }

    return app_info;
}
