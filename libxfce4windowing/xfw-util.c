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

/**
 * SECTION:xfw-util
 * @title: Utilities
 * @short_description: Miscellaneous windowing utilities
 * @stability: Unstable
 * @include: libxfce4windowing/libxfce4windowing.h
 **/

#include "config.h"

#include <stdlib.h>

#include <gdk/gdk.h>
#ifdef ENABLE_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#endif

#include "libxfce4windowing-private.h"
#include "xfw-util.h"

#define WINDOWING_UNKNOWN 0

G_DEFINE_ENUM_TYPE(XfwDirection, xfw_direction,
                   G_DEFINE_ENUM_VALUE(XFW_DIRECTION_UP, "up"),
                   G_DEFINE_ENUM_VALUE(XFW_DIRECTION_DOWN, "down"),
                   G_DEFINE_ENUM_VALUE(XFW_DIRECTION_LEFT, "left"),
                   G_DEFINE_ENUM_VALUE(XFW_DIRECTION_RIGHT, "right"));

/**
 * xfw_windowing_get:
 *
 * Determines the windowing environment that is currently active.
 *
 * Return value: A value from the #XfwWindowing enum.
 **/
XfwWindowing
xfw_windowing_get(void)
{
    static XfwWindowing windowing = WINDOWING_UNKNOWN;

    if (G_UNLIKELY(windowing == WINDOWING_UNKNOWN)) {
        GdkDisplay *gdpy = gdk_display_get_default();

        _libxfce4windowing_init();

#ifdef ENABLE_X11
        if (GDK_IS_X11_DISPLAY(gdpy)) {
            windowing = XFW_WINDOWING_X11;
        } else
#endif  /* ENABLE_X11 */
#ifdef ENABLE_WAYLAND
        if (GDK_IS_WAYLAND_DISPLAY(gdpy)) {
            windowing = XFW_WINDOWING_WAYLAND;
        } else
#endif  /* ENABLE_WAYLAND */
        {
            g_critical("Unknown/unsupported GDK windowing type");
            exit(1);
        }
    }

    return windowing;
}

GQuark
xfw_error_quark(void) {
    static GQuark quark = 0;
    if (quark == 0) {
        quark = g_quark_from_static_string("xfw-error-quark");
    }
    return quark;
}

GDesktopAppInfo *
xfw_g_desktop_app_info_get(const gchar *app_id) {
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
            g_strfreev (*p);
        }
        g_free (desktop_ids);
    }

    return app_info;
}
