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

#include <gdk/gdk.h>

#include "libxfce4windowing-private.h"
#include "xfw-workspace-manager.h"
#include "xfw-workspace-manager-dummy.h"
#ifdef ENABLE_WAYLAND
#include "xfw-workspace-manager-wayland.h"
#endif
#ifdef ENABLE_X11
#include "xfw-workspace-manager-x11.h"
#endif
#include "xfw-util.h"

static GHashTable *managers = NULL;

typedef struct _XfwWorkspaceManagerIface XfwWorkspaceManagerInterface;
G_DEFINE_INTERFACE(XfwWorkspaceManager, xfw_workspace_manager, G_TYPE_OBJECT)

static void
xfw_workspace_manager_default_init(XfwWorkspaceManagerIface *iface) {
    if (managers == NULL) {
        managers = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    }

    g_signal_new("workspace-group-added",
                 XFW_TYPE_WORKSPACE_MANAGER,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_group_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE_GROUP);
    g_signal_new("workspace-group-removed",
                 XFW_TYPE_WORKSPACE_MANAGER,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_group_added),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE_GROUP);

    g_object_interface_install_property(iface,
                                        g_param_spec_object("screen",
                                                            "screen",
                                                            "screen",
                                                            GDK_TYPE_SCREEN,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
screen_destroyed(gpointer data, GObject *screen) {
    g_hash_table_remove(managers, screen);
}

static void
manager_destroyed(gpointer screen, GObject *manager) {
    g_hash_table_remove(managers, screen);
}

XfwWorkspaceManager *
xfw_workspace_manager_get(GdkScreen *screen) {
    XfwWorkspaceManager *manager = g_hash_table_lookup(managers, screen);

    if (manager == NULL) {
#ifdef ENABLE_X11
        if (xfw_windowing_get() == XFW_WINDOWING_X11) {
            manager = _xfw_workspace_manager_x11_new(screen);
        } else
#endif
#ifdef ENABLE_WAYLAND
        if (xfw_windowing_get() == XFW_WINDOWING_WAYLAND) {
            manager = _xfw_workspace_manager_wayland_new(screen);
            if (manager == NULL) {
                manager = _xfw_workspace_manager_dummy_new(screen);
            }
        } else
#endif
        {
            manager = _xfw_workspace_manager_dummy_new(screen);
        }

        g_hash_table_insert(managers, screen, manager);
        g_object_weak_ref(G_OBJECT(screen), screen_destroyed, NULL);
        g_object_weak_ref(G_OBJECT(manager), manager_destroyed, screen);
    } else {
        g_object_ref(manager);
    }

    return manager;
}

GList *
xfw_workspace_manager_list_workspace_groups(XfwWorkspaceManager *manager) {
    XfwWorkspaceManagerIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_MANAGER(manager), NULL);
    iface = XFW_WORKSPACE_MANAGER_GET_IFACE(manager);
    return (*iface->list_workspace_groups)(manager);
}

void
_xfw_workspace_manager_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, WORKSPACE_MANAGER_PROP_SCREEN, "screen");
}
