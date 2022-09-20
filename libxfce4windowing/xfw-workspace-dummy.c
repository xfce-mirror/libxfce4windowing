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

#include <glib/gi18n-lib.h>

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-workspace-dummy.h"
#include "xfw-workspace.h"

static void xfw_workspace_dummy_workspace_init(XfwWorkspaceIface *iface);
static void xfw_workspace_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static const gchar *xfw_workspace_dummy_get_id(XfwWorkspace *workspace);
static const gchar *xfw_workspace_dummy_get_name(XfwWorkspace *workspace);
static XfwWorkspaceState xfw_workspace_dummy_get_state(XfwWorkspace *workspace);
static void xfw_workspace_dummy_activate(XfwWorkspace *workspace, GError **error);
static void xfw_workspace_dummy_remove(XfwWorkspace *workspace, GError **error);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceDummy, xfw_workspace_dummy, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE,
                                              xfw_workspace_dummy_workspace_init))

static void
xfw_workspace_dummy_class_init(XfwWorkspaceDummyClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    gklass->set_property = xfw_workspace_dummy_set_property;
    gklass->get_property = xfw_workspace_dummy_get_property;
}

static void
xfw_workspace_dummy_init(XfwWorkspaceDummy *workspace) {

}

static void
xfw_workspace_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    switch (prop_id) {
        case WORKSPACE_PROP_ID:
        case WORKSPACE_PROP_NAME:
        case WORKSPACE_PROP_STATE:
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspace *workspace = XFW_WORKSPACE(obj);
    switch (prop_id) {
        case WORKSPACE_PROP_ID:
            g_value_set_string(value, xfw_workspace_dummy_get_id(workspace));
            break;

        case WORKSPACE_PROP_NAME:
            g_value_set_string(value, xfw_workspace_dummy_get_name(workspace));
            break;

        case WORKSPACE_PROP_STATE:
            g_value_set_uint(value, xfw_workspace_get_state(workspace));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_dummy_workspace_init(XfwWorkspaceIface *iface) {
    iface->get_id = xfw_workspace_dummy_get_id;
    iface->get_name = xfw_workspace_dummy_get_name;
    iface->get_state = xfw_workspace_dummy_get_state;
    iface->activate = xfw_workspace_dummy_activate;
    iface->remove = xfw_workspace_dummy_remove;
}

static const gchar *
xfw_workspace_dummy_get_id(XfwWorkspace *workspace) {
    return "0";
}

static const gchar *
xfw_workspace_dummy_get_name(XfwWorkspace *workspace) {
    return _("Workspace 0");
}

static XfwWorkspaceState
xfw_workspace_dummy_get_state(XfwWorkspace *workspace) {
    return XFW_WORKSPACE_STATE_ACTIVE;
}

static void
xfw_workspace_dummy_activate(XfwWorkspace *workspace, GError **error) {

}

static void
xfw_workspace_dummy_remove(XfwWorkspace *workspace, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, 0, "Cannot remove workspace as it is the only one left");
    }
}
