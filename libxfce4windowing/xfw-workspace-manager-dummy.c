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
#include "xfw-workspace-manager-dummy.h"
#include "xfw-workspace-dummy.h"
#include "xfw-workspace-group-dummy.h"
#include "xfw-workspace-manager.h"

struct _XfwWorkspaceManagerDummyPrivate {
    GdkScreen *screen;
    GList *groups;
    GList *workspaces;
};

static void xfw_workspace_manager_dummy_manager_init(XfwWorkspaceManagerIface *iface);
static void xfw_workspace_manager_dummy_constructed(GObject *obj);
static void xfw_workspace_manager_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_dummy_dispose(GObject *obj);
static GList *xfw_workspace_manager_dummy_list_workspace_groups(XfwWorkspaceManager *manager);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceManagerDummy, xfw_workspace_manager_dummy, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceManagerDummy)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_MANAGER,
                                              xfw_workspace_manager_dummy_manager_init))

static void
xfw_workspace_manager_dummy_class_init(XfwWorkspaceManagerDummyClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    gklass->constructed = xfw_workspace_manager_dummy_constructed;
    gklass->set_property = xfw_workspace_manager_dummy_set_property;
    gklass->get_property = xfw_workspace_manager_dummy_get_property;
    gklass->dispose = xfw_workspace_manager_dummy_dispose;
    _xfw_workspace_manager_install_properties(gklass);
}

static void
xfw_workspace_manager_dummy_manager_init(XfwWorkspaceManagerIface *iface) {
    iface->list_workspace_groups = xfw_workspace_manager_dummy_list_workspace_groups;
}

static void
xfw_workspace_manager_dummy_init(XfwWorkspaceManagerDummy *manager) {
    manager->priv = xfw_workspace_manager_dummy_get_instance_private(manager);
}

static void
xfw_workspace_manager_dummy_constructed(GObject *obj) {
    XfwWorkspaceManagerDummy *manager = XFW_WORKSPACE_MANAGER_DUMMY(obj);
    XfwWorkspaceGroup *group;

    manager->priv = xfw_workspace_manager_dummy_get_instance_private(manager);

    group = g_object_new(XFW_TYPE_WORKSPACE_GROUP_DUMMY,
                         "screen", manager->priv->screen,
                         NULL);
    manager->priv->groups = g_list_append(NULL, group);
    manager->priv->workspaces = g_list_append(NULL, g_object_new(XFW_TYPE_WORKSPACE_DUMMY,
                                                                 "group", group,
                                                                 NULL));
}

static void
xfw_workspace_manager_dummy_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceManagerDummy *manager = XFW_WORKSPACE_MANAGER_DUMMY(obj);

    switch (prop_id) {
        case WORKSPACE_MANAGER_PROP_SCREEN:
            manager->priv->screen = g_value_get_object(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_manager_dummy_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceManagerDummy *manager = XFW_WORKSPACE_MANAGER_DUMMY(obj);

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
xfw_workspace_manager_dummy_dispose(GObject *obj) {
    g_list_free_full(XFW_WORKSPACE_MANAGER_DUMMY(obj)->priv->groups, g_object_unref);
    g_list_free_full(XFW_WORKSPACE_MANAGER_DUMMY(obj)->priv->workspaces, g_object_unref);
}

static GList *
xfw_workspace_manager_dummy_list_workspace_groups(XfwWorkspaceManager *manager) {
    return XFW_WORKSPACE_MANAGER_DUMMY(manager)->priv->groups;
}

XfwWorkspaceManager *
_xfw_workspace_manager_dummy_new(GdkScreen *screen) {
    return g_object_new(XFW_TYPE_WORKSPACE_MANAGER_DUMMY,
                        "screen", screen,
                        NULL);
}
