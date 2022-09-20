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

#include <string.h>

#include <gdk/gdkwayland.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "protocols/ext-workspace-v1-20220919-client.h"

#include "xfw-workspace-manager-wayland.h"
#include "xfw-workspace-wayland.h"
#include "xfw-workspace-manager.h"

struct _XfwWorkspaceManagerWaylandPrivate {
    GList *groups;
    GList *workspaces;
    GHashTable *wl_workspaces;
};

typedef struct {
    struct wl_registry *wl_registry;
    struct ext_workspace_manager_v1 *ext_workspace_manager_v1;
#if 0
    struct ext_workspace_group_handle_v1 *ext_workspace_group_handle_v1;
    struct ext_workspace_handle_v1 *ext_workspace_handle_v1;
#endif
} WaylandSetup;


static void registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id);

static void manager_workspace_group(void *data, struct ext_workspace_manager_v1 *manager, struct ext_workspace_group_handle_v1 *group);
static void manager_done(void *data, struct ext_workspace_manager_v1 *manager);
static void manager_finished(void *data, struct ext_workspace_manager_v1 *manager);

static void group_capabilities(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_array *capabilities);
static void group_output_enter(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_output *output);
static void group_output_leave(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_output *output);
static void group_workspace(void *data, struct ext_workspace_group_handle_v1 *group, struct ext_workspace_handle_v1 *workspace);
static void group_removed(void *data, struct ext_workspace_group_handle_v1 *group);

static void workspace_name(void *data, struct ext_workspace_handle_v1 *workspace, const char *name);
static void workspace_coordinates(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *coordinates);
static void workspace_state(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *state);
static void workspace_capabilities(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *capabilities);
static void workspace_removed(void *data, struct ext_workspace_handle_v1 *workspace);

static void xfw_workspace_manager_wayland_dispose(GObject *obj);
static void xfw_workspace_manager_wayland_manager_init(XfwWorkspaceManagerIface *iface);
static GList *xfw_workspace_manager_wayland_list_workspaces(XfwWorkspaceManager *manager);


static WaylandSetup *wl_setup = NULL;
static XfwWorkspaceManagerWayland *singleton = NULL;

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};
static const struct ext_workspace_manager_v1_listener manager_listener = {
    .workspace_group = manager_workspace_group,
    .done = manager_done,
    .finished = manager_finished,
};
static const struct ext_workspace_group_handle_v1_listener group_listener = {
    .capabilities = group_capabilities,
    .output_enter = group_output_enter,
    .output_leave = group_output_leave,
    .workspace = group_workspace,
    .removed = group_removed,
};
static const struct ext_workspace_handle_v1_listener workspace_listener = {
    .name = workspace_name,
    .coordinates = workspace_coordinates,
    .state = workspace_state,
    .capabilities = workspace_capabilities,
    .removed = workspace_removed,
};


G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceManagerWayland, xfw_workspace_manager_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceManagerWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_MANAGER,
                                              xfw_workspace_manager_wayland_manager_init))

static void
xfw_workspace_manager_wayland_class_init(XfwWorkspaceManagerWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->dispose = xfw_workspace_manager_wayland_dispose;
}

static void
xfw_workspace_manager_wayland_manager_init(XfwWorkspaceManagerIface *iface) {
    iface->list_workspaces = xfw_workspace_manager_wayland_list_workspaces;
}

static void
xfw_workspace_manager_wayland_init(XfwWorkspaceManagerWayland *manager) {
    XfwWorkspaceManagerWaylandPrivate *priv = manager->priv;
    priv->wl_workspaces = g_hash_table_new_full(g_direct_hash, g_direct_equal, g_object_unref, g_object_unref);
}

static void
xfw_workspace_manager_wayland_dispose(GObject *obj) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(obj);
    XfwWorkspaceManagerWaylandPrivate *priv = manager->priv;
    for (GList *l = priv->workspaces; l != NULL; l = l->next) {
        struct ext_workspace_handle_v1 *handle = NULL;
        g_object_get(l->data, "handle", &handle, NULL);
        if (handle != NULL) {
            ext_workspace_handle_v1_destroy(handle);
        }
    }
    g_list_free(priv->workspaces);
    g_hash_table_destroy(priv->wl_workspaces);
    for (GList *l = priv->groups; l != NULL; l = l->next) {
        ext_workspace_group_handle_v1_destroy((struct ext_workspace_group_handle_v1 *)l->data);
    }
    g_list_free(priv->groups);
}

static GList *
xfw_workspace_manager_wayland_list_workspaces(XfwWorkspaceManager *manager) {
    XfwWorkspaceManagerWayland *xmanager = XFW_WORKSPACE_MANAGER_WAYLAND(manager);
    return xmanager->priv->workspaces;
}

static void
registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    WaylandSetup *setup = (WaylandSetup *)data;

    if (strcmp(interface, ext_workspace_manager_v1_interface.name) == 0) {
        setup->ext_workspace_manager_v1 = wl_registry_bind(setup->wl_registry,
                                                           id,
                                                           &ext_workspace_manager_v1_interface,
                                                           MIN((uint32_t)ext_workspace_manager_v1_interface.version, version));
    }
#if 0
    else if (strcmp(interface, ext_workspace_group_handle_v1_interface.name) == 0) {
        setup->ext_workspace_group_handle_v1 = wl_registry_bind(setup->wl_registry,
                                                                id,
                                                                &ext_workspace_group_handle_v1_interface,
                                                                MIN((uint32_t)ext_workspace_group_handle_v1_interface.version, version));
    } else if (strcmp(interface, ext_workspace_handle_v1_interface.name) == 0) {
        setup->ext_workspace_handle_v1 = wl_registry_bind(setup->wl_registry,
                                                          id,
                                                          &ext_workspace_handle_v1_interface,
                                                          MIN((uint32_t)ext_workspace_handle_v1_interface.version, version));
    }
#endif
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {

}

static void
manager_workspace_group(void *data, struct ext_workspace_manager_v1 *manager, struct ext_workspace_group_handle_v1 *group) {
    ext_workspace_group_handle_v1_add_listener(group, &group_listener, data);
}

static void
manager_done(void *data, struct ext_workspace_manager_v1 *manager) {

}

static void
manager_finished(void *data, struct ext_workspace_manager_v1 *manager) {

}

static void
group_capabilities(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_array *capabilities) {

}

static void
group_output_enter(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_output *output) {

}

static void
group_output_leave(void *data, struct ext_workspace_group_handle_v1 *group, struct wl_output *output) {

}

static void
group_workspace(void *data, struct ext_workspace_group_handle_v1 *group, struct ext_workspace_handle_v1 *wl_workspace) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(data);
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(g_object_new(XFW_TYPE_WORKSPACE_WAYLAND, "handle", wl_workspace, NULL));
    g_hash_table_insert(manager->priv->wl_workspaces, wl_workspace, workspace);
    manager->priv->workspaces = g_list_append(manager->priv->workspaces, workspace);
    ext_workspace_handle_v1_add_listener(wl_workspace, &workspace_listener, manager);
    g_signal_emit_by_name(manager, "workspace-added", workspace);
}

static void
group_removed(void *data, struct ext_workspace_group_handle_v1 *group) {
    // TODO: remove all workspaces that are in this group?  or will we
    // get workspace_removed for these first?
    ext_workspace_group_handle_v1_destroy(group);
}

static void
workspace_name(void *data, struct ext_workspace_handle_v1 *wl_workspace, const char *name) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(data);
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(g_hash_table_lookup(manager->priv->wl_workspaces, wl_workspace));
    if (workspace != NULL) {
        g_object_set(workspace, "name", name, NULL);
        g_signal_emit_by_name(workspace, "name-changed");
    }
}

static void
workspace_coordinates(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *coordinates)
{

}

static void
workspace_state(void *data, struct ext_workspace_handle_v1 *wl_workspace, struct wl_array *wl_state) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(data);
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(g_hash_table_lookup(manager->priv->wl_workspaces, wl_workspace));
    if (workspace != NULL) {
        XfwWorkspaceState state = XFW_WORKSPACE_STATE_NONE;
        enum ext_workspace_handle_v1_state *item;
        wl_array_for_each(item, wl_state) {
            switch (*item) {
                case EXT_WORKSPACE_HANDLE_V1_STATE_ACTIVE:
                    state |= XFW_WORKSPACE_STATE_ACTIVE;
                    break;
                case EXT_WORKSPACE_HANDLE_V1_STATE_URGENT:
                    state |= XFW_WORKSPACE_STATE_URGENT;
                    break;
                case EXT_WORKSPACE_HANDLE_V1_STATE_HIDDEN:
                    state |= XFW_WORKSPACE_STATE_HIDDEN;
                    break;
                default:
                    g_warning("Unrecognized workspace state %d", *item);
                    break;
            }
        }
        g_object_set(workspace, "state", state, NULL);
        
        if ((state & XFW_WORKSPACE_STATE_ACTIVE) == XFW_WORKSPACE_STATE_ACTIVE) {
            g_signal_emit_by_name(manager, "workspace-activated", workspace);
        }
        g_signal_emit_by_name(workspace, "state-changed");
    }
}

static void
workspace_capabilities(void *data, struct ext_workspace_handle_v1 *workspace, struct wl_array *capabilities) {

}

static void
workspace_removed(void *data, struct ext_workspace_handle_v1 *wl_workspace) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(data);
    XfwWorkspaceWayland *workspace = XFW_WORKSPACE_WAYLAND(g_hash_table_lookup(manager->priv->wl_workspaces, wl_workspace));
    if (workspace != NULL) {
        g_object_ref(workspace);
        g_hash_table_remove(manager->priv->wl_workspaces, wl_workspace);
        manager->priv->workspaces = g_list_remove(manager->priv->workspaces, workspace);
        g_signal_emit_by_name(manager, "workspace-removed", workspace);
        g_object_unref(workspace);
    }
    ext_workspace_handle_v1_destroy(wl_workspace);
}

XfwWorkspaceManagerWayland *
_xfw_workspace_manager_wayland_get(void) {
    if (wl_setup == NULL) {
        GdkDisplay *gdk_display;
        struct wl_display *wl_display;

        wl_setup = g_new0(WaylandSetup, 1);

        gdk_display = gdk_display_get_default();
        wl_display = gdk_wayland_display_get_wl_display(GDK_WAYLAND_DISPLAY(gdk_display));
        wl_setup->wl_registry = wl_display_get_registry(wl_display);
        wl_registry_add_listener(wl_setup->wl_registry, &registry_listener, wl_setup);
        wl_display_roundtrip(wl_display);

        if (wl_setup->ext_workspace_manager_v1 == NULL) {
            g_warning("Your compositor does not support the ext_workspace_manager_v1 protocol");
        }
#if 0
        if (wl_setup->ext_workspace_group_handle_v1 == NULL) {
            g_warning("Your compositor does not support the ext_workspace_group_handle_v1 protocol");
        }
        if (wl_setup->ext_workspace_handle_v1 == NULL) {
            g_warning("Your compositor does not support the ext_workspace_handle_v1 protocol");
        }
#endif
    }

    if (singleton == NULL
        && wl_setup->ext_workspace_manager_v1 != NULL
#if 0
        && wl_setup->ext_workspace_group_handle_v1 != NULL
        && wl_setup->ext_workspace_handle_v1 != NULL
#endif
    ) {
        singleton = XFW_WORKSPACE_MANAGER_WAYLAND(g_object_new(XFW_TYPE_WORKSPACE_MANAGER_WAYLAND, NULL));
        ext_workspace_manager_v1_add_listener(wl_setup->ext_workspace_manager_v1, &manager_listener, singleton);
    }

    return singleton;
}
