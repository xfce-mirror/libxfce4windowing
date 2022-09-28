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
    GHashTable *pending_workspace_names;
};

static void xfw_workspace_manager_x11_manager_init(XfwWorkspaceManagerIface *iface);
static void xfw_workspace_manager_x11_constructed(GObject *obj);
static void xfw_workspace_manager_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_x11_finalize(GObject *obj);
static GList * xfw_workspace_manager_x11_list_workspace_groups(XfwWorkspaceManager *manager);

static void active_workspace_changed(WnckScreen *screen, WnckWorkspace *workspace, XfwWorkspaceManagerX11 *manager);
static void workspace_created(WnckScreen *screen, WnckWorkspace *workspace, XfwWorkspaceManagerX11 *manager);
static void workspace_destroyed(WnckScreen *screen, WnckWorkspace *workspace, XfwWorkspaceManagerX11 *manager);

static gboolean group_create_workspace(XfwWorkspaceGroup *group, const gchar *name, GError **error);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceManagerX11, xfw_workspace_manager_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceManagerX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_MANAGER,
                                              xfw_workspace_manager_x11_manager_init))

static void
xfw_workspace_manager_x11_class_init(XfwWorkspaceManagerX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_workspace_manager_x11_constructed;
    gklass->set_property = xfw_workspace_manager_x11_set_property;
    gklass->get_property = xfw_workspace_manager_x11_get_property;
    gklass->finalize = xfw_workspace_manager_x11_finalize;

    _xfw_workspace_manager_install_properties(gklass);
}

static void
xfw_workspace_manager_x11_manager_init(XfwWorkspaceManagerIface *iface) {
    iface->list_workspace_groups = xfw_workspace_manager_x11_list_workspace_groups;
}

static void
xfw_workspace_manager_x11_init(XfwWorkspaceManagerX11 *manager) {
    manager->priv = xfw_workspace_manager_x11_get_instance_private(manager);
}

static void
xfw_workspace_manager_x11_constructed(GObject *obj) {
    XfwWorkspaceManagerX11 *manager = XFW_WORKSPACE_MANAGER_X11(obj);
    XfwWorkspaceManagerX11Private *priv = manager->priv;
    XfwWorkspaceGroupDummy *group;
    GList *wnck_workspaces;
    WnckWorkspace *active_wnck_workspace;

    priv->wnck_screen = wnck_screen_get(gdk_x11_screen_get_screen_number(GDK_X11_SCREEN(priv->screen)));
    g_signal_connect(priv->wnck_screen, "active-workspace-changed", (GCallback)active_workspace_changed, manager);
    g_signal_connect(priv->wnck_screen, "workspace-created", (GCallback)workspace_created, manager);
    g_signal_connect(priv->wnck_screen, "workspace-destroyed", (GCallback)workspace_destroyed, manager);

    group = g_object_new(XFW_TYPE_WORKSPACE_GROUP_DUMMY,
                         "screen", priv->screen,
                         "create-workspace-func", group_create_workspace,
                         NULL);
    priv->groups = g_list_append(NULL, group);

    priv->wnck_workspaces = g_hash_table_new_full(g_direct_hash, g_direct_equal, g_object_unref, g_object_unref);
    active_wnck_workspace = wnck_screen_get_active_workspace(priv->wnck_screen);
    wnck_workspaces = wnck_screen_get_workspaces(priv->wnck_screen);
    for (GList *l = wnck_workspaces; l != NULL; l = l->next) {
        XfwWorkspace *workspace = g_object_new(XFW_TYPE_WORKSPACE_X11,
                                               "group", group,
                                               "wnck-workspace", l->data,
                                               NULL);
        if (active_wnck_workspace == l->data) {
            _xfw_workspace_group_dummy_set_active_workspace(XFW_WORKSPACE_GROUP_DUMMY(priv->groups->data), workspace);
        }
        priv->workspaces = g_list_append(priv->workspaces, workspace);
        g_hash_table_insert(priv->wnck_workspaces, g_object_ref(l->data), workspace);
    }
    _xfw_workspace_group_dummy_set_workspaces(group, priv->workspaces);

    manager->priv->pending_workspace_names = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
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
xfw_workspace_manager_x11_finalize(GObject *obj) {
    XfwWorkspaceManagerX11 *manager = XFW_WORKSPACE_MANAGER_X11(obj);
    XfwWorkspaceManagerX11Private *priv = manager->priv;

    g_signal_handlers_disconnect_by_func(priv->wnck_screen, active_workspace_changed, manager);
    g_signal_handlers_disconnect_by_func(priv->wnck_screen, workspace_created, manager);
    g_signal_handlers_disconnect_by_func(priv->wnck_screen, workspace_destroyed, manager);
    g_list_free(priv->workspaces);
    g_hash_table_destroy(priv->wnck_workspaces);
    g_hash_table_destroy(priv->pending_workspace_names);

    g_list_free_full(priv->groups, g_object_unref);

    G_OBJECT_CLASS(xfw_workspace_manager_x11_parent_class)->finalize(obj);
}

static GList *
xfw_workspace_manager_x11_list_workspace_groups(XfwWorkspaceManager *manager) {
    XfwWorkspaceManagerX11 *xmanager = XFW_WORKSPACE_MANAGER_X11(manager);
    return xmanager->priv->groups;
}

static void
active_workspace_changed(WnckScreen *screen, WnckWorkspace *previous_wnck_workspace, XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceX11 *previous_workspace = g_hash_table_lookup(manager->priv->wnck_workspaces, previous_wnck_workspace);
    XfwWorkspaceGroup *group = XFW_WORKSPACE_GROUP(manager->priv->groups->data);
    WnckWorkspace *active_wnck_workspace = wnck_screen_get_active_workspace(screen);
    XfwWorkspace *active_workspace = g_hash_table_lookup(manager->priv->wnck_workspaces, active_wnck_workspace);

    _xfw_workspace_group_dummy_set_active_workspace(XFW_WORKSPACE_GROUP_DUMMY(group), XFW_WORKSPACE(active_workspace));
    if (previous_workspace != NULL) {
        g_object_notify(G_OBJECT(previous_workspace), "state");
        g_signal_emit_by_name(previous_workspace, "state-changed", XFW_WORKSPACE_STATE_ACTIVE);
    }

    g_object_notify(G_OBJECT(active_workspace), "state");
    g_signal_emit_by_name(active_workspace, "state-changed", XFW_WORKSPACE_STATE_NONE);
}

static void
workspace_created(WnckScreen *screen, WnckWorkspace *wnck_workspace, XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceX11 *workspace = XFW_WORKSPACE_X11(g_object_new(XFW_TYPE_WORKSPACE_X11,
                                                                "group", manager->priv->groups->data,
                                                                "wnck-workspace", wnck_workspace,
                                                                NULL));
    gint workspace_num = wnck_workspace_get_number(wnck_workspace);
    gchar *pending_name = g_hash_table_lookup(manager->priv->pending_workspace_names, GUINT_TO_POINTER(workspace_num));

    if (pending_name != NULL) {
        wnck_workspace_change_name(wnck_workspace, pending_name);
        g_hash_table_remove(manager->priv->pending_workspace_names, GUINT_TO_POINTER(workspace_num));
    }

    g_hash_table_insert(manager->priv->wnck_workspaces, wnck_workspace, workspace);
    manager->priv->workspaces = g_list_insert(manager->priv->workspaces, workspace, workspace_num);

    _xfw_workspace_group_dummy_set_workspaces(XFW_WORKSPACE_GROUP_DUMMY(manager->priv->groups->data), manager->priv->workspaces);
    g_signal_emit_by_name(manager->priv->groups->data, "workspace-created", workspace);
}

static void
workspace_destroyed(WnckScreen *screen, WnckWorkspace *wnck_workspace, XfwWorkspaceManagerX11 *manager) {
    XfwWorkspaceX11 *workspace = XFW_WORKSPACE_X11(g_hash_table_lookup(manager->priv->wnck_workspaces, wnck_workspace));
    if (workspace != NULL) {
        XfwWorkspaceGroup *group = XFW_WORKSPACE_GROUP(manager->priv->groups->data);

        g_object_ref(workspace);

        if (XFW_WORKSPACE(workspace) == xfw_workspace_group_get_active_workspace(group)) {
            _xfw_workspace_group_dummy_set_active_workspace(XFW_WORKSPACE_GROUP_DUMMY(group), XFW_WORKSPACE(workspace));
        }

        g_hash_table_remove(manager->priv->wnck_workspaces, wnck_workspace);
        manager->priv->workspaces = g_list_remove(manager->priv->workspaces, workspace);

        _xfw_workspace_group_dummy_set_workspaces(XFW_WORKSPACE_GROUP_DUMMY(manager->priv->groups->data), manager->priv->workspaces);
        g_signal_emit_by_name(group, "workspace-destroyed", workspace);

        g_object_unref(workspace);
    }
}

static gboolean
group_create_workspace(XfwWorkspaceGroup *group, const gchar *name, GError **error) {
    XfwWorkspaceManagerX11 *manager = XFW_WORKSPACE_MANAGER_X11(xfw_workspace_group_get_workspace_manager(group));
    gint count = wnck_screen_get_workspace_count(manager->priv->wnck_screen);

    if (name != NULL) {
        g_hash_table_insert(manager->priv->pending_workspace_names, GUINT_TO_POINTER(count + 1), g_strdup(name));
    }
    wnck_screen_change_workspace_count(manager->priv->wnck_screen, count + 1);
    return TRUE;
}

XfwWorkspaceManager *
_xfw_workspace_manager_x11_new(GdkScreen *screen) {
    return g_object_new(XFW_TYPE_WORKSPACE_MANAGER_X11,
                        "screen", screen,
                        NULL);
}

XfwWorkspace *
_xfw_workspace_manager_x11_workspace_for_wnck_workspace(XfwWorkspaceManagerX11 *manager, WnckWorkspace *wnck_workspace) {
    return g_hash_table_lookup(manager->priv->wnck_workspaces, wnck_workspace);
}
