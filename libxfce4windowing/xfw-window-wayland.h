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

#ifndef __XFW_WINDOW_WAYLAND_H__
#define __XFW_WINDOW_WAYLAND_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

G_BEGIN_DECLS

#define XFW_TYPE_WINDOW_WAYLAND           (xfw_window_wayland_get_type())
#define XFW_WINDOW_WAYLAND(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WINDOW_WAYLAND, XfwWindowWayland))
#define XFW_IS_WINDOW_WAYLAND(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WINDOW_WAYLAND))

typedef struct _XfwWindowWayland XfwWindowWayland;
typedef struct _XfwWindowWaylandPrivate XfwWindowWaylandPrivate;
typedef struct _XfwWindowWaylandClass XfwWindowWaylandClass;

struct _XfwWindowWayland {
    GObject parent;
    /*< private >*/
    XfwWindowWaylandPrivate *priv;
};

struct _XfwWindowWaylandClass {
    GObjectClass parent_class;
};

GType xfw_window_wayland_get_type(void) G_GNUC_CONST;

struct zwlr_foreign_toplevel_handle_v1 *_xfw_window_wayland_get_handle(XfwWindowWayland *window);
const gchar *_xfw_window_wayland_get_app_id(XfwWindowWayland *window);

#endif  /* __XFW_WINDOW_WAYLAND_H__ */
