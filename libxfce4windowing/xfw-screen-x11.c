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

#include <libwnck/libwnck.h>

#include "libxfce4windowing-private.h"
#include "xfw-util.h"
#include "xfw-screen-x11.h"
#include "xfw-screen.h"
#include "xfw-window-x11.h"
#include "xfw-workspace-manager-x11.h"

enum {
    PROP0,
    PROP_WNCK_SCREEN,
};

struct _XfwScreenX11Private {
    GdkScreen *gdk_screen;
    WnckScreen *wnck_screen;
    XfwWorkspaceManager *workspace_manager;
    GList *windows;
    GList *windows_stacked;
    GHashTable *wnck_windows;
    XfwWindow *active_window;
};

static void xfw_screen_x11_screen_init(XfwScreenIface *iface);
static void xfw_screen_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_screen_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_screen_x11_dispose(GObject *obj);
static XfwWorkspaceManager *xfw_screen_x11_get_workspace_manager(XfwScreen *screen);
static GList *xfw_screen_x11_get_windows(XfwScreen *screen);
static GList *xfw_screen_x11_get_windows_stacked(XfwScreen *screen);
static XfwWindow *xfw_screen_x11_get_active_window(XfwScreen *screen);

static void window_opened(WnckScreen *wnck_screen, WnckWindow *window, XfwScreenX11 *screen);
static void window_closed(WnckScreen *wnck_screen, WnckWindow *window, XfwScreenX11 *screen);
static void active_window_changed(WnckScreen *wnck_screen, WnckWindow *previous_window, XfwScreenX11 *screen);
static void window_stacking_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen);

G_DEFINE_TYPE_WITH_CODE(XfwScreenX11, xfw_screen_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwScreenX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_SCREEN,
                                              xfw_screen_x11_screen_init))

static void
xfw_screen_x11_class_init(XfwScreenX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->set_property = xfw_screen_x11_set_property;
    gklass->get_property = xfw_screen_x11_get_property;
    gklass->dispose = xfw_screen_x11_dispose;

    g_object_class_install_property(gklass,
                                    PROP_WNCK_SCREEN,
                                    g_param_spec_object("wnck-screen",
                                                        "wnck-screen",
                                                        "wnck-screen",
                                                        WNCK_TYPE_SCREEN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_screen_install_properties(gklass);
}

static void
xfw_screen_x11_init(XfwScreenX11 *screen) {
    screen->priv->workspace_manager = _xfw_workspace_manager_x11_new(screen->priv->gdk_screen);
    screen->priv->wnck_windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);

    for (GList *l = wnck_screen_get_windows(screen->priv->wnck_screen); l != NULL; l = l->next) {
        XfwWindowX11 *window = g_object_new(XFW_TYPE_WINDOW_X11,
                                            "wnck-screen", l->data,
                                            NULL);
        screen->priv->windows = g_list_prepend(screen->priv->windows, window);
        g_hash_table_insert(screen->priv->wnck_windows, l->data, window);
    }
    screen->priv->windows = g_list_reverse(screen->priv->windows);
    window_stacking_changed(screen->priv->wnck_screen, screen);

    screen->priv->active_window = g_hash_table_lookup(screen->priv->wnck_windows, wnck_screen_get_active_window(screen->priv->wnck_screen));

    g_signal_connect(screen->priv->wnck_screen, "window-opened", (GCallback)window_opened, screen);
    g_signal_connect(screen->priv->wnck_screen, "window-closed", (GCallback)window_closed, screen);
    g_signal_connect(screen->priv->wnck_screen, "active-window-changed", (GCallback)active_window_changed, screen);
    g_signal_connect(screen->priv->wnck_screen, "window-stacking-changed", (GCallback)window_stacking_changed, screen);
}

static void
xfw_screen_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwScreenX11 *screen = XFW_SCREEN_X11(obj);

    switch (prop_id) {
        case PROP_WNCK_SCREEN:
            screen->priv->wnck_screen = g_value_get_object(value);
            break;

        case SCREEN_PROP_SCREEN:
            screen->priv->gdk_screen = g_value_get_object(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwScreenX11 *screen = XFW_SCREEN_X11(obj);

    switch (prop_id) {
        case PROP_WNCK_SCREEN:
            g_value_set_object(value, screen->priv->wnck_screen);
            break;

        case SCREEN_PROP_SCREEN:
            g_value_set_object(value, screen->priv->gdk_screen);
            break;

        case SCREEN_PROP_WORKSPACE_MANAGER:
            g_value_set_object(value, screen->priv->workspace_manager);
            break;

        case SCREEN_PROP_ACTIVE_WINDOW:
            g_value_set_object(value, xfw_screen_x11_get_active_window(XFW_SCREEN(screen)));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_x11_dispose(GObject *obj) {
    XfwScreenX11 *screen = XFW_SCREEN_X11(obj);
    g_object_unref(screen->priv->workspace_manager);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, window_opened, screen);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, window_closed, screen);
    g_list_free(screen->priv->windows);
    g_list_free(screen->priv->windows_stacked);
    g_hash_table_destroy(screen->priv->wnck_windows);
}

static void
xfw_screen_x11_screen_init(XfwScreenIface *iface) {
    iface->get_workspace_manager = xfw_screen_x11_get_workspace_manager;
    iface->get_windows = xfw_screen_x11_get_windows;
    iface->get_windows_stacked = xfw_screen_x11_get_windows_stacked;
    iface->get_active_window = xfw_screen_x11_get_active_window;
}

static XfwWorkspaceManager *
xfw_screen_x11_get_workspace_manager(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->workspace_manager;
}

static GList *xfw_screen_x11_get_windows(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->windows;
}

static GList *xfw_screen_x11_get_windows_stacked(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->windows_stacked;
}

static XfwWindow *xfw_screen_x11_get_active_window(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->active_window;
} 

static void
window_opened(WnckScreen *wnck_screen, WnckWindow *wnck_window, XfwScreenX11 *screen) {
    XfwWindowX11 *window = XFW_WINDOW_X11(g_object_new(XFW_TYPE_WINDOW_X11,
                                                       "wnck-window", wnck_window,
                                                       NULL));
    screen->priv->windows = g_list_prepend(screen->priv->windows, window);
    g_hash_table_insert(screen->priv->wnck_windows, wnck_window, window);
    // FIXME: window-stacking-changed signal will fire out of order
    window_stacking_changed(screen->priv->wnck_screen, screen);
    g_signal_emit_by_name(screen, "window-opened", window);
}

static void
window_closed(WnckScreen *wnck_screen, WnckWindow *wnck_window, XfwScreenX11 *screen) {
    XfwWindowX11 *window = g_hash_table_lookup(screen->priv->wnck_windows, wnck_window);
    if (window != NULL) {
        g_object_ref(window);

        g_hash_table_remove(screen->priv->wnck_windows, wnck_window);
        screen->priv->windows = g_list_remove(screen->priv->windows, window);
        screen->priv->windows_stacked = g_list_remove(screen->priv->windows_stacked, window);

        if (screen->priv->active_window == XFW_WINDOW(window)) {
            // TODO: maybe hold an extra reference for this so when a new window
            // is activated, the old one will be still around
            screen->priv->active_window = NULL;
        }

        g_signal_emit_by_name(window, "closed");
        g_signal_emit_by_name(screen, "window-closed", window);
        g_signal_emit_by_name(screen, "window-stacking-changed", screen);

        g_object_unref(window);
    }
}

static void
active_window_changed(WnckScreen *wnck_screen, WnckWindow *previous_wnck_window, XfwScreenX11 *screen) {
    XfwWindow *window = g_hash_table_lookup(screen->priv->wnck_windows, wnck_screen_get_active_window(screen->priv->wnck_screen));
    if (window != screen->priv->active_window) {
        screen->priv->active_window = window;
        g_signal_emit_by_name(screen, "active-window-changed", g_hash_table_lookup(screen->priv->wnck_windows, previous_wnck_window));
    }    
}

static void
window_stacking_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen) {
    g_list_free(screen->priv->windows_stacked);
    screen->priv->windows_stacked = NULL;

    for (GList *l = wnck_screen_get_windows_stacked(screen->priv->wnck_screen); l != NULL; l = l->next) {
        XfwWindowX11 *window = g_hash_table_lookup(screen->priv->wnck_windows, l->data);
        if (window != NULL) {
            screen->priv->windows_stacked = g_list_prepend(screen->priv->windows_stacked, window);
        }
    }
    screen->priv->windows_stacked = g_list_reverse(screen->priv->windows_stacked);
    g_signal_emit_by_name(screen, "window-stacking-changed", screen);
}

XfwWorkspace *
_xfw_screen_x11_workspace_for_wnck_workspace(XfwScreenX11 *screen, WnckWorkspace *wnck_workspace) {
    return _xfw_workspace_manager_x11_workspace_for_wnck_workspace(XFW_WORKSPACE_MANAGER_X11(screen->priv->workspace_manager), wnck_workspace);
}
