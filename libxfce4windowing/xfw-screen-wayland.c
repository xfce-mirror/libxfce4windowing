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

#include "config.h"

#include <gdk/gdkwayland.h>
#include <string.h>
#include <wayland-client.h>

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-screen-private.h"
#include "xfw-screen-wayland.h"
#include "xfw-window-wayland.h"
#include "xfw-workspace-manager-dummy.h"
#include "xfw-workspace-manager-wayland.h"

struct _XfwScreenWaylandPrivate {
    struct wl_registry *wl_registry;
    struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager;
    struct wl_seat *wl_seat;
    XfwWorkspaceManager *workspace_manager;
    GList *windows;
    GList *windows_stacked;
    GHashTable *wl_windows;
    XfwWindow *active_window;
    guint show_desktop : 1;
    struct {
        GList *minimized;
        XfwWindow *was_active;
    } show_desktop_data;
};

static void xfw_screen_wayland_constructed(GObject *obj);
static void xfw_screen_wayland_finalize(GObject *obj);

static XfwWorkspaceManager *xfw_screen_wayland_get_workspace_manager(XfwScreen *screen);
static GList *xfw_screen_wayland_get_windows(XfwScreen *screen);
static GList *xfw_screen_wayland_get_windows_stacked(XfwScreen *screen);
static XfwWindow *xfw_screen_wayland_get_active_window(XfwScreen *screen);
static gboolean xfw_screen_wayland_get_show_desktop(XfwScreen *screen);
static void xfw_screen_wayland_set_show_desktop(XfwScreen *screen, gboolean show);

static void show_desktop_disconnect(gpointer object, gpointer data);

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


G_DEFINE_TYPE_WITH_PRIVATE(XfwScreenWayland, xfw_screen_wayland, XFW_TYPE_SCREEN)


static void
xfw_screen_wayland_class_init(XfwScreenWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    XfwScreenClass *screen_class = XFW_SCREEN_CLASS(klass);

    gklass->constructed = xfw_screen_wayland_constructed;
    gklass->finalize = xfw_screen_wayland_finalize;

    screen_class->get_workspace_manager = xfw_screen_wayland_get_workspace_manager;
    screen_class->get_windows = xfw_screen_wayland_get_windows;
    screen_class->get_windows_stacked = xfw_screen_wayland_get_windows_stacked;
    screen_class->get_active_window = xfw_screen_wayland_get_active_window;
    screen_class->get_show_desktop = xfw_screen_wayland_get_show_desktop;
    screen_class->set_show_desktop = xfw_screen_wayland_set_show_desktop;
}

static void
xfw_screen_wayland_init(XfwScreenWayland *screen) {
    screen->priv = xfw_screen_wayland_get_instance_private(screen);
}

static void
xfw_screen_wayland_constructed(GObject *obj) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);
    GdkScreen *gscreen = xfw_screen_get_gdk_screen(XFW_SCREEN(screen));
    GdkDisplay *gdk_display;
    struct wl_display *wl_display;

    gdk_display = gdk_screen_get_display(gscreen);
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

    screen->priv->workspace_manager = _xfw_workspace_manager_wayland_new(gscreen);
    if (screen->priv->workspace_manager == NULL) {
        screen->priv->workspace_manager = _xfw_workspace_manager_dummy_new(gscreen);
    }
}


static void
xfw_screen_wayland_finalize(GObject *obj) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);

    g_object_unref(screen->priv->workspace_manager);

    if (screen->priv->toplevel_manager != NULL) {
        zwlr_foreign_toplevel_manager_v1_destroy(screen->priv->toplevel_manager);
    }
    if (screen->priv->wl_seat != NULL) {
        wl_seat_release(screen->priv->wl_seat);
    }
    if (screen->priv->wl_registry != NULL) {
        wl_registry_destroy(screen->priv->wl_registry);
    }
    g_list_free(screen->priv->windows);
    g_list_free(screen->priv->windows_stacked);
    if (screen->priv->wl_windows != NULL) {
        g_hash_table_destroy(screen->priv->wl_windows);
    }
    g_list_free(screen->priv->show_desktop_data.minimized);

    G_OBJECT_CLASS(xfw_screen_wayland_parent_class)->finalize(obj);
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

static gboolean
xfw_screen_wayland_get_show_desktop(XfwScreen *screen) {
    return XFW_SCREEN_WAYLAND(screen)->priv->show_desktop;
}

static void
show_desktop_state_changed(XfwWindow *window, XfwWindowState changed, XfwWindowState new, XfwScreenWayland *screen) {
    if (!(changed & XFW_WINDOW_STATE_MINIMIZED)) {
        return;
    }

    if (new & XFW_WINDOW_STATE_MINIMIZED) {
        screen->priv->show_desktop_data.minimized = g_list_prepend(screen->priv->show_desktop_data.minimized, window);
    } else {
        show_desktop_disconnect(window, screen);
        screen->priv->show_desktop_data.minimized = g_list_remove(screen->priv->show_desktop_data.minimized, window);
        if (screen->priv->show_desktop_data.minimized == NULL) {
            if (screen->priv->show_desktop) {
                screen->priv->show_desktop = FALSE;
                g_object_notify(G_OBJECT(screen), "show-desktop");
            }
            if (screen->priv->show_desktop_data.was_active != NULL) {
                xfw_window_activate(screen->priv->show_desktop_data.was_active, 0, NULL);
            }
        }
    }
}

static void
show_desktop_closed(XfwWindow *window, XfwScreenWayland *screen) {
    screen->priv->show_desktop_data.minimized = g_list_remove(screen->priv->show_desktop_data.minimized, window);
    if (screen->priv->show_desktop_data.minimized == NULL && screen->priv->show_desktop) {
        screen->priv->show_desktop = FALSE;
        g_object_notify(G_OBJECT(screen), "show-desktop");
    }
}

static void
xfw_screen_wayland_set_show_desktop(XfwScreen *screen, gboolean show) {
    XfwScreenWayland *wscreen = XFW_SCREEN_WAYLAND(screen);
    gboolean revert = TRUE;

    if (!!show == wscreen->priv->show_desktop) {
        return;
    }

    wscreen->priv->show_desktop = !!show;
    g_object_notify(G_OBJECT(screen), "show-desktop");

    // unminimize previously minimized windows
    if (!show) {
        for (GList *lp = wscreen->priv->show_desktop_data.minimized; lp != NULL; lp = lp->next) {
            xfw_window_set_minimized(lp->data, FALSE, NULL);
        }
        return;
    }

    // remove and disconnect from any previously minimized window: probably there is none,
    // but it is asynchronous and the compositor might have failed to unminimize some of them
    g_list_foreach(wscreen->priv->show_desktop_data.minimized, show_desktop_disconnect, wscreen);
    g_list_free(wscreen->priv->show_desktop_data.minimized);
    wscreen->priv->show_desktop_data.minimized = NULL;
    wscreen->priv->show_desktop_data.was_active = NULL;

    // request for showing the desktop and prepare reverse process
    for (GList *lp = xfw_screen_wayland_get_windows(screen); lp != NULL; lp = lp->next) {
        XfwWindowState state = xfw_window_get_state(lp->data);
        if (!(state & XFW_WINDOW_STATE_MINIMIZED)) {
            revert = FALSE;
            g_signal_connect(lp->data, "state-changed", G_CALLBACK(show_desktop_state_changed), wscreen);
            g_signal_connect(lp->data, "closed", G_CALLBACK(show_desktop_closed), wscreen);
            if (state & XFW_WINDOW_STATE_ACTIVE) {
                wscreen->priv->show_desktop_data.was_active = lp->data;
            }
            xfw_window_set_minimized(lp->data, TRUE, NULL);
        }
    }

    // there was no window to minimize, revert state
    if (revert) {
        wscreen->priv->show_desktop = FALSE;
        g_object_notify(G_OBJECT(screen), "show-desktop");
    }
}

static void
show_desktop_disconnect(gpointer object, gpointer data) {
    g_signal_handlers_disconnect_by_func(object, show_desktop_state_changed, data);
    g_signal_handlers_disconnect_by_func(object, show_desktop_closed, data);
}

static void
registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(data);

    if (strcmp(zwlr_foreign_toplevel_manager_v1_interface.name, interface) == 0) {
        screen->priv->toplevel_manager = wl_registry_bind(screen->priv->wl_registry,
                                                          id,
                                                          &zwlr_foreign_toplevel_manager_v1_interface,
                                                          MIN((uint32_t)zwlr_foreign_toplevel_manager_v1_interface.version, version));
    } else if (strcmp(wl_seat_interface.name, interface) == 0) {
        if (screen->priv->wl_seat != NULL) {
            g_debug("We already had a wl_seat, but now we're getting a new one");
            wl_seat_release(screen->priv->wl_seat);
        }
        screen->priv->wl_seat = wl_registry_bind(screen->priv->wl_registry,
                                                 id,
                                                 &wl_seat_interface,
                                                 MIN((uint32_t)wl_seat_interface.version, version));
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
    if (XFW_WINDOW(window) == screen->priv->active_window) {
        _xfw_screen_wayland_set_active_window(screen, NULL);
    }
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

struct wl_seat *
_xfw_screen_wayland_get_wl_seat(XfwScreenWayland *screen) {
    return screen->priv->wl_seat;
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
