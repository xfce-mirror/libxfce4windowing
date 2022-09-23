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

#include "libxfce4windowing-private.h"
#include "xfw-workspace-group-wayland.h"
#include "xfw-workspace-manager-wayland.h"
#include "xfw-workspace-manager.h"
#include "xfw-workspace-wayland.h"

enum {
    PROP0,
    PROP_WL_REGISTRY,
    PROP_WL_MANAGER,
};

struct _XfwWorkspaceManagerWaylandPrivate {
    struct wl_registry *wl_registry;
    struct ext_workspace_manager_v1 *handle;
    GdkScreen *screen;
    GList *groups;
};

static void registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id);

static void manager_workspace_group(void *data, struct ext_workspace_manager_v1 *manager, struct ext_workspace_group_handle_v1 *group);
static void manager_done(void *data, struct ext_workspace_manager_v1 *manager);
static void manager_finished(void *data, struct ext_workspace_manager_v1 *manager);

static void xfw_workspace_manager_wayland_manager_init(XfwWorkspaceManagerIface *iface);
static void xfw_workspace_manager_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_workspace_manager_wayland_dispose(GObject *obj);
static GList *xfw_workspace_manager_wayland_list_workspace_groups(XfwWorkspaceManager *manager);


static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};
static const struct ext_workspace_manager_v1_listener manager_listener = {
    .workspace_group = manager_workspace_group,
    .done = manager_done,
    .finished = manager_finished,
};

G_DEFINE_TYPE_WITH_CODE(XfwWorkspaceManagerWayland, xfw_workspace_manager_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWorkspaceManagerWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WORKSPACE_MANAGER,
                                              xfw_workspace_manager_wayland_manager_init))

static void
xfw_workspace_manager_wayland_class_init(XfwWorkspaceManagerWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->set_property = xfw_workspace_manager_wayland_set_property;
    gklass->get_property = xfw_workspace_manager_wayland_get_property;
    gklass->dispose = xfw_workspace_manager_wayland_dispose;

    g_object_class_install_property(gklass,
                                    PROP_WL_REGISTRY,
                                    g_param_spec_pointer("wl-registry",
                                                         "wl-registry",
                                                         "wl-registry",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(gklass,
                                    PROP_WL_MANAGER,
                                    g_param_spec_pointer("wl-manager",
                                                         "wl-manager",
                                                         "wl-manager",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_workspace_manager_install_properties(gklass);
}

static void
xfw_workspace_manager_wayland_manager_init(XfwWorkspaceManagerIface *iface) {
    iface->list_workspace_groups = xfw_workspace_manager_wayland_list_workspace_groups;
}

static void
xfw_workspace_manager_wayland_init(XfwWorkspaceManagerWayland *manager) {
    ext_workspace_manager_v1_add_listener(manager->priv->handle, &manager_listener, manager);
}

static void
xfw_workspace_manager_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(obj);

    switch (prop_id) {
        case PROP_WL_REGISTRY:
            manager->priv->wl_registry = g_value_get_pointer(value);
            break;

        case PROP_WL_MANAGER:
            manager->priv->handle = g_value_get_pointer(value);
            break;

        case WORKSPACE_MANAGER_PROP_SCREEN:
            manager->priv->screen = GDK_SCREEN(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_manager_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(obj);

    switch (prop_id) {
        case PROP_WL_REGISTRY:
            g_value_set_pointer(value, manager->priv->wl_registry);
            break;

        case PROP_WL_MANAGER:
            g_value_set_pointer(value, manager->priv->handle);
            break;

        case WORKSPACE_MANAGER_PROP_SCREEN:
            g_value_set_object(value, manager->priv->screen);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_workspace_manager_wayland_dispose(GObject *obj) {
    XfwWorkspaceManagerWayland *manager = XFW_WORKSPACE_MANAGER_WAYLAND(obj);
    XfwWorkspaceManagerWaylandPrivate *priv = manager->priv;

    g_list_free_full(priv->groups, g_object_unref);

    ext_workspace_manager_v1_destroy(priv->handle);
    wl_registry_destroy(priv->wl_registry);
}

static GList *
xfw_workspace_manager_wayland_list_workspace_groups(XfwWorkspaceManager *manager) {
    XfwWorkspaceManagerWayland *xmanager = XFW_WORKSPACE_MANAGER_WAYLAND(manager);
    return xmanager->priv->groups;
}

static void
registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    struct ext_workspace_manager_v1 **handle = data;

    if (strcmp(interface, ext_workspace_manager_v1_interface.name) == 0) {
        *handle = wl_registry_bind(registry,
                                   id,
                                   &ext_workspace_manager_v1_interface,
                                   MIN((uint32_t)ext_workspace_manager_v1_interface.version, version));
    }
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {
    // FIXME: do we need to do anything here?
}

static void
group_destroyed(XfwWorkspaceGroupWayland *group, XfwWorkspaceManagerWayland *manager) {
    g_signal_handlers_disconnect_by_func(group, group_destroyed, manager);
    manager->priv->groups = g_list_remove(manager->priv->groups, group);
    g_signal_emit_by_name(manager, "workspace-group-destroyed", group);
    g_object_unref(group);
}

static void
manager_workspace_group(void *data, struct ext_workspace_manager_v1 *manager, struct ext_workspace_group_handle_v1 *wl_group) {
    XfwWorkspaceManagerWayland *wmanager = XFW_WORKSPACE_MANAGER_WAYLAND(data);
    XfwWorkspaceGroupWayland *group = g_object_new(XFW_TYPE_WORKSPACE_GROUP_WAYLAND,
                                                   "screen", wmanager->priv->screen,
                                                   "handle", wl_group,
                                                   NULL);
    wmanager->priv->groups = g_list_append(wmanager->priv->groups, group);
    g_signal_connect(group, "destroyed", (GCallback)group_destroyed, wmanager);
    g_signal_emit_by_name(wmanager, "workspace-group-created", group);
}

static void
manager_done(void *data, struct ext_workspace_manager_v1 *manager) {

}

static void
manager_finished(void *data, struct ext_workspace_manager_v1 *manager) {

}

XfwWorkspaceManager *
_xfw_workspace_manager_wayland_new(GdkScreen *screen) {
    GdkDisplay *gdk_display;
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct ext_workspace_manager_v1 *handle = NULL;

    gdk_display = gdk_screen_get_display(screen);
    wl_display = gdk_wayland_display_get_wl_display(GDK_WAYLAND_DISPLAY(gdk_display));
    wl_registry = wl_display_get_registry(wl_display);
    wl_registry_add_listener(wl_registry, &registry_listener, &handle);
    wl_display_roundtrip(wl_display);

    if (handle != NULL) {
        return XFW_WORKSPACE_MANAGER(g_object_new(XFW_TYPE_WORKSPACE_MANAGER_WAYLAND,
                                                  "wl-registry", wl_registry,
                                                  "wl-manager", handle,
                                                  "screen", screen,
                                                  NULL));
    } else {
        g_warning("Your compositor does not support the ext_workspace_manager_v1 protocol");
        wl_registry_destroy(wl_registry);
        return NULL;
    }
}
