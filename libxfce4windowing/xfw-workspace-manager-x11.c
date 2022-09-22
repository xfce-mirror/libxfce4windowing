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

#include <gdk/gdkx.h>
#include <libwnck/libwnck.h>

#include "libxfce4windowing-private.h"
#include "xfw-workspace-group-dummy.h"
#include "xfw-workspace-manager-x11.h"
#include "xfw-workspace-manager.h"
#include "xfw-workspace-x11.h"

struct _XfwWorkspaceManagerX11Private {
    GdkScreen *screen;
    WnckScreen *wnck_screen;
    GList *groups;
    GList *workspaces;
    GHashTable *wnck_workspaces;
};

static void xfw_workspace_manager_x11_manager_init(XfwWorkspaceManagerIface *iface);
static void xfw_workspace_manager_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_x11_dispose(GObject *obj);
static GList * xfw_workspace_manager_x11_list_workspace_groups(XfwWorkspaceManager *manager);

static void active_workspace_changed(WnckScreen *screen, WnckWorkspace *workspace, XfwWorkspaceManagerX11 *manager);
static void workspace_created(WnckScreen *screen, WnckWorkspace *workspace, XfwWorkspaceManagerX11 *manager);
static void workspace_destroyed(WnckScreen *screen, WnckWorkspace *workspace, XfwWorkspaceManagerX11 *manager);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceManagerX11, xfw_workspace_manager_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceManagerX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_MANAGER,
                                              xfw_workspace_manager_x11_manager_init))

static void
xfw_workspace_manager_x11_class_init(XfwWorkspaceManagerX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->set_property = xfw_workspace_manager_x11_set_property;
    gklass->get_property = xfw_workspace_manager_x11_get_property;
    gklass->dispose = xfw_workspace_manager_x11_dispose;

    _xfw_workspace_manager_install_properties(gklass);
}

static void
xfw_workspace_manager_x11_manager_init(XfwWorkspaceManagerIface *iface) {
    iface->list_workspace_groups = xfw_workspace_manager_x11_list_workspace_groups;
}

static void
xfw_workspace_manager_x11_init(XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceManagerX11Private *priv = manager->priv;
    GList *wnck_workspaces;
    WnckWorkspace *active_wnck_workspace;

    priv->wnck_screen = wnck_screen_get(gdk_x11_screen_get_screen_number(GDK_X11_SCREEN(priv->screen)));
    g_signal_connect(priv->wnck_screen, "active-workspace-changed", (GCallback)active_workspace_changed, manager);
    g_signal_connect(priv->wnck_screen, "workspace-created", (GCallback)workspace_created, manager);
    g_signal_connect(priv->wnck_screen, "workspace-destroyed", (GCallback)workspace_destroyed, manager);

    priv->wnck_workspaces = g_hash_table_new_full(g_direct_hash, g_direct_equal, g_object_unref, g_object_unref);
    active_wnck_workspace = wnck_screen_get_active_workspace(priv->wnck_screen);
    wnck_workspaces = wnck_screen_get_workspaces(priv->wnck_screen);
    for (GList *l = wnck_workspaces; l != NULL; l = l->next) {
        XfwWorkspace *workspace = g_object_new(XFW_TYPE_WORKSPACE_X11, "wnck-workspace", l->data, NULL);
        if (active_wnck_workspace == l->data) {
            _xfw_workspace_group_dummy_set_active_workspace(XFW_WORKSPACE_GROUP_DUMMY(priv->groups->data), workspace);
        }
        priv->workspaces = g_list_prepend(priv->workspaces, workspace);
        g_hash_table_insert(priv->wnck_workspaces, g_object_ref(l->data), workspace);
    }

    priv->groups = g_list_append(NULL,
                                 g_object_new(XFW_TYPE_WORKSPACE_GROUP_DUMMY,
                                              "screen", priv->screen,
                                              "workspaces", priv->workspaces,
                                              NULL));
}

static void
xfw_workspace_manager_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceManagerX11 *manager = XFW_WORKSPACE_MANAGER_X11(obj);

    switch (prop_id) {
        case WORKSPACE_MANAGER_PROP_SCREEN:
            manager->priv->screen = GDK_SCREEN(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void xfw_workspace_manager_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceManagerX11 *manager = XFW_WORKSPACE_MANAGER_X11(obj);

    switch (prop_id) {
        case WORKSPACE_MANAGER_PROP_SCREEN:
            g_value_set_object(value, manager->priv->screen);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_manager_x11_dispose(GObject *obj) {
    XfwWorkspaceManagerX11 *manager = XFW_WORKSPACE_MANAGER_X11(obj);
    XfwWorkspaceManagerX11Private *priv = manager->priv;
    g_signal_handlers_disconnect_by_func(priv->wnck_screen, active_workspace_changed, manager);
    g_signal_handlers_disconnect_by_func(priv->wnck_screen, workspace_created, manager);
    g_signal_handlers_disconnect_by_func(priv->wnck_screen, workspace_destroyed, manager);
    g_list_free(priv->workspaces);
    g_hash_table_destroy(priv->wnck_workspaces);
    g_list_free_full(priv->groups, g_object_unref);
}

static GList *
xfw_workspace_manager_x11_list_workspace_groups(XfwWorkspaceManager *manager) {
    XfwWorkspaceManagerX11 *xmanager = XFW_WORKSPACE_MANAGER_X11(manager);
    return xmanager->priv->groups;
}

static void
active_workspace_changed(WnckScreen *screen, WnckWorkspace *wnck_workspace, XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceX11 *workspace = XFW_WORKSPACE_X11(g_hash_table_lookup(manager->priv->wnck_workspaces, wnck_workspace));
    if (workspace != NULL) {
        XfwWorkspaceGroup *group = XFW_WORKSPACE_GROUP(manager->priv->groups->data);
        XfwWorkspace *old_active_workspace = xfw_workspace_group_get_active_workspace(group);

        if (old_active_workspace != NULL) {
            g_object_notify(G_OBJECT(old_active_workspace), "state");
            g_signal_emit_by_name(old_active_workspace, "state-changed", XFW_WORKSPACE_STATE_ACTIVE);
        }

        g_object_notify(G_OBJECT(workspace), "state");
        g_signal_emit_by_name(workspace, "state-changed", XFW_WORKSPACE_STATE_NONE);
        _xfw_workspace_group_dummy_set_active_workspace(XFW_WORKSPACE_GROUP_DUMMY(group), XFW_WORKSPACE(workspace));
    }
}

static void
workspace_created(WnckScreen *screen, WnckWorkspace *wnck_workspace, XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceX11 *workspace = XFW_WORKSPACE_X11(g_object_new(XFW_TYPE_WORKSPACE_X11, "wnck-workspace", wnck_workspace, NULL));
    int n_workspaces = wnck_screen_get_workspace_count(screen);
    GValue value = G_VALUE_INIT;

    g_hash_table_insert(manager->priv->wnck_workspaces, wnck_workspace, workspace);
    for (int i = 0; i < n_workspaces; ++i) {
        if (wnck_screen_get_workspace(screen, i) == wnck_workspace) {
            manager->priv->workspaces = g_list_insert(manager->priv->workspaces, workspace, i);
            break;
        }
    }

    g_value_set_pointer(&value, manager->priv->workspaces);
    g_object_set_property(G_OBJECT(manager->priv->groups->data), "workspaces", &value);

    g_signal_emit_by_name(manager->priv->groups->data, "workspace-added", workspace);
}

static void
workspace_destroyed(WnckScreen *screen, WnckWorkspace *wnck_workspace, XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceX11 *workspace = XFW_WORKSPACE_X11(g_hash_table_lookup(manager->priv->wnck_workspaces, wnck_workspace));
    if (workspace != NULL) {
        GValue value = G_VALUE_INIT;
        XfwWorkspaceGroup *group = XFW_WORKSPACE_GROUP(manager->priv->groups->data);

        g_object_ref(workspace);

        if (XFW_WORKSPACE(workspace) == xfw_workspace_group_get_active_workspace(group)) {
            _xfw_workspace_group_dummy_set_active_workspace(XFW_WORKSPACE_GROUP_DUMMY(group), XFW_WORKSPACE(workspace));
        }

        g_hash_table_remove(manager->priv->wnck_workspaces, wnck_workspace);
        manager->priv->workspaces = g_list_remove(manager->priv->workspaces, workspace);

        g_value_set_pointer(&value, manager->priv->workspaces);
        g_object_set_property(G_OBJECT(manager->priv->groups->data), "workspaces", &value);

        g_signal_emit_by_name(group, "workspace-removed", workspace);

        g_object_unref(workspace);
    }
}

XfwWorkspaceManager *
_xfw_workspace_manager_x11_new(GdkScreen *screen) {
    return g_object_new(XFW_TYPE_WORKSPACE_MANAGER_X11,
                        "screen", screen,
                        NULL);
}
