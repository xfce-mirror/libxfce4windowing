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

#include "libxfce4windowing-private.h"
#include "xfw-screen-private.h"
#include "xfw-screen-x11.h"
#include "xfw-util.h"
#include "xfw-window-x11.h"
#include "xfw-workspace-manager-x11.h"

#include <gdk/gdkx.h>
#include <libwnck/libwnck.h>

struct _XfwScreenX11Private {
    GdkScreen *gdk_screen;
    WnckScreen *wnck_screen;
    XfwWorkspaceManager *workspace_manager;
    GList *windows;
    GList *windows_stacked;
    GHashTable *wnck_windows;
    XfwWindow *active_window;
    guint show_desktop : 1;
};

static void xfw_screen_x11_screen_init(XfwScreenIface *iface);
static void xfw_screen_x11_constructed(GObject *obj);
static void xfw_screen_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_screen_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_screen_x11_finalize(GObject *obj);
static XfwWorkspaceManager *xfw_screen_x11_get_workspace_manager(XfwScreen *screen);
static GList *xfw_screen_x11_get_windows(XfwScreen *screen);
static GList *xfw_screen_x11_get_windows_stacked(XfwScreen *screen);
static XfwWindow *xfw_screen_x11_get_active_window(XfwScreen *screen);
static gboolean xfw_screen_x11_get_show_desktop(XfwScreen *screen);

static void xfw_screen_x11_set_show_desktop(XfwScreen *screen, gboolean show);

static void window_opened(WnckScreen *wnck_screen, WnckWindow *window, XfwScreenX11 *screen);
static void window_closed(WnckScreen *wnck_screen, WnckWindow *window, XfwScreenX11 *screen);
static void active_window_changed(WnckScreen *wnck_screen, WnckWindow *previous_window, XfwScreenX11 *screen);
static void window_stacking_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen);
static void showing_desktop_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen);
static void window_manager_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen);

G_DEFINE_TYPE_WITH_CODE(XfwScreenX11, xfw_screen_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwScreenX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_SCREEN,
                                              xfw_screen_x11_screen_init))

static void
xfw_screen_x11_class_init(XfwScreenX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_screen_x11_constructed;
    gklass->set_property = xfw_screen_x11_set_property;
    gklass->get_property = xfw_screen_x11_get_property;
    gklass->finalize = xfw_screen_x11_finalize;

    _xfw_screen_install_properties(gklass);
}

static void
xfw_screen_x11_init(XfwScreenX11 *screen) {
    screen->priv = xfw_screen_x11_get_instance_private(screen);
}

static void
xfw_screen_x11_constructed(GObject *obj) {
    XfwScreenX11 *screen = XFW_SCREEN_X11(obj);

    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    screen->priv->wnck_screen = g_object_ref(wnck_screen_get(gdk_x11_screen_get_screen_number(screen->priv->gdk_screen)));
    G_GNUC_END_IGNORE_DEPRECATIONS
    screen->priv->workspace_manager = _xfw_workspace_manager_x11_new(screen->priv->gdk_screen);
    screen->priv->wnck_windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);

    for (GList *l = wnck_screen_get_windows(screen->priv->wnck_screen); l != NULL; l = l->next) {
        XfwWindowX11 *window = g_object_new(XFW_TYPE_WINDOW_X11,
                                            "screen", screen,
                                            "wnck-window", l->data,
                                            NULL);
        screen->priv->windows = g_list_prepend(screen->priv->windows, window);
        g_hash_table_insert(screen->priv->wnck_windows, l->data, window);
    }
    screen->priv->windows = g_list_reverse(screen->priv->windows);
    window_stacking_changed(screen->priv->wnck_screen, screen);

    screen->priv->active_window = g_hash_table_lookup(screen->priv->wnck_windows, wnck_screen_get_active_window(screen->priv->wnck_screen));

    g_signal_connect(screen->priv->wnck_screen, "window-opened", G_CALLBACK(window_opened), screen);
    g_signal_connect(screen->priv->wnck_screen, "window-closed", G_CALLBACK(window_closed), screen);
    g_signal_connect(screen->priv->wnck_screen, "active-window-changed", G_CALLBACK(active_window_changed), screen);
    g_signal_connect(screen->priv->wnck_screen, "window-stacking-changed", G_CALLBACK(window_stacking_changed), screen);
    g_signal_connect(screen->priv->wnck_screen, "window-manager-changed", G_CALLBACK(window_manager_changed), screen);
    g_signal_connect(screen->priv->wnck_screen, "showing-desktop-changed", G_CALLBACK(showing_desktop_changed), screen);
}

static void
xfw_screen_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwScreenX11 *screen = XFW_SCREEN_X11(obj);

    switch (prop_id) {
        case SCREEN_PROP_SCREEN:
            screen->priv->gdk_screen = g_value_get_object(value);
            break;

        case SCREEN_PROP_SHOW_DESKTOP:
            xfw_screen_x11_set_show_desktop(XFW_SCREEN(screen), g_value_get_boolean(value));
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
        case SCREEN_PROP_SCREEN:
            g_value_set_object(value, screen->priv->gdk_screen);
            break;

        case SCREEN_PROP_WORKSPACE_MANAGER:
            g_value_set_object(value, screen->priv->workspace_manager);
            break;

        case SCREEN_PROP_ACTIVE_WINDOW:
            g_value_set_object(value, xfw_screen_x11_get_active_window(XFW_SCREEN(screen)));
            break;

        case SCREEN_PROP_SHOW_DESKTOP:
            g_value_set_boolean(value, xfw_screen_x11_get_show_desktop(XFW_SCREEN(screen)));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_x11_finalize(GObject *obj) {
    XfwScreenX11 *screen = XFW_SCREEN_X11(obj);

    g_object_unref(screen->priv->workspace_manager);

    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, window_opened, screen);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, window_closed, screen);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, active_window_changed, screen);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, window_stacking_changed, screen);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, window_manager_changed, screen);
    g_signal_handlers_disconnect_by_func(screen->priv->wnck_screen, showing_desktop_changed, screen);
    g_list_free(screen->priv->windows);
    g_list_free(screen->priv->windows_stacked);
    g_hash_table_destroy(screen->priv->wnck_windows);

    // to be released last
    g_object_unref(screen->priv->wnck_screen);

    G_OBJECT_CLASS(xfw_screen_x11_parent_class)->finalize(obj);
}

static void
xfw_screen_x11_screen_init(XfwScreenIface *iface) {
    iface->get_workspace_manager = xfw_screen_x11_get_workspace_manager;
    iface->get_windows = xfw_screen_x11_get_windows;
    iface->get_windows_stacked = xfw_screen_x11_get_windows_stacked;
    iface->get_active_window = xfw_screen_x11_get_active_window;
    iface->get_show_desktop = xfw_screen_x11_get_show_desktop;

    iface->set_show_desktop = xfw_screen_x11_set_show_desktop;
}

static XfwWorkspaceManager *
xfw_screen_x11_get_workspace_manager(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->workspace_manager;
}

static GList *
xfw_screen_x11_get_windows(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->windows;
}

static GList *
xfw_screen_x11_get_windows_stacked(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->windows_stacked;
}

static XfwWindow *
xfw_screen_x11_get_active_window(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->active_window;
}

static gboolean
xfw_screen_x11_get_show_desktop(XfwScreen *screen) {
    return XFW_SCREEN_X11(screen)->priv->show_desktop;
}

static void
xfw_screen_x11_set_show_desktop(XfwScreen *screen, gboolean show) {
    XfwScreenX11 *xscreen = XFW_SCREEN_X11(screen);
    if (!!show != xscreen->priv->show_desktop) {
        xscreen->priv->show_desktop = !!show;
        wnck_screen_toggle_showing_desktop(xscreen->priv->wnck_screen, show);
        g_object_notify(G_OBJECT(screen), "show-desktop");
    }
}

static void
window_opened(WnckScreen *wnck_screen, WnckWindow *wnck_window, XfwScreenX11 *screen) {
    XfwWindowX11 *window = XFW_WINDOW_X11(g_object_new(XFW_TYPE_WINDOW_X11,
                                                       "screen", screen,
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
            screen->priv->active_window = NULL;
            g_signal_emit_by_name(wnck_window, "state-changed", 0, wnck_window_get_state(wnck_window));
            g_signal_emit_by_name(screen, "active-window-changed", window);
        }

        g_signal_emit_by_name(window, "closed");
        g_signal_emit_by_name(screen, "window-closed", window);
        g_signal_emit_by_name(screen, "window-stacking-changed", screen);

        g_object_unref(window);
    }
}

static void
active_window_changed(WnckScreen *wnck_screen, WnckWindow *previous_wnck_window, XfwScreenX11 *screen) {
    WnckWindow *wnck_window = wnck_screen_get_active_window(screen->priv->wnck_screen);
    XfwWindow *window = g_hash_table_lookup(screen->priv->wnck_windows, wnck_window);
    if (window != screen->priv->active_window) {
        screen->priv->active_window = window;
        if (previous_wnck_window != NULL) {
            g_signal_emit_by_name(previous_wnck_window, "state-changed", 0, wnck_window_get_state(previous_wnck_window));
        }
        if (wnck_window != NULL) {
            g_signal_emit_by_name(wnck_window, "state-changed", 0, wnck_window_get_state(wnck_window));
        }
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
    g_signal_emit_by_name(screen, "window-stacking-changed");
}

static void
window_manager_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen) {
    g_signal_emit_by_name(screen, "window-manager-changed");
}

static void
showing_desktop_changed(WnckScreen *wnck_screen, XfwScreenX11 *screen) {
    gboolean show_desktop = wnck_screen_get_showing_desktop(wnck_screen);
    if (show_desktop != screen->priv->show_desktop) {
        screen->priv->show_desktop = show_desktop;
        g_object_notify(G_OBJECT(screen), "show-desktop");
    }
}

XfwWorkspace *
_xfw_screen_x11_workspace_for_wnck_workspace(XfwScreenX11 *screen, WnckWorkspace *wnck_workspace) {
    return _xfw_workspace_manager_x11_workspace_for_wnck_workspace(XFW_WORKSPACE_MANAGER_X11(screen->priv->workspace_manager), wnck_workspace);
}
