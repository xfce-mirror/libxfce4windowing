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

enum {
    PROP0,
    PROP_CREATE_WORKSPACE_FUNC,
    PROP_MOVE_VIEWPORT_FUNC,
};

struct _XfwWorkspaceGroupDummyPrivate {
    XfwCreateWorkspaceFunc create_workspace_func;
    XfwMoveViewportFunc move_viewport_func;
    GdkScreen *screen;
    XfwWorkspaceManager *workspace_manager;
    GList *workspaces;
    GList *monitors;
    XfwWorkspace *active_workspace;
};

static void xfw_workspace_group_dummy_workspace_group_init(XfwWorkspaceGroupIface *iface);
static void xfw_workspace_group_dummy_constructed(GObject *obj);
static void xfw_workspace_group_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_group_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_group_dummy_finalize(GObject *obj);
static XfwWorkspaceGroupCapabilities xfw_workspace_group_dummy_get_capabilities(XfwWorkspaceGroup *group);
static guint xfw_workspace_group_dummy_get_workspace_count(XfwWorkspaceGroup *group);
static GList *xfw_workspace_group_dummy_list_workspaces(XfwWorkspaceGroup *group);
static XfwWorkspace *xfw_workspace_group_dummy_get_active_workspace(XfwWorkspaceGroup *group);
static GList *xfw_workspace_group_dummy_get_monitors(XfwWorkspaceGroup *group);
static XfwWorkspaceManager *xfw_workspace_group_dummy_get_workspace_manager(XfwWorkspaceGroup *group);
static gboolean xfw_workspace_group_dummy_create_workspace(XfwWorkspaceGroup *group, const gchar *name, GError **error);
static gboolean xfw_workspace_group_dummy_move_viewport(XfwWorkspaceGroup *group, gint x, gint y, GError **error);

static void monitor_added(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupDummy *group);
static void monitor_removed(GdkDisplay *display, GdkMonitor *monitor, XfwWorkspaceGroupDummy *group);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceGroupDummy, xfw_workspace_group_dummy, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceGroupDummy)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_GROUP,
                                              xfw_workspace_group_dummy_workspace_group_init))

static void
xfw_workspace_group_dummy_class_init(XfwWorkspaceGroupDummyClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_workspace_group_dummy_constructed;
    gklass->set_property = xfw_workspace_group_dummy_set_property;
    gklass->get_property = xfw_workspace_group_dummy_get_property;
    gklass->finalize = xfw_workspace_group_dummy_finalize;

    g_object_class_install_property(gklass,
                                    PROP_CREATE_WORKSPACE_FUNC,
                                    g_param_spec_pointer("create-workspace-func",
                                                         "create-workspace-func",
                                                         "create-workspace-func",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
    g_object_class_install_property(gklass,
                                    PROP_MOVE_VIEWPORT_FUNC,
                                    g_param_spec_pointer("move-viewport-func",
                                                         "move-viewport-func",
                                                         "move-viewport-func",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
    _xfw_workspace_group_install_properties(gklass);
}

static void
xfw_workspace_group_dummy_init(XfwWorkspaceGroupDummy *group) {
    group->priv = xfw_workspace_group_dummy_get_instance_private(group);
}

static void
xfw_workspace_group_dummy_constructed(GObject *obj) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);
    GdkDisplay *display = gdk_screen_get_display(group->priv->screen);
    int n_monitors = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n_monitors; ++i) {
        group->priv->monitors = g_list_prepend(group->priv->monitors, gdk_display_get_monitor(display, i));
    }
    group->priv->monitors = g_list_reverse(group->priv->monitors);
    g_signal_connect(display, "monitor-added", G_CALLBACK(monitor_added), group);
    g_signal_connect(display, "monitor-removed", G_CALLBACK(monitor_removed), group);
}

static void
xfw_workspace_group_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);

    switch (prop_id) {
        case PROP_CREATE_WORKSPACE_FUNC:
            group->priv->create_workspace_func = g_value_get_pointer(value);
            break;

        case PROP_MOVE_VIEWPORT_FUNC:
            group->priv->move_viewport_func = g_value_get_pointer(value);
            break;

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
xfw_workspace_group_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);

    switch (prop_id) {
        case PROP_CREATE_WORKSPACE_FUNC:
            g_value_set_pointer(value, group->priv->create_workspace_func);
            break;

        case PROP_MOVE_VIEWPORT_FUNC:
            g_value_set_pointer(value, group->priv->move_viewport_func);
            break;

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
xfw_workspace_group_dummy_finalize(GObject *obj) {
    XfwWorkspaceGroupDummy *group = XFW_WORKSPACE_GROUP_DUMMY(obj);
    GdkDisplay *display;

    g_list_free(group->priv->workspaces);

    display = gdk_screen_get_display(group->priv->screen);
    g_signal_handlers_disconnect_by_func(display, monitor_added, group);
    g_signal_handlers_disconnect_by_func(display, monitor_removed, group);
    g_list_free(group->priv->monitors);

    G_OBJECT_CLASS(xfw_workspace_group_dummy_parent_class)->finalize(obj);
}

static void
xfw_workspace_group_dummy_workspace_group_init(XfwWorkspaceGroupIface *iface) {
    iface->get_capabilities = xfw_workspace_group_dummy_get_capabilities;
    iface->get_workspace_count = xfw_workspace_group_dummy_get_workspace_count;
    iface->list_workspaces = xfw_workspace_group_dummy_list_workspaces;
    iface->get_active_workspace = xfw_workspace_group_dummy_get_active_workspace;
    iface->get_monitors = xfw_workspace_group_dummy_get_monitors;
    iface->get_workspace_manager = xfw_workspace_group_dummy_get_workspace_manager;
    iface->create_workspace = xfw_workspace_group_dummy_create_workspace;
    iface->move_viewport = xfw_workspace_group_dummy_move_viewport;
}

static XfwWorkspaceGroupCapabilities
xfw_workspace_group_dummy_get_capabilities(XfwWorkspaceGroup *group) {
    XfwWorkspaceGroupDummyPrivate *priv = XFW_WORKSPACE_GROUP_DUMMY(group)->priv;
    XfwWorkspaceGroupCapabilities capabilities = XFW_WORKSPACE_GROUP_CAPABILITIES_NONE;
    if (priv->create_workspace_func != NULL) {
        capabilities |= XFW_WORKSPACE_GROUP_CAPABILITIES_CREATE_WORKSPACE;
    }
    if (priv->move_viewport_func != NULL) {
        capabilities |= XFW_WORKSPACE_GROUP_CAPABILITIES_MOVE_VIEWPORT;
    }
    return capabilities;
}

static guint
xfw_workspace_group_dummy_get_workspace_count(XfwWorkspaceGroup *group) {
    return g_list_length(XFW_WORKSPACE_GROUP_DUMMY(group)->priv->workspaces);
}

static GList *
xfw_workspace_group_dummy_list_workspaces(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_DUMMY(group)->priv->workspaces;
}

static XfwWorkspace *
xfw_workspace_group_dummy_get_active_workspace(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_DUMMY(group)->priv->active_workspace;
}

static GList *
xfw_workspace_group_dummy_get_monitors(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_DUMMY(group)->priv->monitors;
}

static XfwWorkspaceManager *
xfw_workspace_group_dummy_get_workspace_manager(XfwWorkspaceGroup *group) {
    return XFW_WORKSPACE_GROUP_DUMMY(group)->priv->workspace_manager;
}

static gboolean
xfw_workspace_group_dummy_create_workspace(XfwWorkspaceGroup *group, const gchar *name, GError **error) {
    XfwWorkspaceGroupDummy *dgroup = XFW_WORKSPACE_GROUP_DUMMY(group);
    if (dgroup->priv->create_workspace_func != NULL) {
        return (*dgroup->priv->create_workspace_func)(group, name, error);
    } else {
        if (error) {
            *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This workspace group does not support creating new workspaces");
        }
        return FALSE;
    }
}

static gboolean
xfw_workspace_group_dummy_move_viewport(XfwWorkspaceGroup *group, gint x, gint y, GError **error) {
    XfwWorkspaceGroupDummy *dgroup = XFW_WORKSPACE_GROUP_DUMMY(group);
    if (dgroup->priv->move_viewport_func != NULL) {
        return (*dgroup->priv->move_viewport_func)(group, x, y, error);
    } else {
        if (error) {
            *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This workspace group does not support moving viewports");
        }
        return FALSE;
    }
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

void
_xfw_workspace_group_dummy_set_workspaces(XfwWorkspaceGroupDummy *group, GList *workspaces) {
    if (group->priv->workspaces != NULL) {
        g_list_free(group->priv->workspaces);
    }
    group->priv->workspaces = g_list_copy(workspaces);
}

void
_xfw_workspace_group_dummy_set_active_workspace(XfwWorkspaceGroupDummy *group, XfwWorkspace *workspace) {
    if (workspace != group->priv->active_workspace) {
        XfwWorkspace *old_workspace = group->priv->active_workspace;
        group->priv->active_workspace = workspace;
        g_object_notify(G_OBJECT(group), "active-workspace");
        g_signal_emit_by_name(group, "active-workspace-changed", old_workspace);
    }
}
