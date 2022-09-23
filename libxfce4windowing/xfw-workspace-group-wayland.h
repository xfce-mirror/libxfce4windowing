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

#ifndef __XFW_WORKSPACE_GROUP_WAYLAND_H__
#define __XFW_WORKSPACE_GROUP_WAYLAND_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>

#include "xfw-workspace.h"

G_BEGIN_DECLS

#define XFW_TYPE_WORKSPACE_GROUP_WAYLAND           (xfw_workspace_group_wayland_get_type())
#define XFW_WORKSPACE_GROUP_WAYLAND(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WORKSPACE_GROUP_WAYLAND, XfwWorkspaceGroupWayland))
#define XFW_IS_WORKSPACE_GROUP_WAYLAND(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WORKSPACE_GROUP_WAYLAND))

typedef struct _XfwWorkspaceGroupWayland XfwWorkspaceGroupWayland;
typedef struct _XfwWorkspaceGroupWaylandPrivate XfwWorkspaceGroupWaylandPrivate;
typedef struct _XfwWorkspaceGroupWaylandClass XfwWorkspaceGroupWaylandClass;

struct _XfwWorkspaceGroupWayland {
    GObject parent;

    /*< private >*/
    XfwWorkspaceGroupWaylandPrivate *priv;
};

struct _XfwWorkspaceGroupWaylandClass {
    GObjectClass parent_class;

    /* Signals */
    void (*destroyed)(XfwWorkspaceGroupWayland *group);
};

GType xfw_workspace_group_wayland_get_type(void) G_GNUC_CONST;

void _xfw_workspace_group_wayland_set_active_workspace(XfwWorkspaceGroupWayland *group, XfwWorkspace *workspace);

#endif  /* __XFW_WORKSPACE_GROUP_WAYLAND_H__ */
