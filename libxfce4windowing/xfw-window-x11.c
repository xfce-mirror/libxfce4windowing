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
#include "xfw-screen-x11.h"
#include "xfw-screen.h"
#include "xfw-util.h"
#include "xfw-window-x11.h"
#include "xfw-window.h"

enum {
    PROP0,
    PROP_WNCK_WINDOW,
};

struct _XfwWindowX11Private {
    XfwScreen *screen;
    WnckWindow *wnck_window;
    XfwWindowState state;
};

static void xfw_window_x11_window_init(XfwWindowIface *iface);
static void xfw_window_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_window_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_window_x11_dispose(GObject *obj);
static guint64 xfw_window_x11_get_id(XfwWindow *window);
static const gchar *xfw_window_x11_get_name(XfwWindow *window);
static GdkPixbuf *xfw_window_x11_get_icon(XfwWindow *window);
static XfwWindowState xfw_window_x11_get_state(XfwWindow *window);
static XfwScreen *xfw_window_x11_get_screen(XfwWindow *window);
static XfwWorkspace *xfw_window_x11_get_workspace(XfwWindow *window);
static void xfw_window_x11_activate(XfwWindow *window, guint64 event_timestamp, GError **error);
static void xfw_window_x11_close(XfwWindow *window, guint64 event_timestamp, GError **error);
static void xfw_window_x11_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error);
static void xfw_window_x11_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error);
static void xfw_window_x11_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error);
static void xfw_window_x11_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error);
static void xfw_window_x11_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
static void xfw_window_x11_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error);

static void name_changed(WnckWindow *wnck_window, XfwWindowX11 *window);
static void icon_changed(WnckWindow *wnck_window, XfwWindowX11 *window);
static void state_changed(WnckWindow *wnck_window, WnckWindowState changed_mask, WnckWindowState new_state, XfwWindowX11 *window);
static void workspace_changed(WnckWindow *wnck_window, XfwWindowX11 *window);

static XfwWindowState convert_state(WnckWindow *wnck_window, WnckWindowState wnck_state);

G_DEFINE_TYPE_WITH_CODE(XfwWindowX11, xfw_window_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWindowX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WINDOW,
                                              xfw_window_x11_window_init))

static void
xfw_window_x11_class_init(XfwWindowX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->set_property = xfw_window_x11_set_property;
    gklass->get_property = xfw_window_x11_get_property;
    gklass->dispose = xfw_window_x11_dispose;

    g_object_class_install_property(gklass,
                                    PROP_WNCK_WINDOW,
                                    g_param_spec_object("wnck-window",
                                                        "wnck-window",
                                                        "wnck-window",
                                                        WNCK_TYPE_WINDOW,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_window_install_properties(gklass);
}

static void
xfw_window_x11_init(XfwWindowX11 *window) {
    window->priv->state = convert_state(window->priv->wnck_window, wnck_window_get_state(window->priv->wnck_window));

    g_signal_connect(window->priv->wnck_window, "name-changed", (GCallback)name_changed, window);
    g_signal_connect(window->priv->wnck_window, "icon-changed", (GCallback)icon_changed, window);
    g_signal_connect(window->priv->wnck_window, "state-changed", (GCallback)state_changed, window);
    g_signal_connect(window->priv->wnck_window, "workspace-changed", (GCallback)workspace_changed, window);
}

static void
xfw_window_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWindowX11 *window = XFW_WINDOW_X11(obj);

    switch (prop_id) {
        case PROP_WNCK_WINDOW:
            window->priv->wnck_window = g_value_get_object(value);
            break;

        case WINDOW_PROP_SCREEN:
            window->priv->screen = g_value_get_object(value);
            break;

        case WINDOW_PROP_ID:
        case WINDOW_PROP_NAME:
        case WINDOW_PROP_ICON:
        case WINDOW_PROP_STATE:
        case WINDOW_PROP_WORKSPACE:
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWindow *window = XFW_WINDOW(obj);

    switch (prop_id) {
        case PROP_WNCK_WINDOW:
            g_value_set_object(value, XFW_WINDOW_X11(window)->priv->wnck_window);
            break;

        case WINDOW_PROP_SCREEN:
            g_value_set_object(value, xfw_window_x11_get_screen(window));
            break;

        case WINDOW_PROP_ID:
            g_value_set_uint64(value, xfw_window_x11_get_id(window));
            break;

        case WINDOW_PROP_NAME:
            g_value_set_string(value, xfw_window_x11_get_name(window));
            break;

        case WINDOW_PROP_ICON:
            g_value_set_object(value, xfw_window_x11_get_icon(window));
            break;

        case WINDOW_PROP_STATE:
            g_value_set_flags(value, xfw_window_x11_get_state(window));
            break;

        case WINDOW_PROP_WORKSPACE:
            g_value_set_object(value, xfw_window_x11_get_workspace(window));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_x11_dispose(GObject *obj) {
    XfwWindowX11 *window = XFW_WINDOW_X11(obj);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, name_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, icon_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, state_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, workspace_changed, window);
}

static void
xfw_window_x11_window_init(XfwWindowIface *iface) {
    iface->get_id = xfw_window_x11_get_id;
    iface->get_name = xfw_window_x11_get_name;
    iface->get_icon = xfw_window_x11_get_icon;
    iface->get_state = xfw_window_x11_get_state;
    iface->get_screen = xfw_window_x11_get_screen;
    iface->get_workspace = xfw_window_x11_get_workspace;
    iface->activate = xfw_window_x11_activate;
    iface->close = xfw_window_x11_close;
    iface->set_minimized = xfw_window_x11_set_minimized;
    iface->set_maximized = xfw_window_x11_set_maximized;
    iface->set_fullscreen = xfw_window_x11_set_fullscreen;
    iface->set_skip_pager = xfw_window_x11_set_skip_pager;
    iface->set_skip_tasklist = xfw_window_x11_set_skip_tasklist;
    iface->set_pinned = xfw_window_x11_set_pinned;
}

static guint64
xfw_window_x11_get_id(XfwWindow *window) {
    return wnck_window_get_xid(XFW_WINDOW_X11(window)->priv->wnck_window);
}

static const gchar *
xfw_window_x11_get_name(XfwWindow *window) {
    return wnck_window_get_name(XFW_WINDOW_X11(window)->priv->wnck_window);
}

static GdkPixbuf *
xfw_window_x11_get_icon(XfwWindow *window) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    GdkPixbuf *icon = wnck_window_get_icon(priv->wnck_window);
    GdkPixbuf *mini_icon = wnck_window_get_mini_icon(priv->wnck_window);
    if (icon == NULL) {
        return mini_icon;
    } else if (mini_icon == NULL) {
        return icon;
    } else if (gdk_pixbuf_get_width(icon) >= gdk_pixbuf_get_height(mini_icon)) {
        return icon;
    } else {
        return mini_icon;
    }
}

static XfwWindowState
xfw_window_x11_get_state(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->state;
}

static XfwScreen *
xfw_window_x11_get_screen(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->screen;
}

static XfwWorkspace *
xfw_window_x11_get_workspace(XfwWindow *window) {
    XfwWindowX11 *xwindow = XFW_WINDOW_X11(window);
    WnckWorkspace *workspace = wnck_window_get_workspace(xwindow->priv->wnck_window);
    if (workspace != NULL) {
        return _xfw_screen_x11_workspace_for_wnck_workspace(XFW_SCREEN_X11(xwindow->priv->screen), workspace);
    } else {
        return NULL;
    }
}

static void
xfw_window_x11_activate(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    wnck_window_activate(priv->wnck_window, (guint32)event_timestamp);
}

static void
xfw_window_x11_close(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    wnck_window_close(priv->wnck_window, (guint32)event_timestamp);
}

static void
xfw_window_x11_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    if (is_minimized) {
        wnck_window_minimize(priv->wnck_window);
    } else {
        wnck_window_unminimize(priv->wnck_window, GDK_CURRENT_TIME);
    }
}

static void
xfw_window_x11_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    if (is_maximized) {
        wnck_window_maximize(priv->wnck_window);
    } else {
        wnck_window_unmaximize(priv->wnck_window);
    }
}

static void
xfw_window_x11_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error) {
    wnck_window_set_fullscreen(XFW_WINDOW_X11(window)->priv->wnck_window, is_fullscreen);
}

static void
xfw_window_x11_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error) {
    wnck_window_set_skip_pager(XFW_WINDOW_X11(window)->priv->wnck_window, is_skip_pager);
}

static void
xfw_window_x11_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error) {
    wnck_window_set_skip_tasklist(XFW_WINDOW_X11(window)->priv->wnck_window, is_skip_tasklist);
}

static void
xfw_window_x11_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    if (is_pinned) {
        wnck_window_pin(priv->wnck_window);
    } else {
        wnck_window_unpin(priv->wnck_window);
    }
}

static void
name_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    g_object_notify(G_OBJECT(window), "name");
    g_signal_emit_by_name(window, "name-changed");
}

static void
icon_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    g_object_notify(G_OBJECT(window), "icon");
    g_signal_emit_by_name(window, "icon-changed");
}

static void
state_changed(WnckWindow *wnck_window, WnckWindowState wnck_changed_mask, WnckWindowState wnck_new_state, XfwWindowX11 *window) {
    XfwWindowState old_state = window->priv->state;
    XfwWindowState new_state = convert_state(wnck_window, wnck_new_state);
    XfwWindowState changed_mask = old_state ^ new_state;
    window->priv->state = new_state;

    g_object_notify(G_OBJECT(window), "state");
    g_signal_emit_by_name(window, "state-changed", changed_mask, new_state);
}

static void
workspace_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    g_object_notify(G_OBJECT(window), "workspace");
    g_signal_emit_by_name(window, "workspace-changed");
}

static const struct {
    WnckWindowState wnck_state_bits;
    XfwWindowState state_bit;
} state_converters[] = {
    { WNCK_WINDOW_STATE_MINIMIZED, XFW_WINDOW_STATE_MINIMIZED },
    { WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY | WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY, XFW_WINDOW_STATE_MAXIMIZED },
    { WNCK_WINDOW_STATE_FULLSCREEN, XFW_WINDOW_STATE_FULLSCREEN },
    { WNCK_WINDOW_STATE_SKIP_PAGER, XFW_WINDOW_STATE_SKIP_PAGER },
    { WNCK_WINDOW_STATE_SKIP_TASKLIST, XFW_WINDOW_STATE_SKIP_TASKLIST },
    { WNCK_WINDOW_STATE_STICKY, XFW_WINDOW_STATE_PINNED },
};

static XfwWindowState
convert_state(WnckWindow *wnck_window, WnckWindowState wnck_state) {
    XfwWindowState state = XFW_WINDOW_STATE_NONE;
    for (size_t i = 0; i < sizeof(state_converters) / sizeof(*state_converters); ++i) {
        if ((wnck_state & state_converters[i].wnck_state_bits) != 0) {
            state |= state_converters[i].state_bit;
        }
    }
    if (wnck_window_is_active(wnck_window)) {
        state |= XFW_WINDOW_STATE_ACTIVE;
    }
    return state;
}
