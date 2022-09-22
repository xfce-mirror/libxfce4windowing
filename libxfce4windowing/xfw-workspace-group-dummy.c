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
#include <gdk/gdk.h>

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-workspace-group-dummy.h"
#include "xfw-workspace-group.h"
#include "xfw-workspace.h"

struct _XfwWorkspaceGroupDummyPrivate {
    GdkScreen *screen;
    GList *workspaces;
    GList *monitors;
};

static void xfw_workspace_group_dummy_workspace_init(XfwWorkspaceGroupIface *iface);
static void xfw_workspace_group_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_group_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_group_dummy_dispose(GObject *obj);
static GList *xfw_workspace_group_dummy_list_workspaces(XfwWorkspaceGroup *group);
static GList *xfw_workspace_group_dummy_get_monitors(XfwWorkspaceGroup *group);

static void monitor_added(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupDummy *group);
static void monitor_removed(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupDummy *group);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceGroupDummy, xfw_workspace_group_dummy, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE,
                                              xfw_workspace_group_dummy_workspace_init))

static void
xfw_workspace_group_dummy_class_init(XfwWorkspaceGroupDummyClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->set_property = xfw_workspace_group_dummy_set_property;
    gklass->get_property = xfw_workspace_group_dummy_get_property;
    gklass->dispose = xfw_workspace_group_dummy_dispose;

    _xfw_workspace_group_install_properties(gklass);
}

static void
xfw_workspace_group_dummy_init(XfwWorkspaceGroupDummy *group) {
    GdkDisplay *display = gdk_screen_get_display(group->priv->screen);
    int n_monitors = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n_monitors; ++i) {
        group->priv->monitors = g_list_prepend(group->priv->monitors, gdk_display_get_monitor(display, i));
    }
    group->priv->monitors = g_list_reverse(group->priv->monitors);
    g_signal_connect(display, "monitor-added", (GCallback)monitor_added, group);
    g_signal_connect(display, "monitor-removed", (GCallback)monitor_removed, group);
}

static void
xfw_workspace_group_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);

    switch (prop_id) {
        case WORKSPACE_MANAGER_PROP_SCREEN:
            group->priv->screen = g_value_get_object(value);
            break;

        case WORKSPACE_GROUP_PROP_WORKSPACES:
            g_list_free(group->priv->workspaces);
            group->priv->workspaces = g_list_copy(g_value_get_pointer(value));

        case WORKSPACE_GROUP_PROP_MONITORS:
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_group_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);

    switch (prop_id) {
        case WORKSPACE_GROUP_PROP_SCREEN:
            g_value_set_object(value, group->priv->screen);
            break;

        case WORKSPACE_GROUP_PROP_WORKSPACES:
            g_value_set_pointer(value, group->priv->workspaces);
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
xfw_workspace_group_dummy_dispose(GObject *obj) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);
    GdkDisplay *display;

    g_list_free(group->priv->workspaces);

    display = gdk_screen_get_display(group->priv->screen);
    g_signal_handlers_disconnect_by_func(display, monitor_added, group);
    g_signal_handlers_disconnect_by_func(display, monitor_removed, group);
    g_list_free(group->priv->monitors);
}

static void
xfw_workspace_group_dummy_workspace_init(XfwWorkspaceGroupIface *iface) {
    iface->list_workspaces = xfw_workspace_group_dummy_list_workspaces;
    iface->get_monitors = xfw_workspace_group_dummy_get_monitors;
}

static GList *
xfw_workspace_group_dummy_list_workspaces(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_DUMMY(group)->priv->workspaces;
}

static GList *
xfw_workspace_group_dummy_get_monitors(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_DUMMY(group)->priv->monitors;
}

static void
monitor_added(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupDummy *group) {
    int n_monitors = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n_monitors; ++i) {
        if (gdk_display_get_monitor(display, i) == monitor) {
            group->priv->monitors = g_list_insert(group->priv->monitors, monitor, i);
            break;
        }
    }
}

static void
monitor_removed(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupDummy *group) {
    group->priv->monitors = g_list_remove(group->priv->monitors, monitor);
    g_signal_emit_by_name(group, "monitors-changed");
}
