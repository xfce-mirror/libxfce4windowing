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

#include <gdk/gdk.h>

#include "libxfce4windowing-private.h"
#include "xfw-workspace-group.h"

typedef struct _XfwWorkspaceGroupIface XfwWorkspaceGroupInterface;
G_DEFINE_INTERFACE(XfwWorkspaceGroup, xfw_workspace_group, G_TYPE_OBJECT)

static void
xfw_workspace_group_default_init(XfwWorkspaceGroupIface *iface) {
    g_signal_new("workspace-added",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, workspace_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE);
    g_signal_new("workspace-activated",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, workspace_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE);
    g_signal_new("workspace-removed",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, workspace_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE);
    g_signal_new("monitors-changed",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, monitors_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    g_object_interface_install_property(iface,
                                        g_param_spec_object("screen",
                                                            "screen",
                                                            "screen",
                                                            GDK_TYPE_SCREEN,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("workspaces",
                                                             "workspaces",
                                                             "workspaces",
                                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_interface_install_property(iface,
                                        g_param_spec_object("active-workspace",
                                                            "active-workspace",
                                                            "active-workspace",
                                                            XFW_TYPE_WORKSPACE,
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("monitors",
                                                             "monitors",
                                                             "monitors",
                                                             G_PARAM_READABLE));
}

GList *
xfw_workspace_group_list_workspaces(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), NULL);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->list_workspaces)(group);
}

XfwWorkspace *
xfw_workspace_group_get_active_workspace(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), NULL);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->get_active_workspace)(group);
}

GList *
xfw_workspace_group_get_monitors(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), NULL);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->get_monitors)(group);
}

void
_xfw_workspace_group_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_SCREEN, "screen");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_WORKSPACES, "workspaces");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_ACTIVE_WORKSPACE, "active-workspace");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_MONITORS, "monitors");
}
