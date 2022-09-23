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

#ifndef __XFW_WORKSPACE_GROUP_H__
#define __XFW_WORKSPACE_GROUP_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

#include "xfw-workspace.h"

G_BEGIN_DECLS

#define XFW_TYPE_WORKSPACE_GROUP           (xfw_workspace_group_get_type())
#define XFW_WORKSPACE_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WORKSPACE_GROUP, XfwWorkspaceGroup))
#define XFW_IS_WORKSPACE_GROUP(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WORKSPACE_GROUP))
#define XFW_WORKSPACE_GROUP_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFW_TYPE_WORKSPACE_GROUP, XfwWorkspaceGroupIface))

typedef struct _XfwWorkspaceGroup XfwWorkspaceGroup;
typedef struct _XfwWorkspaceGroupIface XfwWorkspaceGroupIface;

GType xfw_workspace_group_get_type() G_GNUC_CONST;

struct _XfwWorkspaceGroupIface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*workspace_created)(XfwWorkspaceGroup *group,
                              XfwWorkspace *workspace);
    void (*active_workspace_changed)(XfwWorkspaceGroup *group,
                                     XfwWorkspace *previously_active_workspace);
    void (*workspace_destroyed)(XfwWorkspaceGroup *group,
                                XfwWorkspace *workspace);
    void (*monitors_changed)(XfwWorkspaceGroup *group);

    /* Virtual Table */
    guint (*get_workspace_count)(XfwWorkspaceGroup *group);
    GList *(*list_workspaces)(XfwWorkspaceGroup *group);
    XfwWorkspace *(*get_active_workspace)(XfwWorkspaceGroup *group);
    GList *(*get_monitors)(XfwWorkspaceGroup *group);
};

guint xfw_workspace_group_get_workspace_count(XfwWorkspaceGroup *group);
GList *xfw_workspace_group_list_workspaces(XfwWorkspaceGroup *group);
XfwWorkspace *xfw_workspace_group_get_active_workspace(XfwWorkspaceGroup *group);
GList *xfw_workspace_group_get_monitors(XfwWorkspaceGroup *group);

G_END_DECLS

#endif  /* !__XFW_WORKSPACE_GROUP_H__ */
