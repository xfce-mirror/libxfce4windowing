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

#include "xfw-workspace-manager-dummy.h"
#include "xfw-workspace-dummy.h"
#include "xfw-workspace-manager.h"

struct _XfwWorkspaceManagerDummyPrivate {
    GList *workspaces;
};

static void xfw_workspace_manager_dummy_manager_init(XfwWorkspaceManagerIface *iface);
static void xfw_workspace_manager_dummy_dispose(GObject *obj);
static GList *xfw_workspace_manager_dummy_list_workspaces(XfwWorkspaceManager *manager);

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceManagerDummy, xfw_workspace_manager_dummy, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceManagerDummy)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_MANAGER,
                                              xfw_workspace_manager_dummy_manager_init))

static void
xfw_workspace_manager_dummy_class_init(XfwWorkspaceManagerDummyClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    gklass->dispose = xfw_workspace_manager_dummy_dispose;
}

static void
xfw_workspace_manager_dummy_manager_init(XfwWorkspaceManagerIface *iface) {
    iface->list_workspaces = xfw_workspace_manager_dummy_list_workspaces;
}

static void
xfw_workspace_manager_dummy_init(XfwWorkspaceManagerDummy *manager) {
    manager->priv->workspaces = g_list_append(NULL, g_object_new(XFW_TYPE_WORKSPACE_DUMMY, NULL));
}

static void
xfw_workspace_manager_dummy_dispose(GObject *obj) {
    g_list_free(XFW_WORKSPACE_MANAGER_DUMMY(obj)->priv->workspaces);
}

static GList *
xfw_workspace_manager_dummy_list_workspaces(XfwWorkspaceManager *manager) {
    return XFW_WORKSPACE_MANAGER_DUMMY(manager)->priv->workspaces;
}

XfwWorkspaceManager *
_xfw_workspace_manager_dummy_new(GdkScreen *screen) {
    return g_object_new(XFW_TYPE_WORKSPACE_MANAGER_DUMMY, NULL);
}
