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

#include "xfw-workspace-manager.h"
#include "xfw-workspace-manager-dummy.h"
#ifdef ENABLE_WAYLAND
#include "xfw-workspace-manager-wayland.h"
#endif
#ifdef ENABLE_X11
#include "xfw-workspace-manager-x11.h"
#endif
#include "xfw-util.h"

enum {
    WORKSPACE_ADDED = 0,
    WORKSPACE_ACTIVATED,
    WORKSPACE_REMOVED,

    LAST_SIGNAL,
};

static guint manager_signals[LAST_SIGNAL] = { 0, };

typedef struct _XfwWorkspaceManagerIface XfwWorkspaceManagerInterface;
G_DEFINE_INTERFACE(XfwWorkspaceManager, xfw_workspace_manager, G_TYPE_OBJECT)

static void
xfw_workspace_manager_default_init(XfwWorkspaceManagerIface *iface) {
    manager_signals[WORKSPACE_ADDED] = g_signal_new("workspace-added",
                                                    XFW_TYPE_WORKSPACE_MANAGER,
                                                    G_SIGNAL_RUN_LAST,
                                                    G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_added),
                                                    NULL, NULL,
                                                    g_cclosure_marshal_VOID__OBJECT,
                                                    G_TYPE_NONE, 1,
                                                    XFW_TYPE_WORKSPACE);
    manager_signals[WORKSPACE_ACTIVATED] = g_signal_new("workspace-activated",
                                                        XFW_TYPE_WORKSPACE_MANAGER,
                                                        G_SIGNAL_RUN_LAST,
                                                        G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_added),
                                                        NULL, NULL,
                                                        g_cclosure_marshal_VOID__OBJECT,
                                                        G_TYPE_NONE, 1,
                                                        XFW_TYPE_WORKSPACE);
    manager_signals[WORKSPACE_REMOVED] = g_signal_new("workspace-removed",
                                                      XFW_TYPE_WORKSPACE_MANAGER,
                                                      G_SIGNAL_RUN_LAST,
                                                      G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_added),
                                                      NULL, NULL,
                                                      g_cclosure_marshal_VOID__OBJECT,
                                                      G_TYPE_NONE, 1,
                                                      XFW_TYPE_WORKSPACE);
}

XfwWorkspaceManager *
xfw_workspace_manager_create(GdkScreen *screen) {
#ifdef ENABLE_X11
    if (xfw_windowing_get() == XFW_WINDOWING_X11) {
        return XFW_WORKSPACE_MANAGER(g_object_new(XFW_TYPE_WORKSPACE_MANAGER_X11, "screen", screen, NULL));
    } else
#endif
#ifdef ENABLE_WAYLAND
    if (xfw_windowing_get() == XFW_WINDOWING_WAYLAND) {
        XfwWorkspaceManagerWayland *manager = _xfw_workspace_manager_wayland_get();
        if (manager != NULL) {
            return XFW_WORKSPACE_MANAGER(g_object_ref(manager));
        } else {
            return XFW_WORKSPACE_MANAGER(g_object_ref(_xfw_workspace_manager_dummy_get()));
        }
    } else
#endif
    {
        return XFW_WORKSPACE_MANAGER(g_object_ref(_xfw_workspace_manager_dummy_get()));
    }
}

GList *
xfw_workspace_manager_list_workspaces(XfwWorkspaceManager *manager) {
    XfwWorkspaceManagerIface *iface;
    g_return_val_if_fail(XFW_IS_WORKSPACE_MANAGER(manager), NULL);
    iface = XFW_WORKSPACE_MANAGER_GET_IFACE(manager);
    return (*iface->list_workspaces)(manager);
}
