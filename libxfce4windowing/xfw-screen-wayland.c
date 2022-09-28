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

#include <gdk/gdkwayland.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-screen-wayland.h"
#include "xfw-screen.h"
#include "xfw-window-wayland.h"
#include "xfw-workspace-manager-dummy.h"
#include "xfw-workspace-manager-wayland.h"

struct _XfwScreenWaylandPrivate {
    GdkScreen *gdk_screen;
    struct wl_registry *wl_registry;
    struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager;
    XfwWorkspaceManager *workspace_manager;
    GList *windows;
    GList *windows_stacked;
    GHashTable *wl_windows;
    XfwWindow *active_window;
};

static void xfw_screen_wayland_screen_init(XfwScreenIface *iface);
static void xfw_screen_wayland_constructed(GObject *obj);
static void xfw_screen_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_screen_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_screen_wayland_finalize(GObject *obj);
static gint xfw_screen_wayland_get_number(XfwScreen *screen);
static XfwWorkspaceManager *xfw_screen_wayland_get_workspace_manager(XfwScreen *screen);
static GList *xfw_screen_wayland_get_windows(XfwScreen *screen);
static GList *xfw_screen_wayland_get_windows_stacked(XfwScreen *screen);
static XfwWindow *xfw_screen_wayland_get_active_window(XfwScreen *screen);

static void registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id);

static void toplevel_manager_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);
static void toplevel_manager_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager);

const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};
const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
    .toplevel = toplevel_manager_toplevel,
    .finished = toplevel_manager_finished,
};

G_DEFINE_TYPE_WITH_CODE(XfwScreenWayland, xfw_screen_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwScreenWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_SCREEN,
                                              xfw_screen_wayland_screen_init))

static void
xfw_screen_wayland_class_init(XfwScreenWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_screen_wayland_constructed;
    gklass->set_property = xfw_screen_wayland_set_property;
    gklass->get_property = xfw_screen_wayland_get_property;
    gklass->finalize = xfw_screen_wayland_finalize;

    _xfw_screen_install_properties(gklass);
}

static void
xfw_screen_wayland_init(XfwScreenWayland *screen) {
    screen->priv = xfw_screen_wayland_get_instance_private(screen);
}

static void xfw_screen_wayland_constructed(GObject *obj) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);
    GdkDisplay *gdk_display;
    struct wl_display *wl_display;

    gdk_display = gdk_screen_get_display(screen->priv->gdk_screen);
    wl_display = gdk_wayland_display_get_wl_display(GDK_WAYLAND_DISPLAY(gdk_display));
    screen->priv->wl_registry = wl_display_get_registry(wl_display);
    wl_registry_add_listener(screen->priv->wl_registry, &registry_listener, screen);
    wl_display_roundtrip(wl_display);

    if (screen->priv->toplevel_manager != NULL) {
        screen->priv->wl_windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
        zwlr_foreign_toplevel_manager_v1_add_listener(screen->priv->toplevel_manager, &toplevel_manager_listener, screen);
    } else {
        g_message("Your compositor does not support wlr_foreign_toplevel_manager_v1 protocol");
        wl_registry_destroy(screen->priv->wl_registry);
        screen->priv->wl_registry = NULL;
    }

    screen->priv->workspace_manager = _xfw_workspace_manager_wayland_new(screen->priv->gdk_screen);
    if (screen->priv->workspace_manager == NULL) {
        screen->priv->workspace_manager = _xfw_workspace_manager_dummy_new(screen->priv->gdk_screen);
    }
}

static void
xfw_screen_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);

    switch (prop_id) {
        case SCREEN_PROP_SCREEN:
            screen->priv->gdk_screen = g_value_get_object(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);

    switch (prop_id) {
        case SCREEN_PROP_SCREEN:
            g_value_set_object(value, screen->priv->gdk_screen);
            break;

        case SCREEN_PROP_WORKSPACE_MANAGER:
            g_value_set_object(value, screen->priv->workspace_manager);
            break;

        case SCREEN_PROP_ACTIVE_WINDOW:
            g_value_set_object(value, xfw_screen_wayland_get_active_window(XFW_SCREEN(screen)));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_wayland_finalize(GObject *obj) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);

    g_object_unref(screen->priv->workspace_manager);

    if (screen->priv->toplevel_manager != NULL) {
        zwlr_foreign_toplevel_manager_v1_destroy(screen->priv->toplevel_manager);
    }
    if (screen->priv->wl_registry != NULL) {
        wl_registry_destroy(screen->priv->wl_registry);
    }
    g_list_free(screen->priv->windows);
    g_list_free(screen->priv->windows_stacked);
    g_hash_table_destroy(screen->priv->wl_windows);

    G_OBJECT_CLASS(xfw_screen_wayland_parent_class)->finalize(obj);
}

static void
xfw_screen_wayland_screen_init(XfwScreenIface *iface) {
    iface->get_number = xfw_screen_wayland_get_number;
    iface->get_workspace_manager = xfw_screen_wayland_get_workspace_manager;
    iface->get_windows = xfw_screen_wayland_get_windows;
    iface->get_windows_stacked = xfw_screen_wayland_get_windows_stacked;
    iface->get_active_window = xfw_screen_wayland_get_active_window;
}

static gint
xfw_screen_wayland_get_number(XfwScreen *screen) {
    return 0;
}

static XfwWorkspaceManager *
xfw_screen_wayland_get_workspace_manager(XfwScreen *screen) {
    return XFW_SCREEN_WAYLAND(screen)->priv->workspace_manager;
}

static GList *xfw_screen_wayland_get_windows(XfwScreen *screen) {
    return XFW_SCREEN_WAYLAND(screen)->priv->windows;
}

static GList *xfw_screen_wayland_get_windows_stacked(XfwScreen *screen) {
    g_message("Wayland does not support discovering window stacking; windows returned are unordered");
    return XFW_SCREEN_WAYLAND(screen)->priv->windows_stacked;
}

static XfwWindow *xfw_screen_wayland_get_active_window(XfwScreen *screen) {
    return XFW_SCREEN_WAYLAND(screen)->priv->active_window;
}

static void
registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(data);

    if (strcmp(zwlr_foreign_toplevel_manager_v1_interface.name, interface) == 0) {
        screen->priv->toplevel_manager = wl_registry_bind(screen->priv->wl_registry,
                                                          id,
                                                          &zwlr_foreign_toplevel_manager_v1_interface,
                                                          MIN((uint32_t)zwlr_foreign_toplevel_manager_v1_interface.version, version));
    }
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {
    // XXX: do we need to do something here?
}

static void
window_closed(XfwWindowWayland *window, XfwScreenWayland *screen) {
    g_object_ref(window);
    g_signal_handlers_disconnect_by_func(window, window_closed, screen);
    screen->priv->windows = g_list_remove(screen->priv->windows, window);
    screen->priv->windows_stacked = g_list_remove(screen->priv->windows_stacked, window);
    g_hash_table_remove(screen->priv->wl_windows, _xfw_window_wayland_get_handle(window));
    g_signal_emit_by_name(screen, "window-closed", window);
    g_object_unref(window);
}

static void
toplevel_manager_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(data);
    XfwWindowWayland *window = g_object_new(XFW_TYPE_WINDOW_WAYLAND,
                                            "screen", screen,
                                            "handle", wl_toplevel,
                                            NULL);
    screen->priv->windows = g_list_prepend(screen->priv->windows, window);
    screen->priv->windows_stacked = g_list_prepend(screen->priv->windows_stacked, window);
    g_hash_table_insert(screen->priv->wl_windows, wl_toplevel, window);
    g_signal_connect(window, "closed", G_CALLBACK(window_closed), screen);
}

static void
toplevel_manager_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(data);
    zwlr_foreign_toplevel_manager_v1_destroy(screen->priv->toplevel_manager);
    screen->priv->toplevel_manager = NULL;
}

void
_xfw_screen_wayland_set_active_window(XfwScreenWayland *screen, XfwWindow *window) {
    if (screen->priv->active_window != window) {
        screen->priv->active_window = window;
        g_object_notify(G_OBJECT(screen), "active-window");
        g_signal_emit_by_name(screen, "active-window-changed", window);
    }
}
XfwWorkspace *
_xfw_screen_wayland_get_window_workspace(XfwScreenWayland *screen, XfwWindow *window) {
    if (XFW_IS_WORKSPACE_MANAGER_DUMMY(screen->priv->workspace_manager)) {
        XfwWorkspaceGroup *group = XFW_WORKSPACE_GROUP(xfw_workspace_manager_list_workspace_groups(screen->priv->workspace_manager)->data);
        return XFW_WORKSPACE(xfw_workspace_group_list_workspaces(group)->data);
    } else {
        g_message("Window<->Workspace association is not available on Wayland");
        return NULL;
    }
}
