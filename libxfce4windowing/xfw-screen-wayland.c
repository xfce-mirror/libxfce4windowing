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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdkwayland.h>
#include <string.h>
#include <wayland-client.h>

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-monitor-private.h"
#include "xfw-monitor-wayland.h"
#include "xfw-screen-private.h"
#include "xfw-screen-wayland.h"
#include "xfw-util.h"
#include "xfw-window-wayland.h"
#include "xfw-workspace-manager-dummy.h"
#include "xfw-workspace-manager-wayland.h"

struct _XfwScreenWayland {
    XfwScreen parent;

    struct wl_registry *wl_registry;
    struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager;
    struct wl_seat *wl_seat;
    GList *windows;
    GList *windows_stacked;
    GHashTable *wl_windows;
    struct {
        GList *minimized;
        XfwWindow *was_active;
    } show_desktop_data;
};

static void xfw_screen_wayland_constructed(GObject *obj);
static void xfw_screen_wayland_finalize(GObject *obj);
static GList *xfw_screen_wayland_get_windows(XfwScreen *screen);
static GList *xfw_screen_wayland_get_windows_stacked(XfwScreen *screen);
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


G_DEFINE_TYPE(XfwScreenWayland, xfw_screen_wayland, XFW_TYPE_SCREEN)


static void
xfw_screen_wayland_class_init(XfwScreenWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    gklass->constructed = xfw_screen_wayland_constructed;
    gklass->finalize = xfw_screen_wayland_finalize;

    XfwScreenClass *screen_class = XFW_SCREEN_CLASS(klass);
    screen_class->get_windows = xfw_screen_wayland_get_windows;
    screen_class->get_windows_stacked = xfw_screen_wayland_get_windows_stacked;
    screen_class->set_show_desktop = xfw_screen_wayland_set_show_desktop;
}

static void
xfw_screen_wayland_init(XfwScreenWayland *screen) {}

static void
xfw_screen_wayland_constructed(GObject *obj) {
    XfwScreen *screen = XFW_SCREEN(obj);
    XfwScreenWayland *wscreen = XFW_SCREEN_WAYLAND(obj);

    G_OBJECT_CLASS(xfw_screen_wayland_parent_class)->constructed(obj);

    _xfw_screen_set_workspace_manager(screen, _xfw_workspace_manager_wayland_new(screen));

    GdkDisplay *gdk_display = gdk_screen_get_display(_xfw_screen_get_gdk_screen(screen));
    struct wl_display *wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    wscreen->wl_registry = wl_display_get_registry(wl_display);
    wl_registry_add_listener(wscreen->wl_registry, &registry_listener, wscreen);
    wl_display_roundtrip(wl_display);

    if (wscreen->toplevel_manager != NULL) {
        wscreen->wl_windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
        zwlr_foreign_toplevel_manager_v1_add_listener(wscreen->toplevel_manager, &toplevel_manager_listener, wscreen);
    } else {
        g_message("Your compositor does not support wlr_foreign_toplevel_manager_v1 protocol");
        wl_registry_destroy(wscreen->wl_registry);
        wscreen->wl_registry = NULL;
    }

    _xfw_monitor_wayland_init(wscreen);
}

static void
xfw_screen_wayland_finalize(GObject *obj) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);

    if (screen->toplevel_manager != NULL) {
        zwlr_foreign_toplevel_manager_v1_destroy(screen->toplevel_manager);
    }
    if (screen->wl_seat != NULL) {
        wl_seat_release(screen->wl_seat);
    }
    if (screen->wl_registry != NULL) {
        wl_registry_destroy(screen->wl_registry);
    }
    g_list_free(screen->windows);
    g_list_free(screen->windows_stacked);
    if (screen->wl_windows != NULL) {
        g_hash_table_destroy(screen->wl_windows);
    }
    g_list_free(screen->show_desktop_data.minimized);

    G_OBJECT_CLASS(xfw_screen_wayland_parent_class)->finalize(obj);
}

static GList *
xfw_screen_wayland_get_windows(XfwScreen *screen) {
    return XFW_SCREEN_WAYLAND(screen)->windows;
}

static GList *
xfw_screen_wayland_get_windows_stacked(XfwScreen *screen) {
    _xfw_g_message_once("Wayland does not support discovering window stacking; windows returned are unordered");
    return XFW_SCREEN_WAYLAND(screen)->windows_stacked;
}

static void
show_desktop_state_changed(XfwWindow *window, XfwWindowState changed_mask, XfwWindowState new_state, XfwScreenWayland *wscreen) {
    if (!(changed_mask & XFW_WINDOW_STATE_MINIMIZED)) {
        return;
    }

    if (new_state & XFW_WINDOW_STATE_MINIMIZED) {
        wscreen->show_desktop_data.minimized = g_list_prepend(wscreen->show_desktop_data.minimized, window);
    } else {
        show_desktop_disconnect(window, wscreen);
        wscreen->show_desktop_data.minimized = g_list_remove(wscreen->show_desktop_data.minimized, window);
        if (wscreen->show_desktop_data.minimized == NULL) {
            XfwScreen *screen = XFW_SCREEN(wscreen);
            if (xfw_screen_get_show_desktop(screen)) {
                _xfw_screen_set_show_desktop(screen, FALSE);
            }
            if (wscreen->show_desktop_data.was_active != NULL) {
                xfw_window_activate(wscreen->show_desktop_data.was_active, 0, NULL);
            }
        }
    }
}

static void
show_desktop_closed(XfwWindow *window, XfwScreenWayland *wscreen) {
    XfwScreen *screen = XFW_SCREEN(wscreen);
    wscreen->show_desktop_data.minimized = g_list_remove(wscreen->show_desktop_data.minimized, window);
    if (wscreen->show_desktop_data.minimized == NULL) {
        _xfw_screen_set_show_desktop(XFW_SCREEN(screen), FALSE);
    }
}

static void
xfw_screen_wayland_set_show_desktop(XfwScreen *screen, gboolean show) {
    XfwScreenWayland *wscreen = XFW_SCREEN_WAYLAND(screen);
    gboolean revert = TRUE;

    _xfw_screen_set_show_desktop(screen, !!show);

    // unminimize previously minimized windows
    if (!show) {
        for (GList *lp = wscreen->show_desktop_data.minimized; lp != NULL; lp = lp->next) {
            xfw_window_set_minimized(lp->data, FALSE, NULL);
        }
        return;
    }

    // remove and disconnect from any previously minimized window: probably there is none,
    // but it is asynchronous and the compositor might have failed to unminimize some of them
    g_list_foreach(wscreen->show_desktop_data.minimized, show_desktop_disconnect, wscreen);
    g_list_free(wscreen->show_desktop_data.minimized);
    wscreen->show_desktop_data.minimized = NULL;
    wscreen->show_desktop_data.was_active = NULL;

    // request for showing the desktop and prepare reverse process
    for (GList *lp = xfw_screen_wayland_get_windows(screen); lp != NULL; lp = lp->next) {
        XfwWindowState state = xfw_window_get_state(lp->data);
        if (!(state & XFW_WINDOW_STATE_MINIMIZED)) {
            revert = FALSE;
            g_signal_connect(lp->data, "state-changed", G_CALLBACK(show_desktop_state_changed), wscreen);
            g_signal_connect(lp->data, "closed", G_CALLBACK(show_desktop_closed), wscreen);
            if (state & XFW_WINDOW_STATE_ACTIVE) {
                wscreen->show_desktop_data.was_active = lp->data;
            }
            xfw_window_set_minimized(lp->data, TRUE, NULL);
        }
    }

    // there was no window to minimize, revert state
    if (revert) {
        _xfw_screen_set_show_desktop(screen, FALSE);
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
        screen->toplevel_manager = wl_registry_bind(screen->wl_registry,
                                                    id,
                                                    &zwlr_foreign_toplevel_manager_v1_interface,
                                                    MIN((uint32_t)zwlr_foreign_toplevel_manager_v1_interface.version, version));
    } else if (strcmp(wl_seat_interface.name, interface) == 0) {
        if (screen->wl_seat != NULL) {
            g_debug("We already had a wl_seat, but now we're getting a new one");
            wl_seat_release(screen->wl_seat);
        }
        screen->wl_seat = wl_registry_bind(screen->wl_registry,
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
window_closed(XfwWindowWayland *window, XfwScreenWayland *wscreen) {
    g_object_ref(window);
    g_signal_handlers_disconnect_by_func(window, window_closed, wscreen);
    wscreen->windows = g_list_remove(wscreen->windows, window);
    wscreen->windows_stacked = g_list_remove(wscreen->windows_stacked, window);
    g_hash_table_remove(wscreen->wl_windows, _xfw_window_wayland_get_handle(window));
    g_signal_emit_by_name(wscreen, "window-closed", window);

    XfwScreen *screen = XFW_SCREEN(wscreen);
    if (XFW_WINDOW(window) == xfw_screen_get_active_window(screen)) {
        _xfw_screen_set_active_window(screen, NULL);
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
    screen->windows = g_list_prepend(screen->windows, window);
    screen->windows_stacked = g_list_prepend(screen->windows_stacked, window);
    g_hash_table_insert(screen->wl_windows, wl_toplevel, window);
    g_signal_connect(window, "closed", G_CALLBACK(window_closed), screen);
}

static void
toplevel_manager_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(data);
    zwlr_foreign_toplevel_manager_v1_destroy(screen->toplevel_manager);
    screen->toplevel_manager = NULL;
}

struct wl_seat *
_xfw_screen_wayland_get_wl_seat(XfwScreenWayland *screen) {
    return screen->wl_seat;
}

XfwWorkspace *
_xfw_screen_wayland_get_window_workspace(XfwScreenWayland *screen, XfwWindow *window) {
    XfwWorkspaceManager *workspace_manager = xfw_screen_get_workspace_manager(XFW_SCREEN(screen));
    if (XFW_IS_WORKSPACE_MANAGER_DUMMY(workspace_manager)) {
        XfwWorkspaceGroup *group = XFW_WORKSPACE_GROUP(xfw_workspace_manager_list_workspace_groups(workspace_manager)->data);
        return XFW_WORKSPACE(xfw_workspace_group_list_workspaces(group)->data);
    } else {
        _xfw_g_message_once("Window<->Workspace association is not available on Wayland");
        return NULL;
    }
}
