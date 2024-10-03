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

#ifndef __XFW_SCREEN_WAYLAND_H__
#define __XFW_SCREEN_WAYLAND_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>
#include <wayland-client.h>

#include "xfw-screen-private.h"
#include "xfw-seat-wayland.h"
#include "xfw-window.h"
#include "xfw-workspace.h"

G_BEGIN_DECLS

#define XFW_TYPE_SCREEN_WAYLAND (xfw_screen_wayland_get_type())
G_DECLARE_FINAL_TYPE(XfwScreenWayland, xfw_screen_wayland, XFW, SCREEN_WAYLAND, XfwScreen)

typedef struct _XfwScreenWaylandPrivate XfwScreenWaylandPrivate;

GdkScreen *_xfw_screen_wayland_get_gdk_screen(XfwScreenWayland *screen);

void _xfw_screen_wayland_seat_ready(XfwScreenWayland *screen, XfwSeatWayland *seat);

XfwWorkspace *_xfw_screen_wayland_get_window_workspace(XfwScreenWayland *screen, XfwWindow *window);

G_END_DECLS

#endif /* __XFW_SCREEN_WAYLAND_H__ */
