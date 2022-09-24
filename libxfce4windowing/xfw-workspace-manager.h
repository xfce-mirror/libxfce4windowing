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

#ifndef __XFW_WORKSPACE_MANAGER_H__
#define __XFW_WORKSPACE_MANAGER_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>
#include <gdk/gdk.h>

#include "xfw-workspace-group.h"

G_BEGIN_DECLS

#define XFW_TYPE_WORKSPACE_MANAGER           (xfw_workspace_manager_get_type())
#define XFW_WORKSPACE_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WORKSPACE_MANAGER, XfwWorkspaceManager))
#define XFW_IS_WORKSPACE_MANAGER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WORKSPACE_MANAGER))
#define XFW_WORKSPACE_MANAGER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFW_TYPE_WORKSPACE_MANAGER, XfwWorkspaceManagerIface))

typedef struct _XfwWorkspaceManager XfwWorkspaceManager;
typedef struct _XfwWorkspaceManagerIface XfwWorkspaceManagerIface;

GType xfw_workspace_manager_get_type() G_GNUC_CONST;

struct _XfwWorkspaceManagerIface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*workspace_group_created)(XfwWorkspaceManager *manager,
                                    XfwWorkspaceGroup *group);
    void (*workspace_group_destroyed)(XfwWorkspaceManager *manager,
                                      XfwWorkspaceGroup *group);

    /* Virtual Table */
    GList *(*list_workspace_groups)(XfwWorkspaceManager *);
};

GList *xfw_workspace_manager_list_workspace_groups(XfwWorkspaceManager *manager);

G_END_DECLS

#endif  /* !__XFW_WORKSPACE_MANAGER_H__ */
