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
#include "xfw-workspace-group-wayland.h"
#include "xfw-workspace-wayland.h"
#include "xfw-workspace.h"

struct _XfwWorkspaceWaylandPrivate {
    XfwWorkspaceGroupWayland *group;
    struct ext_workspace_handle_v1 *handle;
    gchar *id;
    gchar *name;
    XfwWorkspaceState state;
    guint number;
};

enum {
    SIGNAL_DESTROYED,

    N_SIGNALS,
};

enum {
    PROP0,
    PROP_GROUP,
    PROP_HANDLE,
};

static guint workspace_signals[N_SIGNALS] = { 0, };

static void xfw_workspace_wayland_workspace_init(XfwWorkspaceIface *iface);
static void xfw_workspace_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_wayland_dispose(GObject *obj);
static const gchar *xfw_workspace_wayland_get_id(XfwWorkspace *workspace);
static const gchar *xfw_workspace_wayland_get_name(XfwWorkspace *workspace);
static XfwWorkspaceState xfw_workspace_wayland_get_state(XfwWorkspace *workspace);
static guint xfw_workspace_wayland_get_number(XfwWorkspace *workspace);
static void xfw_workspace_wayland_activate(XfwWorkspace *workspace, GError **error);
static void xfw_workspace_wayland_remove(XfwWorkspace *workspace, GError **error);

static void workspace_name(void *data, struct ext_workspace_handle_v1 *workspace, const char *name);
static void workspace_coordinates(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *coordinates);
static void workspace_state(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *state);
static void workspace_capabilities(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *capabilities);
static void workspace_removed(void *data, struct ext_workspace_handle_v1 *workspace);

static const struct ext_workspace_handle_v1_listener workspace_listener = {
    .name = workspace_name,
    .coordinates = workspace_coordinates,
    .state = workspace_state,
    .capabilities = workspace_capabilities,
    .removed = workspace_removed,
};

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceWayland, xfw_workspace_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE,
                                              xfw_workspace_wayland_workspace_init))

static void
xfw_workspace_wayland_class_init(XfwWorkspaceWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->set_property = xfw_workspace_wayland_set_property;
    gklass->get_property = xfw_workspace_wayland_get_property;
    gklass->dispose = xfw_workspace_wayland_dispose;

    workspace_signals[SIGNAL_DESTROYED] = g_signal_new("destroyed",
                                                       XFW_TYPE_WORKSPACE_WAYLAND,
                                                       G_SIGNAL_RUN_LAST,
                                                       G_STRUCT_OFFSET(XfwWorkspaceWaylandClass, destroyed),
                                                       NULL, NULL,
                                                       g_cclosure_marshal_VOID__VOID,
                                                       G_TYPE_NONE, 0);

    g_object_class_install_property(gklass,
                                    PROP_GROUP,
                                    g_param_spec_object("group",
                                                        "group",
                                                        "group",
                                                        XFW_TYPE_WORKSPACE_GROUP_WAYLAND,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(gklass,
                                    PROP_HANDLE,
                                    g_param_spec_pointer("handle",
                                                         "handle",
                                                         "handle",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_workspace_install_properties(gklass);
}

static void
xfw_workspace_wayland_init(XfwWorkspaceWayland *workspace) {
    workspace->priv->id = g_strdup_printf("%u", wl_proxy_get_id((struct wl_proxy *)workspace->priv->handle));
    ext_workspace_handle_v1_add_listener(workspace->priv->handle, &workspace_listener, workspace);
}

static void
xfw_workspace_wayland_workspace_init(XfwWorkspaceIface *iface) {
    iface->get_id = xfw_workspace_wayland_get_id;
    iface->get_name = xfw_workspace_wayland_get_name;
    iface->get_state = xfw_workspace_wayland_get_state;
    iface->get_number = xfw_workspace_wayland_get_number;
    iface->activate = xfw_workspace_wayland_activate;
    iface->remove = xfw_workspace_wayland_remove;
}

static void
xfw_workspace_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(obj);
    switch (prop_id) {
        case PROP_GROUP:
            workspace->priv->group = g_value_get_object(value);
            break;

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
            workspace->priv->state = g_value_get_flags(value);
            break;

        case WORKSPACE_PROP_NUMBER:
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
        case PROP_GROUP:
            g_value_set_object(value, workspace->priv->group);
            break;

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
            g_value_set_flags(value, workspace->priv->state);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_wayland_dispose(GObject *obj) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(obj);
    ext_workspace_handle_v1_destroy(workspace->priv->handle);
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

static guint
xfw_workspace_wayland_get_number(XfwWorkspace *workspace) {
    return XFW_WORKSPACE_WAYLAND(workspace)->priv->number;
}

static void
xfw_workspace_wayland_activate(XfwWorkspace *workspace, GError **error) {
    ext_workspace_handle_v1_activate(XFW_WORKSPACE_WAYLAND(workspace)->priv->handle);
}

static void
xfw_workspace_wayland_remove(XfwWorkspace *workspace, GError **error) {
    ext_workspace_handle_v1_remove(XFW_WORKSPACE_WAYLAND(workspace)->priv->handle);
}

static void
workspace_name(void *data, struct ext_workspace_handle_v1 *wl_workspace, const char *name) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(data);
    g_object_set(workspace, "name", name, NULL);
    g_signal_emit_by_name(workspace, "name-changed");
}

static void
workspace_coordinates(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *coordinates)
{

}

static void
workspace_state(void *data, struct ext_workspace_handle_v1 *wl_workspace, struct wl_array *wl_state) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(data);
    XfwWorkspaceState old_state = workspace->priv->state;
    XfwWorkspaceState state = XFW_WORKSPACE_STATE_NONE;
    enum ext_workspace_handle_v1_state *item;

    wl_array_for_each(item, wl_state) {
        switch (*item) {
            case EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE:
                state |= XFW_WORKSPACE_STATE_ACTIVE;
                break;
            case EXT_WORKSPACE_HANDLE_V1_STATE_URGENT:
                state |= XFW_WORKSPACE_STATE_URGENT;
                break;
            case EXT_WORKSPACE_HANDLE_V1_STATE_HIDDEN:
                state |= XFW_WORKSPACE_STATE_HIDDEN;
                break;
            default:
                g_warning("Unrecognized workspace state %d", *item);
                break;
        }
    }
    g_object_set(workspace, "state", state, NULL);
    g_signal_emit_by_name(workspace, "state-changed", old_state);
    if ((old_state & XFW_WORKSPACE_STATE_ACTIVE) == 0 && (state & XFW_WORKSPACE_STATE_ACTIVE) != 0) {
        _xfw_workspace_group_wayland_set_active_workspace(workspace->priv->group, XFW_WORKSPACE(workspace));
    }
}

static void
workspace_capabilities(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *capabilities) {

}

static void
workspace_removed(void *data, struct ext_workspace_handle_v1 *wl_workspace) {
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(data);
    g_signal_emit(workspace, workspace_signals[SIGNAL_DESTROYED], 0);
}

void
_xfw_workspace_wayland_set_number(XfwWorkspaceWayland *workspace, guint number) {
    if (number != workspace->priv->number) {
        workspace->priv->number = number;
        g_object_notify(G_OBJECT(workspace), "number");
    }
}
