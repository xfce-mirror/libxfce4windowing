/*
 * Copyright (c) 2022 Brian Tarricone <brian@tarricone.org>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

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
