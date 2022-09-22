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

#include <libwnck/libwnck.h>

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-workspace-x11.h"
#include "xfw-workspace.h"

struct _XfwWorkspaceX11Private {
    WnckWorkspace *wnck_workspace;
};

static void xfw_workspace_x11_workspace_init(XfwWorkspaceIface *iface);
static void xfw_workspace_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_x11_dispose(GObject *obj);
static const gchar *xfw_workspace_x11_get_id(XfwWorkspace *workspace);
static const gchar *xfw_workspace_x11_get_name(XfwWorkspace *workspace);
static XfwWorkspaceState xfw_workspace_x11_get_state(XfwWorkspace *workspace);
static guint xfw_workspace_x11_get_number(XfwWorkspace *workspace);
static void xfw_workspace_x11_activate(XfwWorkspace *workspace, GError **error);
static void xfw_workspace_x11_remove(XfwWorkspace *workspace, GError **error);

static void name_changed(WnckWorkspace *wnck_workspace, XfwWorkspaceX11 *workspace);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceX11, xfw_workspace_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE,
                                              xfw_workspace_x11_workspace_init))

static void
xfw_workspace_x11_class_init(XfwWorkspaceX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    gklass->set_property = xfw_workspace_x11_set_property;
    gklass->get_property = xfw_workspace_x11_get_property;
    gklass->dispose = xfw_workspace_x11_dispose;
    _xfw_workspace_install_properties(gklass);
}

static void
xfw_workspace_x11_init(XfwWorkspaceX11 *workspace) {
    g_signal_connect(workspace->priv->wnck_workspace, "name-changed", (GCallback)name_changed, workspace);
}

static void
xfw_workspace_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    switch (prop_id) {
        case WORKSPACE_PROP_ID:
        case WORKSPACE_PROP_NAME:
        case WORKSPACE_PROP_STATE:
        case WORKSPACE_PROP_NUMBER:
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspace *workspace = XFW_WORKSPACE(obj);
    switch (prop_id) {
        case WORKSPACE_PROP_ID:
            g_value_set_string(value, xfw_workspace_x11_get_id(workspace));
            break;

        case WORKSPACE_PROP_NAME:
            g_value_set_string(value, xfw_workspace_x11_get_name(workspace));
            break;

        case WORKSPACE_PROP_STATE:
            g_value_set_uint(value, xfw_workspace_x11_get_state(workspace));
            break;

        case WORKSPACE_PROP_NUMBER:
            g_value_set_uint(value, xfw_workspace_x11_get_number(workspace));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_x11_dispose(GObject *obj) {
    XfwWorkspaceX11 *workspace = XFW_WORKSPACE_X11(obj);
    g_signal_handlers_disconnect_by_func(workspace->priv->wnck_workspace, name_changed, workspace);
}

static void
xfw_workspace_x11_workspace_init(XfwWorkspaceIface *iface) {
    iface->get_id = xfw_workspace_x11_get_id;
    iface->get_name = xfw_workspace_x11_get_name;
    iface->get_state = xfw_workspace_x11_get_state;
    iface->get_number = xfw_workspace_x11_get_number;
    iface->activate = xfw_workspace_x11_activate;
    iface->remove = xfw_workspace_x11_remove;
}

static const gchar *
xfw_workspace_x11_get_id(XfwWorkspace *workspace) {
    return wnck_workspace_get_name(XFW_WORKSPACE_X11(workspace)->priv->wnck_workspace);
}

static const gchar *
xfw_workspace_x11_get_name(XfwWorkspace *workspace) {
    return wnck_workspace_get_name(XFW_WORKSPACE_X11(workspace)->priv->wnck_workspace);
}

static XfwWorkspaceState
xfw_workspace_x11_get_state(XfwWorkspace *workspace) {
    XfwWorkspaceX11Private *priv = XFW_WORKSPACE_X11(workspace)->priv;
    XfwWorkspaceState state = XFW_WORKSPACE_STATE_NONE;
    if (wnck_screen_get_active_workspace(wnck_workspace_get_screen(priv->wnck_workspace)) == priv->wnck_workspace) {
        state |= XFW_WORKSPACE_STATE_ACTIVE;
    }
    return state;
}

static guint
xfw_workspace_x11_get_number(XfwWorkspace *workspace) {
    return wnck_workspace_get_number(XFW_WORKSPACE_X11(workspace)->priv->wnck_workspace);
}

static void
xfw_workspace_x11_activate(XfwWorkspace *workspace, GError **error) {
    XfwWorkspaceX11Private *priv = XFW_WORKSPACE_X11(workspace)->priv;
    wnck_workspace_activate(priv->wnck_workspace, GDK_CURRENT_TIME);
}

static void
xfw_workspace_x11_remove(XfwWorkspace *workspace, GError **error) {
    XfwWorkspaceX11Private *priv = XFW_WORKSPACE_X11(workspace)->priv;
    WnckScreen *wnck_screen = wnck_workspace_get_screen(priv->wnck_workspace);
    gint count = wnck_screen_get_workspace_count(wnck_screen);
    if (count > 1) {
        wnck_screen_change_workspace_count(wnck_screen, count - 1);
    } else if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, 0, "Cannot remove workspace as it is the only one left");
    }
}

static void
name_changed(WnckWorkspace *wnck_workspace, XfwWorkspaceX11 *workspace) {
    g_object_notify(G_OBJECT(workspace), "id");
    g_object_notify(G_OBJECT(workspace), "name");
    g_signal_emit_by_name(workspace, "name-changed");
}
