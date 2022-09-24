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

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <gdk/gdkwayland.h>

#include "protocols/ext-workspace-v1-20220919-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-workspace-group-wayland.h"
#include "xfw-workspace-group.h"
#include "xfw-workspace-wayland.h"
#include "xfw-workspace.h"

enum {
    SIGNAL_DESTROYED,

    N_SIGNALS,
};

struct _XfwWorkspaceGroupWaylandPrivate {
    GdkScreen *screen;
    XfwWorkspaceManager *workspace_manager;
    struct ext_workspace_group_handle_v1 *handle;
    GList *workspaces;
    XfwWorkspace *active_workspace;
    GHashTable *wl_workspaces;
    GList *monitors;
};

static guint group_signals[N_SIGNALS]  = { 0, };

static void xfw_workspace_group_wayland_workspace_group_init(XfwWorkspaceGroupIface *iface);
static void xfw_workspace_group_wayland_constructed(GObject *obj);
static void xfw_workspace_group_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_group_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_group_wayland_dispose(GObject *obj);
static guint xfw_workspace_group_wayland_get_workspace_count(XfwWorkspaceGroup *group);
static GList *xfw_workspace_group_wayland_list_workspaces(XfwWorkspaceGroup *group);
static XfwWorkspace *xfw_workspace_group_wayland_get_active_workspace(XfwWorkspaceGroup *group);
static GList *xfw_workspace_group_wayland_get_monitors(XfwWorkspaceGroup *group);
static XfwWorkspaceManager * xfw_workspace_group_wayland_get_workspace_manager(XfwWorkspaceGroup *group);

static void group_capabilities(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_array *capabilities);
static void group_output_enter(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_output *output);
static void group_output_leave(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_output *output);
static void group_workspace(void *data, struct ext_workspace_group_handle_v1 *group, struct ext_workspace_handle_v1 *workspace);
static void group_removed(void *data, struct ext_workspace_group_handle_v1 *group);

static void monitor_added(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupWayland *group);
static void monitor_removed(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupWayland *group);

static const struct ext_workspace_group_handle_v1_listener group_listener = {
    .capabilities = group_capabilities,
    .output_enter = group_output_enter,
    .output_leave = group_output_leave,
    .workspace = group_workspace,
    .removed = group_removed,
};

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceGroupWayland, xfw_workspace_group_wayland, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_GROUP,
                                              xfw_workspace_group_wayland_workspace_group_init))

static void
xfw_workspace_group_wayland_class_init(XfwWorkspaceGroupWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_workspace_group_wayland_constructed;
    gklass->set_property = xfw_workspace_group_wayland_set_property;
    gklass->get_property = xfw_workspace_group_wayland_get_property;
    gklass->dispose = xfw_workspace_group_wayland_dispose;

    group_signals[SIGNAL_DESTROYED] = g_signal_new("destroyed",
                                                   XFW_TYPE_WORKSPACE_GROUP_WAYLAND,
                                                   G_SIGNAL_RUN_LAST,
                                                   G_STRUCT_OFFSET(XfwWorkspaceGroupWaylandClass, destroyed),
                                                   NULL, NULL,
                                                   g_cclosure_marshal_VOID__VOID,
                                                   G_TYPE_NONE, 0);

    _xfw_workspace_group_install_properties(gklass);
}

static void
xfw_workspace_group_wayland_init(XfwWorkspaceGroupWayland *group) {
    group->priv = xfw_workspace_group_wayland_get_instance_private(group);
}

static void
xfw_workspace_group_wayland_constructed(GObject *obj) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(obj);
    group->priv->wl_workspaces = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    ext_workspace_group_handle_v1_add_listener(group->priv->handle, &group_listener, group);
}

static void
xfw_workspace_group_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(obj);

    switch (prop_id) {
        case WORKSPACE_GROUP_PROP_SCREEN:
            group->priv->screen = g_value_get_object(value);
            break;

        case WORKSPACE_GROUP_PROP_WORKSPACE_MANAGER:
            group->priv->workspace_manager = g_value_get_object(value);
            break;

        case WORKSPACE_GROUP_PROP_WORKSPACES:
        case WORKSPACE_GROUP_PROP_ACTIVE_WORKSPACE:
        case WORKSPACE_GROUP_PROP_MONITORS:
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_group_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(obj);

    switch (prop_id) {
        case WORKSPACE_GROUP_PROP_SCREEN:
            g_value_set_object(value, group->priv->screen);
            break;

        case WORKSPACE_GROUP_PROP_WORKSPACE_MANAGER:
            g_value_set_object(value, group->priv->workspace_manager);
            break;

        case WORKSPACE_GROUP_PROP_WORKSPACES:
            g_value_set_pointer(value, group->priv->workspaces);
            break;

        case WORKSPACE_GROUP_PROP_ACTIVE_WORKSPACE:
            g_value_set_object(value, group->priv->active_workspace);
            break;

        case WORKSPACE_GROUP_PROP_MONITORS:
            g_value_set_pointer(value, group->priv->monitors);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_group_wayland_dispose(GObject *obj) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(obj);
    GdkDisplay *display;

    ext_workspace_group_handle_v1_destroy(group->priv->handle);

    g_list_free(group->priv->workspaces);
    g_hash_table_destroy(group->priv->wl_workspaces);

    display = gdk_screen_get_display(group->priv->screen);
    g_signal_handlers_disconnect_by_func(display, monitor_added, group);
    g_signal_handlers_disconnect_by_func(display, monitor_removed, group);
    g_list_free(group->priv->monitors);
}

static void
xfw_workspace_group_wayland_workspace_group_init(XfwWorkspaceGroupIface *iface) {
    iface->get_workspace_count = xfw_workspace_group_wayland_get_workspace_count;
    iface->list_workspaces = xfw_workspace_group_wayland_list_workspaces;
    iface->get_active_workspace = xfw_workspace_group_wayland_get_active_workspace;
    iface->get_monitors = xfw_workspace_group_wayland_get_monitors;
    iface->get_workspace_manager = xfw_workspace_group_wayland_get_workspace_manager;
}

static guint
xfw_workspace_group_wayland_get_workspace_count(XfwWorkspaceGroup *group) {
    return g_list_length(XFW_WORKSPACE_GROUP_WAYLAND(group)->priv->workspaces);
}

static GList *
xfw_workspace_group_wayland_list_workspaces(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_WAYLAND(group)->priv->workspaces;
}

static XfwWorkspace *
xfw_workspace_group_wayland_get_active_workspace(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_WAYLAND(group)->priv->active_workspace;
}

static GList *
xfw_workspace_group_wayland_get_monitors(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_WAYLAND(group)->priv->monitors;
}

static XfwWorkspaceManager *
xfw_workspace_group_wayland_get_workspace_manager(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_WAYLAND(group)->priv->workspace_manager;
}

static void
group_capabilities(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_array *capabilities) {

}

static void
group_output_enter(void *data, struct ext_workspace_group_handle_v1 *wl_group, struct wl_output *output) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(data);
    GdkDisplay *display = gdk_screen_get_display(group->priv->screen);
    int n_monitors = gdk_display_get_n_monitors(display);

    for (int i = 0; i < n_monitors; ++i) {
        GdkMonitor *monitor = gdk_display_get_monitor(display, i);
        if (gdk_wayland_monitor_get_wl_output(monitor) == output) {
            group->priv->monitors = g_list_append(group->priv->monitors, monitor);
            g_signal_emit_by_name(group, "monitors-changed");
            break;
        }
    }
}

static void
group_output_leave(void *data, struct ext_workspace_group_handle_v1 *wl_group, struct wl_output *output) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(data);

    for (GList *l = group->priv->monitors; l != NULL; l = l->next) {
        if (gdk_wayland_monitor_get_wl_output(GDK_MONITOR(l->data)) == output) {
            group->priv->monitors = g_list_remove_link(group->priv->monitors, l);
            g_list_free_1(l);
            g_signal_emit_by_name(group, "monitors-changed");
            break;
        }
    }
}

static void
workspace_destroyed(XfwWorkspace *workspace, XfwWorkspaceGroupWayland *group) {
    guint old_number = UINT_MAX;
    GList *l;

    g_signal_handlers_disconnect_by_func(workspace, workspace_destroyed, group);

    l = group->priv->workspaces;
    while (l != NULL) {
        XfwWorkspace *cur_workspace = XFW_WORKSPACE(l->data);
        guint cur_number = xfw_workspace_get_number(cur_workspace);
        l = l->next;

        if (cur_workspace == workspace) {
            old_number = cur_number;
            group->priv->workspaces = g_list_remove_link(group->priv->workspaces, l);
            g_list_free_1(l);
        } else if (old_number < cur_number) {
            _xfw_workspace_wayland_set_number(XFW_WORKSPACE_WAYLAND(cur_workspace), cur_number - 1);
        }
    }

    g_signal_emit_by_name(group, "workspace-destroyed", workspace);
    g_object_unref(workspace);
}

static void
group_workspace(void *data, struct ext_workspace_group_handle_v1 *wl_group, struct ext_workspace_handle_v1 *wl_workspace) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(data);
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(g_object_new(XFW_TYPE_WORKSPACE_WAYLAND,
                                                                        "group", group,
                                                                        "handle", wl_workspace,
                                                                        NULL));
    _xfw_workspace_wayland_set_number(workspace, g_list_length(group->priv->workspaces));
    g_hash_table_insert(group->priv->wl_workspaces, wl_workspace, workspace);
    group->priv->workspaces = g_list_append(group->priv->workspaces, workspace);
    g_signal_connect(workspace, "destroyed", (GCallback)workspace_destroyed, group);
    g_signal_emit_by_name(group, "workspace-created", workspace);
}

static void
group_removed(void *data, struct ext_workspace_group_handle_v1 *wl_group) {
    XfwWorkspaceGroupWayland *group = XFW_WORKSPACE_GROUP_WAYLAND(data);
    GList *l = group->priv->workspaces;

    // NB: use a while loop here instead of a for loop because we have to advance
    // the 'l' pointer _before_ calling workspace_destroyed().
    while (l != NULL) {
        XfwWorkspace *workspace = XFW_WORKSPACE(l->data);
        l = l->next;
        workspace_destroyed(workspace, group);
    }

    g_signal_emit(group, group_signals[SIGNAL_DESTROYED], 0);
}

static void
monitor_added(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupWayland *group) {
    int n_monitors = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n_monitors; ++i) {
        if (gdk_display_get_monitor(display, i) == monitor) {
            group->priv->monitors = g_list_insert(group->priv->monitors, monitor, i);
            break;
        }
    }
}

static void
monitor_removed(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupWayland *group) {
    group->priv->monitors = g_list_remove(group->priv->monitors, monitor);
    g_signal_emit_by_name(group, "monitors-changed");
}

void
_xfw_workspace_group_wayland_set_active_workspace(XfwWorkspaceGroupWayland *group, XfwWorkspace *workspace) {
    if (group->priv->active_workspace != workspace) {
        XfwWorkspace *old_workspace = group->priv->active_workspace;
        group->priv->active_workspace = workspace;
        g_object_notify(G_OBJECT(group), "active-workspace");
        g_signal_emit_by_name(group, "active-workspace-changed", old_workspace);
    }
}
