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
#include "protocols/ext-workspace-v1-20220919-client.h"
#include "xfw-util.h"
#include "xfw-workspace-wayland.h"
#include "xfw-workspace.h"

struct _XfwWorkspaceWaylandPrivate {
    struct ext_workspace_handle_v1 *handle;
    gchar *id;
    gchar *name;
    XfwWorkspaceState state;
};

enum {
    PROP0,
    PROP_HANDLE,
};

static void xfw_workspace_wayland_workspace_init(XfwWorkspaceIface *iface);
static void xfw_workspace_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_wayland_dispose(GObject *obj);
static const gchar *xfw_workspace_wayland_get_id(XfwWorkspace *workspace);
static const gchar *xfw_workspace_wayland_get_name(XfwWorkspace *workspace);
static XfwWorkspaceState xfw_workspace_wayland_get_state(XfwWorkspace *workspace);
static void xfw_workspace_wayland_activate(XfwWorkspace *workspace, GError **error);
static void xfw_workspace_wayland_remove(XfwWorkspace *workspace, GError **error);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceWayland, xfw_workspace_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE,
                                              xfw_workspace_wayland_workspace_init))

static void
xfw_workspace_wayland_class_init(XfwWorkspaceWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    g_object_class_install_property(gklass,
                                    PROP_HANDLE,
                                    g_param_spec_pointer("handle",
                                                         "handle",
                                                         "handle",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_workspace_install_properties(gklass);

    gklass->set_property = xfw_workspace_wayland_set_property;
    gklass->get_property = xfw_workspace_wayland_get_property;
    gklass->dispose = xfw_workspace_wayland_dispose;
}

static void
xfw_workspace_wayland_init(XfwWorkspaceWayland *workspace) {
    workspace->priv->id = g_strdup_printf("%u", wl_proxy_get_id((struct wl_proxy *)workspace->priv->handle));
}

static void
xfw_workspace_wayland_workspace_init(XfwWorkspaceIface *iface) {
    iface->get_id = xfw_workspace_wayland_get_id;
    iface->get_name = xfw_workspace_wayland_get_name;
    iface->get_state = xfw_workspace_wayland_get_state;
    iface->activate = xfw_workspace_wayland_activate;
    iface->remove = xfw_workspace_wayland_remove;
}

static void
xfw_workspace_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(obj);
    switch (prop_id) {
        case PROP_HANDLE:
            workspace->priv->handle = g_value_get_pointer(value);
            break;

        case WORKSPACE_PROP_ID:
            break;

        case WORKSPACE_PROP_NAME:
            g_free(workspace->priv->name);
            workspace->priv->name = g_value_dup_string(value);
            break;

        case WORKSPACE_PROP_STATE:
            workspace->priv->state = g_value_get_uint(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(obj);
    switch (prop_id) {
        case PROP_HANDLE:
            g_value_set_pointer(value, workspace->priv->handle);
            break;

        case WORKSPACE_PROP_ID:
            g_value_set_string(value, workspace->priv->name);
            break;

        case WORKSPACE_PROP_NAME:
            g_value_set_string(value, workspace->priv->name);
            break;

        case WORKSPACE_PROP_STATE:
            g_value_set_uint(value, workspace->priv->state);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_wayland_dispose(GObject *obj) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(obj);
    g_free(workspace->priv->id);
    g_free(workspace->priv->name);
}

static const gchar *
xfw_workspace_wayland_get_id(XfwWorkspace *workspace) {
    return XFW_WORKSPACE_WAYLAND(workspace)->priv->id;
}

static const gchar *
xfw_workspace_wayland_get_name(XfwWorkspace *workspace) {
    return XFW_WORKSPACE_WAYLAND(workspace)->priv->name;
}

static XfwWorkspaceState
xfw_workspace_wayland_get_state(XfwWorkspace *workspace) {
    return XFW_WORKSPACE_WAYLAND(workspace)->priv->state;
}

static void
xfw_workspace_wayland_activate(XfwWorkspace *workspace, GError **error) {
    ext_workspace_handle_v1_activate(XFW_WORKSPACE_WAYLAND(workspace)->priv->handle);
}

static void
xfw_workspace_wayland_remove(XfwWorkspace *workspace, GError **error) {
    ext_workspace_handle_v1_remove(XFW_WORKSPACE_WAYLAND(workspace)->priv->handle);
}
