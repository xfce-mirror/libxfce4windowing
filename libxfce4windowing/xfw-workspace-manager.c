/*
 * Copyright (c) 2022 Brian Tarricone <brian@tarricone.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

/**
 * SECTION:xfw-workspace-manager
 * @title: XfwWorkspaceManager
 * @short_description: An object that manages the workspace groups
 * @stability: Unstable
 * @include: libxfce4windowing/libxfce4windowing.h
 *
 * #XfwWorkspaceManager is used to enumerate and perform actions on the
 * workspace groups present on the parent #XfwScreen.
 *
 * Note that #XfwWorkspaceManager is actually an interface; when obtaining an
 * instance, an instance of a windowing-environment-specific object that
 * implements this interface will be returned.
 **/

#include "config.h"

#include <gdk/gdk.h>

#include "libxfce4windowing-private.h"
#include "xfw-workspace-manager.h"

G_DEFINE_INTERFACE(XfwWorkspaceManager, xfw_workspace_manager, G_TYPE_OBJECT)

static void
xfw_workspace_manager_default_init(XfwWorkspaceManagerIface *iface) {
    /**
     * XfwWorkspaceManager::workspace-group-created:
     * @manager: the object which received the signal.
     * @group: (not nullable): the newly-created #XfwWorkspaceGroup.
     *
     * Emitted when a new workspace group is craeted.
     **/
    g_signal_new("workspace-group-created",
                 XFW_TYPE_WORKSPACE_MANAGER,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_group_created),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE_GROUP);

    /**
     * XfwWorkspaceManager::workspace-group-destroyed:
     * @manager: the object which received the signal.
     * @group: (not nullable): the recently-destroyed #XfwWorkspaceGroup.
     *
     * Emitted when a workspace group is destroyed.
     **/
    g_signal_new("workspace-group-destroyed",
                 XFW_TYPE_WORKSPACE_MANAGER,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWorkspaceManagerIface, workspace_group_created),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WORKSPACE_GROUP);

    /**
     * XfwWorkspaceManager:screen:
     *
     * The #GdkScreen instance used to construct the #XfwScreen that owns this
     * workspace manager.
     **/
    g_object_interface_install_property(iface,
                                        g_param_spec_object("screen",
                                                            "screen",
                                                            "screen",
                                                            GDK_TYPE_SCREEN,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * xfw_workspace_manager_list_workspace_groups:
 * @manager: an #XfwWorkspaceManager.
 *
 * Lists all workspace groups known to the workspace manager.
 *
 * Return value: (nullable) (element-type XfwWorkspaceGroup) (transfer none):
 * the list of #XfwWorkspaceGroup managed by @maanger, or %NULL if there are
 * no workspace groups.  The list and its contents are owned by @manager.
 **/
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
