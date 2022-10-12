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

#include <gdk/gdk.h>

#include "libxfce4windowing-private.h"
#include "xfw-marshal.h"
#include "xfw-workspace-group.h"
#include "xfw-workspace-manager.h"

typedef struct _XfwWorkspaceGroupIface XfwWorkspaceGroupInterface;
G_DEFINE_INTERFACE(XfwWorkspaceGroup, xfw_workspace_group, G_TYPE_OBJECT)

G_DEFINE_FLAGS_TYPE(XfwWorkspaceGroupCapabilities, xfw_workspace_group_capabilities,
                    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_GROUP_CAPABILITIES_NONE, "none"),
                    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_GROUP_CAPABILITIES_CREATE_WORKSPACE, "create-workspace"),
                    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_GROUP_CAPABILITIES_MOVE_VIEWPORT, "move-viewport"),
                    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_GROUP_CAPABILITIES_SET_LAYOUT, "set-layout"))

static void
xfw_workspace_group_default_init(XfwWorkspaceGroupIface *iface) {
    g_signal_new("capabilities-changed",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, capabilities_changed),
                 NULL, NULL,
                 xfw_marshal_VOID__FLAGS_FLAGS,
                 G_TYPE_NONE, 2,
                 XFW_TYPE_WORKSPACE_GROUP_CAPABILITIES,
                 XFW_TYPE_WORKSPACE_GROUP_CAPABILITIES);
    g_signal_new("workspace-created",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, workspace_created),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE);
    g_signal_new("active-workspace-changed",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, active_workspace_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE);
    g_signal_new("workspace-destroyed",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, workspace_destroyed),
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
    g_signal_new("viewports-changed",
                 XFW_TYPE_WORKSPACE_GROUP,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceGroupIface, viewports_changed),
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
                                        g_param_spec_object("workspace-manager",
                                                            "workspace-manager",
                                                            "workspace-manager",
                                                            XFW_TYPE_WORKSPACE_MANAGER,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("workspaces",
                                                             "workspaces",
                                                             "workspaces",
                                                             G_PARAM_READABLE));
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

XfwWorkspaceGroupCapabilities
xfw_workspace_group_get_capabilities(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), XFW_WORKSPACE_GROUP_CAPABILITIES_NONE);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->get_capabilities)(group);
}

guint
xfw_workspace_group_get_workspace_count(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), 0);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->get_workspace_count)(group);
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

XfwWorkspaceManager *
xfw_workspace_group_get_workspace_manager(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), NULL);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->get_workspace_manager)(group);
}

gboolean
xfw_workspace_group_create_workspace(XfwWorkspaceGroup *group, const gchar *name, GError **error) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), FALSE);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->create_workspace)(group, name, error);
}

gboolean
xfw_workspace_group_move_viewport(XfwWorkspaceGroup *group, gint x, gint y, GError **error) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), FALSE);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->move_viewport)(group, x, y, error);
}

gboolean
xfw_workspace_group_set_layout(XfwWorkspaceGroup *group, gint rows, gint columns, GError **error) {
    XfwWorkspaceGroupIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_GROUP(group), FALSE);
    iface = XFW_WORKSPACE_GROUP_GET_IFACE(group);
    return (*iface->move_viewport)(group, rows, columns, error);
}

void
_xfw_workspace_group_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_SCREEN, "screen");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_WORKSPACE_MANAGER, "workspace-manager");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_WORKSPACES, "workspaces");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_ACTIVE_WORKSPACE, "active-workspace");
    g_object_class_override_property(gklass, WORKSPACE_GROUP_PROP_MONITORS, "monitors");
}
