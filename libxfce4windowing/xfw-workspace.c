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

#include <limits.h>

#include "libxfce4windowing-private.h"
#include "xfw-workspace.h"

enum {
    NAME_CHANGED,
    STATE_CHANGED,

    LAST_SIGNAL
};

static guint workspace_signals[LAST_SIGNAL] = { 0, };

typedef struct _XfwWorkspaceIface XfwWorkspaceInterface;
G_DEFINE_INTERFACE(XfwWorkspace, xfw_workspace, G_TYPE_OBJECT)

G_DEFINE_FLAGS_TYPE(XfwWorkspaceState, xfw_workspace_state,
    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_STATE_NONE, "none"),
    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_STATE_ACTIVE, "active"),
    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_STATE_URGENT, "urgent"),
    G_DEFINE_ENUM_VALUE(XFW_WORKSPACE_STATE_HIDDEN, "hidden"))

static void
xfw_workspace_default_init(XfwWorkspaceIface *iface) {
    workspace_signals[NAME_CHANGED] = g_signal_new("name-changed",
                                                   XFW_TYPE_WORKSPACE,
                                                   G_SIGNAL_RUN_LAST,
                                                   G_STRUCT_OFFSET(XfwWorkspaceIface, name_changed),
                                                   NULL, NULL,
                                                   g_cclosure_marshal_VOID__VOID,
                                                   G_TYPE_NONE, 0);
    workspace_signals[STATE_CHANGED] = g_signal_new("state-changed",
                                                    XFW_TYPE_WORKSPACE,
                                                    G_SIGNAL_RUN_LAST,
                                                    G_STRUCT_OFFSET(XfwWorkspaceIface, state_changed),
                                                    NULL, NULL,
                                                    g_cclosure_marshal_VOID__FLAGS,
                                                    G_TYPE_NONE, 1,
                                                    XFW_TYPE_WORKSPACE_STATE);

    g_object_interface_install_property(iface,
                                        g_param_spec_string("id",
                                                            "id",
                                                            "id",
                                                            "",
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_string("name",
                                                            "name",
                                                            "name",
                                                            "",
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
    g_object_interface_install_property(iface,
                                        g_param_spec_flags("state",
                                                          "state",
                                                          "state",
                                                          XFW_TYPE_WORKSPACE_STATE,
                                                          XFW_WORKSPACE_STATE_NONE,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
    g_object_interface_install_property(iface,
                                        g_param_spec_uint("number",
                                                          "number",
                                                          "number",
                                                          0, UINT_MAX, 0,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

const gchar *
xfw_workspace_get_id(XfwWorkspace *workspace) {
    XfwWorkspaceIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE(workspace), NULL);
    iface = XFW_WORKSPACE_GET_IFACE(workspace);
    return (*iface->get_id)(workspace);
}

const gchar *
xfw_workspace_get_name(XfwWorkspace *workspace) {
    XfwWorkspaceIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE(workspace), NULL);
    iface = XFW_WORKSPACE_GET_IFACE(workspace);
    return (*iface->get_name)(workspace);
}

XfwWorkspaceState
xfw_workspace_get_state(XfwWorkspace *workspace) {
    XfwWorkspaceIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE(workspace), XFW_WORKSPACE_STATE_NONE);
    iface = XFW_WORKSPACE_GET_IFACE(workspace);
    return (*iface->get_state)(workspace);
}

guint
xfw_workspace_get_number(XfwWorkspace *workspace) {
    XfwWorkspaceIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE(workspace), 0);
    iface = XFW_WORKSPACE_GET_IFACE(workspace);
    return (*iface->get_number)(workspace);
}

void
xfw_workspace_activate(XfwWorkspace *workspace, GError **error) {
    XfwWorkspaceIface *iface;
    g_return_if_fail(XFW_IS_WORKSPACE(workspace));
    iface = XFW_WORKSPACE_GET_IFACE(workspace);
    return (*iface->activate)(workspace, error);
}

void
xfw_workspace_remove(XfwWorkspace *workspace, GError **error) {
    XfwWorkspaceIface *iface;
    g_return_if_fail(XFW_IS_WORKSPACE(workspace));
    iface = XFW_WORKSPACE_GET_IFACE(workspace);
    return (*iface->remove)(workspace, error);
}

void
_xfw_workspace_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, WORKSPACE_PROP_ID, "id");
    g_object_class_override_property(gklass, WORKSPACE_PROP_NAME, "name");
    g_object_class_override_property(gklass, WORKSPACE_PROP_STATE, "state");
    g_object_class_override_property(gklass, WORKSPACE_PROP_NUMBER, "number");
}
