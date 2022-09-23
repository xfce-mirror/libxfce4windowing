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

#ifndef __XFW_UTIL_H__
#define __XFW_UTIL_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#define XFW_ERROR (xfw_error_quark())

typedef enum _XfwErrorCode {
    XFW_ERROR_UNSUPPORTED = 0,
} XfwErrorCode;

G_BEGIN_DECLS

typedef enum {
    XFW_WINDOWING_X11 = 1,
    XFW_WINDOWING_WAYLAND = 2
} XfwWindowing;

GQuark xfw_error_quark(void);

XfwWindowing xfw_windowing_get(void);

static inline void
xfw_windowing_error_trap_push(GdkDisplay *display) {
#ifdef GDK_WINDOWING_X11
    if (xfw_windowing_get() == XFW_WINDOWING_X11) {
        gdk_x11_display_error_trap_push(display);
    }
#endif  /* GDK_WINDOWING_X11 */
}

static inline gint
xfw_windowing_error_trap_pop(GdkDisplay *display) {
#ifdef GDK_WINDOWING_X11
    if (xfw_windowing_get() == XFW_WINDOWING_X11) {
        return gdk_x11_display_error_trap_pop(display);
    } else
#endif  /* GDK_WINDOWING_X11 */
    {
        return 0;
    }
}

static inline void
xfw_windowing_error_trap_pop_ignored(GdkDisplay *display) {
#ifdef GDK_WINDOWING_X11
    if (xfw_windowing_get() == XFW_WINDOWING_X11) {
        gdk_x11_display_error_trap_pop_ignored(display);
    }
#endif  /* GDK_WINDOWING_X11 */
}

G_END_DECLS

#endif  /* __XFW_UTIL_H__ */
