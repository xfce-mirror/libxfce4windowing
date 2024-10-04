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

#include "protocols/ext-workspace-v1-20230427-client.h"
#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"
#include "protocols/xdg-output-unstable-v1-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-monitor-private.h"
#include "xfw-monitor-wayland.h"
#include "xfw-screen-private.h"
#include "xfw-screen-wayland.h"
#include "xfw-seat-wayland.h"
#include "xfw-util.h"
#include "xfw-window-wayland.h"
#include "xfw-workspace-manager-dummy.h"
#include "xfw-workspace-manager-wayland.h"

struct _XfwScreenWayland {
    XfwScreen parent;

    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    GList *async_roundtrips;

    GList *pending_seats;

    gboolean defer_toplevel_manager;
    uint32_t toplevel_manager_id;
    uint32_t toplevel_manager_version;
    struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager;

    GList *windows;
    GList *windows_stacked;
    GHashTable *wl_windows;
    struct {
        GList *minimized;
        XfwWindow *was_active;
    } show_desktop_data;

    XfwMonitorManagerWayland *monitor_manager;
};

static void xfw_screen_wayland_constructed(GObject *obj);
static void xfw_screen_wayland_finalize(GObject *obj);
static GList *xfw_screen_wayland_get_windows(XfwScreen *screen);
static GList *xfw_screen_wayland_get_windows_stacked(XfwScreen *screen);
static void xfw_screen_wayland_set_show_desktop(XfwScreen *screen, gboolean show);

static void show_desktop_disconnect(gpointer object, gpointer data);

static void async_roundtrip_done(void *data, struct wl_callback *callback, uint32_t callback_id);

static void init_toplevel_manager(XfwScreenWayland *screen);

static void registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t id);

static void toplevel_manager_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);
static void toplevel_manager_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *wl_manager);

static const struct wl_callback_listener callback_listener = {
    .done = async_roundtrip_done,
};
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};
static const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
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
xfw_screen_wayland_init(XfwScreenWayland *screen) {
    screen->defer_toplevel_manager = TRUE;
    screen->wl_windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
}

static void
xfw_screen_wayland_constructed(GObject *obj) {
    XfwScreen *screen = XFW_SCREEN(obj);
    XfwScreenWayland *wscreen = XFW_SCREEN_WAYLAND(obj);

    G_OBJECT_CLASS(xfw_screen_wayland_parent_class)->constructed(obj);

    wscreen->monitor_manager = _xfw_monitor_manager_wayland_new(wscreen);

    GdkDisplay *gdk_display = gdk_screen_get_display(_xfw_screen_get_gdk_screen(screen));
    wscreen->wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    wscreen->wl_registry = wl_display_get_registry(wscreen->wl_display);
    wl_registry_add_listener(wscreen->wl_registry, &registry_listener, wscreen);

    wl_display_roundtrip(wscreen->wl_display);
    while (wscreen->async_roundtrips != NULL) {
        wl_display_dispatch(wscreen->wl_display);
    }

    // We defer binding to the toplevel manager until after we have all
    // XfwMonitor instances initialized.  Otherwise, we would get output_enter
    // events for toplevels, but have no XfwMonitor to match them to.
    wscreen->defer_toplevel_manager = FALSE;
    if (wscreen->toplevel_manager_id != 0 && wscreen->toplevel_manager_version != 0) {
        init_toplevel_manager(wscreen);
    }

    if (wscreen->toplevel_manager != NULL) {
        while (wscreen->async_roundtrips != NULL) {
            wl_display_dispatch(wscreen->wl_display);
        }
    } else {
        g_message("Your compositor does not support the wlr_foreign_toplevel_manager_v1 protocol");
    }

    if (xfw_screen_get_workspace_manager(XFW_SCREEN(screen)) == NULL) {
        g_message("Your compositor does not support the ext_workspace_manager_v1 protocol");
        _xfw_screen_set_workspace_manager(XFW_SCREEN(screen), _xfw_workspace_manager_dummy_new(screen));
    }
}

static void
xfw_screen_wayland_finalize(GObject *obj) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(obj);

    g_list_free_full(screen->async_roundtrips, (GDestroyNotify)wl_callback_destroy);

    _xfw_monitor_manager_wayland_destroy(screen->monitor_manager);

    if (screen->toplevel_manager != NULL) {
        zwlr_foreign_toplevel_manager_v1_destroy(screen->toplevel_manager);
    }
    g_list_free_full(screen->pending_seats, g_object_unref);
    if (screen->wl_registry != NULL) {
        wl_registry_destroy(screen->wl_registry);
    }
    g_list_free(screen->windows);
    g_list_free(screen->windows_stacked);
    g_hash_table_destroy(screen->wl_windows);
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
                for (GList *l = xfw_screen_get_seats(XFW_SCREEN(wscreen)); l != NULL; l = l->next) {
                    XfwSeat *seat = XFW_SEAT(l->data);
                    xfw_window_activate(wscreen->show_desktop_data.was_active, seat, 0, NULL);
                }
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
add_async_roundtrip(XfwScreenWayland *screen) {
    struct wl_callback *callback = wl_display_sync(screen->wl_display);
    wl_callback_add_listener(callback, &callback_listener, screen);
    screen->async_roundtrips = g_list_prepend(screen->async_roundtrips, callback);
}

static void
async_roundtrip_done(void *data, struct wl_callback *callback, uint32_t callback_id) {
    XfwScreenWayland *screen = data;
    screen->async_roundtrips = g_list_remove(screen->async_roundtrips, callback);
    wl_callback_destroy(callback);
}

static void
init_toplevel_manager(XfwScreenWayland *screen) {
    g_return_if_fail(!screen->defer_toplevel_manager);
    g_return_if_fail(screen->toplevel_manager_id != 0);
    g_return_if_fail(screen->toplevel_manager_version != 0);
    g_return_if_fail(screen->toplevel_manager == NULL);

    screen->toplevel_manager = wl_registry_bind(screen->wl_registry,
                                                screen->toplevel_manager_id,
                                                &zwlr_foreign_toplevel_manager_v1_interface,
                                                MIN(screen->toplevel_manager_version, 3));
    zwlr_foreign_toplevel_manager_v1_add_listener(screen->toplevel_manager, &toplevel_manager_listener, screen);
    add_async_roundtrip(screen);
}


static void
registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    XfwScreenWayland *wscreen = XFW_SCREEN_WAYLAND(data);

    if (strcmp(zwlr_foreign_toplevel_manager_v1_interface.name, interface) == 0) {
        wscreen->toplevel_manager_id = id;
        wscreen->toplevel_manager_version = version;
        if (!wscreen->defer_toplevel_manager) {
            init_toplevel_manager(wscreen);
        }
    } else if (strcmp(wl_seat_interface.name, interface) == 0) {
        struct wl_seat *wl_seat = wl_registry_bind(wscreen->wl_registry, id, &wl_seat_interface, 2);
        XfwSeatWayland *seat = _xfw_seat_wayland_new(XFW_SCREEN(wscreen), wl_seat);
        wscreen->pending_seats = g_list_prepend(wscreen->pending_seats, seat);
        add_async_roundtrip(wscreen);
    } else if (strcmp(ext_workspace_manager_v1_interface.name, interface) == 0) {
        XfwScreen *screen = XFW_SCREEN(wscreen);
        if (xfw_screen_get_workspace_manager(screen) != NULL) {
            g_message("Already have a workspace manager, but got a new ext_workspace_manager_v1 global");
        } else {
            struct ext_workspace_manager_v1 *wl_workspace_manager = wl_registry_bind(registry,
                                                                                     id,
                                                                                     &ext_workspace_manager_v1_interface,
                                                                                     MIN((uint32_t)ext_workspace_manager_v1_interface.version, version));
            _xfw_screen_set_workspace_manager(screen, _xfw_workspace_manager_wayland_new(wscreen, wl_workspace_manager));
            add_async_roundtrip(wscreen);
        }
    } else if (strcmp(wl_output_interface.name, interface) == 0) {
        struct wl_output *output = wl_registry_bind(registry, id, &wl_output_interface, MIN(version, 4));
        _xfw_monitor_manager_wayland_new_output(wscreen->monitor_manager, output);
        add_async_roundtrip(wscreen);
    } else if (strcmp(zxdg_output_manager_v1_interface.name, interface) == 0) {
        struct zxdg_output_manager_v1 *xdg_output_manager = wl_registry_bind(registry,
                                                                             id,
                                                                             &zxdg_output_manager_v1_interface,
                                                                             MIN(version, 3));
        _xfw_monitor_manager_wayland_new_xdg_output_manager(wscreen->monitor_manager, xdg_output_manager);
        add_async_roundtrip(wscreen);
    }
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t id) {
    XfwScreenWayland *screen = XFW_SCREEN_WAYLAND(data);

    gboolean seat_removed = FALSE;

    for (GList *l = xfw_screen_get_seats(XFW_SCREEN(screen)); l != NULL; l = l->next) {
        XfwSeatWayland *seat = XFW_SEAT_WAYLAND(l->data);
        struct wl_seat *wl_seat = _xfw_seat_wayland_get_wl_seat(seat);
        if (id == wl_proxy_get_id((struct wl_proxy *)wl_seat)) {
            _xfw_screen_seat_removed(XFW_SCREEN(screen), XFW_SEAT(seat));
            seat_removed = TRUE;
            break;
        }
    }

    if (!seat_removed) {
        for (GList *l = screen->pending_seats; l != NULL; l = l->next) {
            XfwSeatWayland *seat = XFW_SEAT_WAYLAND(l->data);
            struct wl_seat *wl_seat = _xfw_seat_wayland_get_wl_seat(seat);
            if (id == wl_proxy_get_id((struct wl_proxy *)wl_seat)) {
                screen->pending_seats = g_list_delete_link(screen->pending_seats, l);
                g_object_unref(seat);
                seat_removed = TRUE;
                break;
            }
        }
    }

    if (!seat_removed) {
        _xfw_monitor_manager_wayland_global_removed(screen->monitor_manager, id);
    }
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

void
_xfw_screen_wayland_seat_ready(XfwScreenWayland *screen, XfwSeatWayland *seat) {
    GList *link = g_list_find(screen->pending_seats, seat);
    if (link != NULL) {
        screen->pending_seats = g_list_delete_link(screen->pending_seats, link);
        _xfw_screen_seat_added(XFW_SCREEN(screen), XFW_SEAT(seat));
    }
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
