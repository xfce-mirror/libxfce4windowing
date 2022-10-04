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
#include "xfw-workspace-x11.h"

enum {
    PROP0,
    PROP_WNCK_WINDOW,
};

struct _XfwWindowX11Private {
    XfwScreen *screen;
    WnckWindow *wnck_window;
    XfwWindowType window_type;
    XfwWindowState state;
    XfwWindowCapabilities capabilities;
    GdkRectangle geometry;
    XfwWorkspace *workspace;
    GList *monitors;
};

static void xfw_window_x11_window_init(XfwWindowIface *iface);
static void xfw_window_x11_constructed(GObject *obj);
static void xfw_window_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_window_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_window_x11_finalize(GObject *obj);
static guint64 xfw_window_x11_get_id(XfwWindow *window);
static const gchar *xfw_window_x11_get_name(XfwWindow *window);
static GdkPixbuf *xfw_window_x11_get_icon(XfwWindow *window, gint size);
static XfwWindowType xfw_window_x11_get_window_type(XfwWindow *window);
static XfwWindowState xfw_window_x11_get_state(XfwWindow *window);
static XfwWindowCapabilities xfw_window_x11_get_capabilities(XfwWindow *window);
static GdkRectangle *xfw_window_x11_get_geometry(XfwWindow *window);
static XfwScreen *xfw_window_x11_get_screen(XfwWindow *window);
static XfwWorkspace *xfw_window_x11_get_workspace(XfwWindow *window);
static GList *xfw_window_x11_get_monitors(XfwWindow *window);
static gboolean xfw_window_x11_activate(XfwWindow *window, guint64 event_timestamp, GError **error);
static gboolean xfw_window_x11_close(XfwWindow *window, guint64 event_timestamp, GError **error);
static gboolean xfw_window_x11_start_move(XfwWindow *window, GError **error);
static gboolean xfw_window_x11_start_resize(XfwWindow *window, GError **error);
static gboolean xfw_window_x11_move_to_workspace(XfwWindow *window, XfwWorkspace *workspace, GError **error);
static gboolean xfw_window_x11_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error);
static gboolean xfw_window_x11_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error);
static gboolean xfw_window_x11_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error);
static gboolean xfw_window_x11_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error);
static gboolean xfw_window_x11_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
static gboolean xfw_window_x11_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error);
static gboolean xfw_window_x11_set_shaded(XfwWindow *window, gboolean is_shaded, GError **error);
static gboolean xfw_window_x11_set_above(XfwWindow *window, gboolean is_above, GError **error);
static gboolean xfw_window_x11_set_below(XfwWindow *window, gboolean is_below, GError **error);

static void name_changed(WnckWindow *wnck_window, XfwWindowX11 *window);
static void icon_changed(WnckWindow *wnck_window, XfwWindowX11 *window);
static void type_changed(WnckWindow *wnck_window, XfwWindowX11 *window);
static void state_changed(WnckWindow *wnck_window, WnckWindowState changed_mask, WnckWindowState new_state, XfwWindowX11 *window);
static void actions_changed(WnckWindow *wnck_window, WnckWindowActions wnck_changed_mask, WnckWindowActions wnck_new_actions, XfwWindowX11 *window);
static void geometry_changed(WnckWindow *wnck_window, XfwWindowX11 *window);
static void workspace_changed(WnckWindow *wnck_window, XfwWindowX11 *window);

static XfwWindowType convert_type(WnckWindowType wnck_type);
static XfwWindowState convert_state(WnckWindow *wnck_window, WnckWindowState wnck_state);
static XfwWindowCapabilities convert_capabilities(WnckWindow *wnck_window, WnckWindowActions wnck_actions);

G_DEFINE_TYPE_WITH_CODE(XfwWindowX11, xfw_window_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWindowX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WINDOW,
                                              xfw_window_x11_window_init))

static void
xfw_window_x11_class_init(XfwWindowX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_window_x11_constructed;
    gklass->set_property = xfw_window_x11_set_property;
    gklass->get_property = xfw_window_x11_get_property;
    gklass->finalize = xfw_window_x11_finalize;

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
    window->priv = xfw_window_x11_get_instance_private(window);
}

static void xfw_window_x11_constructed(GObject *obj) {
    XfwWindowX11 *window = XFW_WINDOW_X11(obj);

    window->priv->window_type = convert_type(wnck_window_get_window_type(window->priv->wnck_window));
    window->priv->state = convert_state(window->priv->wnck_window, wnck_window_get_state(window->priv->wnck_window));
    window->priv->capabilities = convert_capabilities(window->priv->wnck_window, wnck_window_get_actions(window->priv->wnck_window));
    window->priv->workspace = _xfw_screen_x11_workspace_for_wnck_workspace(XFW_SCREEN_X11(window->priv->screen),
                                                                           wnck_window_get_workspace(window->priv->wnck_window));

    g_signal_connect(window->priv->wnck_window, "name-changed", G_CALLBACK(name_changed), window);
    g_signal_connect(window->priv->wnck_window, "icon-changed", G_CALLBACK(icon_changed), window);
    g_signal_connect(window->priv->wnck_window, "type-changed", G_CALLBACK(type_changed), window);
    g_signal_connect(window->priv->wnck_window, "state-changed", G_CALLBACK(state_changed), window);
    g_signal_connect(window->priv->wnck_window, "actions-changed", G_CALLBACK(actions_changed), window);
    g_signal_connect(window->priv->wnck_window, "geometry-changed", G_CALLBACK(geometry_changed), window);
    g_signal_connect(window->priv->wnck_window, "workspace-changed", G_CALLBACK(workspace_changed), window);
}

static void
xfw_window_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWindowX11 *window = XFW_WINDOW_X11(obj);

    switch (prop_id) {
        case PROP_WNCK_WINDOW:
            window->priv->wnck_window = g_value_dup_object(value);
            break;

        case WINDOW_PROP_SCREEN:
            window->priv->screen = g_value_get_object(value);
            break;

        case WINDOW_PROP_ID:
        case WINDOW_PROP_NAME:
        case WINDOW_PROP_TYPE:
        case WINDOW_PROP_STATE:
        case WINDOW_PROP_CAPABILITIES:
        case WINDOW_PROP_WORKSPACE:
        case WINDOW_PROP_MONITORS:
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

        case WINDOW_PROP_TYPE:
            g_value_set_enum(value, xfw_window_x11_get_window_type(window));
            break;

        case WINDOW_PROP_STATE:
            g_value_set_flags(value, xfw_window_x11_get_state(window));
            break;

        case WINDOW_PROP_CAPABILITIES:
            g_value_set_flags(value, xfw_window_x11_get_capabilities(window));
            break;

        case WINDOW_PROP_WORKSPACE:
            g_value_set_object(value, xfw_window_x11_get_workspace(window));
            break;

        case WINDOW_PROP_MONITORS:
            g_value_set_pointer(value, xfw_window_x11_get_monitors(window));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_x11_finalize(GObject *obj) {
    XfwWindowX11 *window = XFW_WINDOW_X11(obj);

    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, name_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, icon_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, type_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, state_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, actions_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, geometry_changed, window);
    g_signal_handlers_disconnect_by_func(window->priv->wnck_window, workspace_changed, window);

    g_list_free(window->priv->monitors);

    // to be released last
    g_object_unref(window->priv->wnck_window);

    G_OBJECT_CLASS(xfw_window_x11_parent_class)->finalize(obj);
}

static void
xfw_window_x11_window_init(XfwWindowIface *iface) {
    iface->get_id = xfw_window_x11_get_id;
    iface->get_name = xfw_window_x11_get_name;
    iface->get_icon = xfw_window_x11_get_icon;
    iface->get_window_type = xfw_window_x11_get_window_type;
    iface->get_state = xfw_window_x11_get_state;
    iface->get_capabilities = xfw_window_x11_get_capabilities;
    iface->get_geometry = xfw_window_x11_get_geometry;
    iface->get_screen = xfw_window_x11_get_screen;
    iface->get_workspace = xfw_window_x11_get_workspace;
    iface->get_monitors = xfw_window_x11_get_monitors;
    iface->activate = xfw_window_x11_activate;
    iface->close = xfw_window_x11_close;
    iface->start_move = xfw_window_x11_start_move;
    iface->start_resize = xfw_window_x11_start_resize;
    iface->move_to_workspace = xfw_window_x11_move_to_workspace;
    iface->set_minimized = xfw_window_x11_set_minimized;
    iface->set_maximized = xfw_window_x11_set_maximized;
    iface->set_fullscreen = xfw_window_x11_set_fullscreen;
    iface->set_skip_pager = xfw_window_x11_set_skip_pager;
    iface->set_skip_tasklist = xfw_window_x11_set_skip_tasklist;
    iface->set_pinned = xfw_window_x11_set_pinned;
    iface->set_shaded = xfw_window_x11_set_shaded;
    iface->set_above = xfw_window_x11_set_above;
    iface->set_below = xfw_window_x11_set_below;
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
xfw_window_x11_get_icon(XfwWindow *window, gint size) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    if (size < WNCK_DEFAULT_ICON_SIZE) {
        return wnck_window_get_mini_icon(priv->wnck_window);
    }
    return wnck_window_get_icon(priv->wnck_window);
}

static XfwWindowType
xfw_window_x11_get_window_type(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->window_type;
}

static XfwWindowState
xfw_window_x11_get_state(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->state;
}

static XfwWindowCapabilities
xfw_window_x11_get_capabilities(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->capabilities;
}

static GdkRectangle *
xfw_window_x11_get_geometry(XfwWindow *window) {
    return &XFW_WINDOW_X11(window)->priv->geometry;
}

static XfwScreen *
xfw_window_x11_get_screen(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->screen;
}

static XfwWorkspace *
xfw_window_x11_get_workspace(XfwWindow *window) {
    return XFW_WINDOW_X11(window)->priv->workspace;
}

static GList *
xfw_window_x11_get_monitors(XfwWindow *window) {
    XfwWindowX11 *xwindow = XFW_WINDOW_X11(window);
    GdkMonitor *monitor = NULL;
    GdkWindow *gwindow = gtk_widget_get_window(GTK_WIDGET(window));

    if (gwindow != NULL) {
        GdkDisplay *display = gdk_display_get_default();

        monitor = gdk_display_get_monitor_at_window(display, gwindow);
        if (xwindow->priv->monitors == NULL || monitor != xwindow->priv->monitors->data) {
            xwindow->priv->monitors = g_list_remove(xwindow->priv->monitors, xwindow->priv->monitors->data);
            xwindow->priv->monitors = g_list_prepend(xwindow->priv->monitors, monitor);
        }
    }

    return xwindow->priv->monitors;
}

static gboolean
xfw_window_x11_activate(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    wnck_window_activate(priv->wnck_window, (guint32)event_timestamp);
    return TRUE;
}

static gboolean
xfw_window_x11_close(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;
    wnck_window_close(priv->wnck_window, (guint32)event_timestamp);
    return TRUE;
}

static gboolean
xfw_window_x11_start_move(XfwWindow *window, GError **error) {
    wnck_window_keyboard_move(XFW_WINDOW_X11(window)->priv->wnck_window);
    return TRUE;
}

static gboolean
xfw_window_x11_start_resize(XfwWindow *window, GError **error) {
    wnck_window_keyboard_size(XFW_WINDOW_X11(window)->priv->wnck_window);
    return TRUE;
}

static gboolean
xfw_window_x11_move_to_workspace(XfwWindow *window, XfwWorkspace *workspace, GError **error) {
    WnckWorkspace *wnck_workspace;

    g_return_val_if_fail(XFW_IS_WORKSPACE(workspace), FALSE);

    wnck_workspace = _xfw_workspace_x11_get_wnck_workspace(XFW_WORKSPACE_X11(workspace));
    wnck_window_move_to_workspace(XFW_WINDOW_X11(window)->priv->wnck_window, wnck_workspace);
    return TRUE;
}

static gboolean
xfw_window_x11_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;

    if (is_minimized) {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE) != 0) {
            wnck_window_minimize(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being minimized");
            }
            return FALSE;
        }
    } else {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE) != 0) {
            wnck_window_unminimize(priv->wnck_window, GDK_CURRENT_TIME);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being unminimized");
            }
            return FALSE;
        }
    }
}

static gboolean
xfw_window_x11_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;

    if (is_maximized) {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE) != 0) {
            wnck_window_maximize(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being maximized");
            }
            return FALSE;
        }
    } else {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE) != 0) {
            wnck_window_unmaximize(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being unmaximized");
            }
            return FALSE;
        }
    }
}

static gboolean
xfw_window_x11_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error) {
    XfwWindowX11 *xwindow = XFW_WINDOW_X11(window);

    if (is_fullscreen && (xwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN) == 0) {
        if (error != NULL) {
            *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being set fullscreen");
        }
        return FALSE;
    } else if (!is_fullscreen && (xwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN) == 0) {
        if (error != NULL) {
            *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being unset fullscreen");
        }
        return FALSE;
    } else {
        wnck_window_set_fullscreen(xwindow->priv->wnck_window, is_fullscreen);
        return TRUE;
    }
}

static gboolean
xfw_window_x11_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error) {
    wnck_window_set_skip_pager(XFW_WINDOW_X11(window)->priv->wnck_window, is_skip_pager);
    return TRUE;
}

static gboolean
xfw_window_x11_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error) {
    wnck_window_set_skip_tasklist(XFW_WINDOW_X11(window)->priv->wnck_window, is_skip_tasklist);
    return TRUE;
}

static gboolean
xfw_window_x11_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;

    if (is_pinned) {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_PIN) != 0) {
            wnck_window_pin(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being pinned");
            }
            return FALSE;
        }
    } else {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNPIN) != 0) {
            wnck_window_unpin(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being unpinned");
            }
            return FALSE;
        }
    }
}

static gboolean
xfw_window_x11_set_shaded(XfwWindow *window, gboolean is_shaded, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;

    if (is_shaded) {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_SHADE) != 0) {
            wnck_window_shade(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being shaded");
            }
            return FALSE;
        }
    } else {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNSHADE) != 0) {
            wnck_window_unshade(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being unshaded");
            }
            return FALSE;
        }
    }
}

static gboolean
xfw_window_x11_set_above(XfwWindow *window, gboolean is_above, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;

    if (is_above) {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_PLACE_ABOVE) != 0) {
            wnck_window_make_above(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being placed above others");
            }
            return FALSE;
        }
    } else {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_ABOVE) != 0) {
            wnck_window_unmake_above(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being placed back in the normal stacking order");
            }
            return FALSE;
        }
    }
}

static gboolean
xfw_window_x11_set_below(XfwWindow *window, gboolean is_below, GError **error) {
    XfwWindowX11Private *priv = XFW_WINDOW_X11(window)->priv;

    if (is_below) {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_PLACE_BELOW) != 0) {
            wnck_window_make_below(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being placed below others");
            }
            return FALSE;
        }
    } else {
        if ((priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_BELOW) != 0) {
            wnck_window_unmake_below(priv->wnck_window);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being placed back in the normal stacking order");
            }
            return FALSE;
        }
    }
}

static void
name_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    g_object_notify(G_OBJECT(window), "name");
    g_signal_emit_by_name(window, "name-changed");
}

static void
icon_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    g_signal_emit_by_name(window, "icon-changed");
}

static void
type_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    XfwWindowType old_type = window->priv->window_type;
    window->priv->window_type = convert_type(wnck_window_get_window_type(window->priv->wnck_window));
    g_object_notify(G_OBJECT(window), "type");
    g_signal_emit_by_name("window", "type-changed", old_type);
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
actions_changed(WnckWindow *wnck_window, WnckWindowActions wnck_changed_mask, WnckWindowActions wnck_new_actions, XfwWindowX11 *window) {
    XfwWindowCapabilities old_capabilities = window->priv->capabilities;
    XfwWindowCapabilities new_capabilities = convert_capabilities(wnck_window, wnck_new_actions);
    XfwWindowCapabilities changed_mask = old_capabilities ^ new_capabilities;
    window->priv->capabilities = new_capabilities;

    g_object_notify(G_OBJECT(window), "capabilities");
    g_signal_emit_by_name(window, "capabilities-changed", changed_mask, new_capabilities);
}

static void
geometry_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    wnck_window_get_geometry(wnck_window,
                             &window->priv->geometry.x, &window->priv->geometry.y,
                             &window->priv->geometry.width, &window->priv->geometry.height);
    g_signal_emit_by_name(window, "geometry-changed");
}

static void
workspace_changed(WnckWindow *wnck_window, XfwWindowX11 *window) {
    XfwWorkspace *new_workspace = _xfw_screen_x11_workspace_for_wnck_workspace(XFW_SCREEN_X11(window->priv->screen),
                                                                               wnck_window_get_workspace(wnck_window));
    XfwWorkspace *old_workspace = window->priv->workspace;

    if (old_workspace != new_workspace) {
        window->priv->workspace = new_workspace;
    }

    // workspace-changed is also fired when the windows pinned state changes
    if ((!wnck_window_is_pinned(wnck_window) && (window->priv->state & XFW_WINDOW_STATE_PINNED) != 0)
        || (wnck_window_is_pinned(wnck_window) && (window->priv->state & XFW_WINDOW_STATE_PINNED) == 0))
    {
        state_changed(wnck_window, 0, wnck_window_get_state(wnck_window), window);
    }

    if (old_workspace != new_workspace) {
        g_object_notify(G_OBJECT(window), "workspace");
        g_signal_emit_by_name(window, "workspace-changed");
    }
}

static XfwWindowType
convert_type(WnckWindowType wnck_type) {
    switch (wnck_type) {
        case WNCK_WINDOW_NORMAL:
            return XFW_WINDOW_TYPE_NORMAL;
        case WNCK_WINDOW_DESKTOP:
            return XFW_WINDOW_TYPE_DESKTOP;
        case WNCK_WINDOW_DOCK:
            return XFW_WINDOW_TYPE_DOCK;
        case WNCK_WINDOW_DIALOG:
            return XFW_WINDOW_TYPE_DIALOG;
        case WNCK_WINDOW_TOOLBAR:
            return XFW_WINDOW_TYPE_TOOLBAR;
        case WNCK_WINDOW_MENU:
            return XFW_WINDOW_TYPE_MENU;
        case WNCK_WINDOW_UTILITY:
            return XFW_WINDOW_TYPE_UTILITY;
        case WNCK_WINDOW_SPLASHSCREEN:
            return XFW_WINDOW_TYPE_SPLASHSCREEN;
    }
    return XFW_WINDOW_TYPE_NORMAL;
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
    { WNCK_WINDOW_STATE_ABOVE, XFW_WINDOW_STATE_ABOVE },
    { WNCK_WINDOW_STATE_BELOW, XFW_WINDOW_STATE_BELOW },
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
    if (wnck_window_is_pinned(wnck_window)) {
        state |= XFW_WINDOW_STATE_PINNED;
    }
    return state;
}

static const struct {
    // Action bits we need to find in actions in order to enable capabilities_bits
    WnckWindowActions wnck_actions_bits;
    // State bits from the window state that we...
    WnckWindowState wnck_state_bits;
    // ... need or do not need to find in order to enable capabilities_bit
    gboolean need_wnck_state_bits_present;
    XfwWindowCapabilities capabilities_bit;
} capabilities_converters[] = {
    { WNCK_WINDOW_ACTION_MINIMIZE, WNCK_WINDOW_STATE_MINIMIZED, FALSE, XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE },
    { WNCK_WINDOW_ACTION_MAXIMIZE, WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY | WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY, FALSE, XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE },
    { WNCK_WINDOW_ACTION_FULLSCREEN, WNCK_WINDOW_STATE_FULLSCREEN, FALSE, XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN },
    { WNCK_WINDOW_ACTION_STICK | WNCK_WINDOW_ACTION_CHANGE_WORKSPACE, WNCK_WINDOW_STATE_STICKY, FALSE,  XFW_WINDOW_CAPABILITIES_CAN_PIN },
    { WNCK_WINDOW_ACTION_UNMINIMIZE, WNCK_WINDOW_STATE_MINIMIZED, TRUE, XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE },
    { WNCK_WINDOW_ACTION_UNMAXIMIZE, WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY | WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY, TRUE, XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE },
    { WNCK_WINDOW_ACTION_FULLSCREEN, WNCK_WINDOW_STATE_FULLSCREEN, TRUE, XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN },
    { WNCK_WINDOW_ACTION_UNSTICK | WNCK_WINDOW_ACTION_CHANGE_WORKSPACE, WNCK_WINDOW_STATE_STICKY, TRUE, XFW_WINDOW_CAPABILITIES_CAN_UNPIN },
    { WNCK_WINDOW_ACTION_MOVE, 0, FALSE, XFW_WINDOW_CAPABILITIES_CAN_MOVE },
    { WNCK_WINDOW_ACTION_RESIZE, 0, FALSE, XFW_WINDOW_CAPABILITIES_CAN_RESIZE },
    { WNCK_WINDOW_ACTION_ABOVE, WNCK_WINDOW_STATE_ABOVE, FALSE, XFW_WINDOW_CAPABILITIES_CAN_PLACE_ABOVE },
    { WNCK_WINDOW_ACTION_ABOVE, WNCK_WINDOW_STATE_ABOVE, TRUE, XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_ABOVE },
    { WNCK_WINDOW_ACTION_BELOW, WNCK_WINDOW_STATE_BELOW, FALSE, XFW_WINDOW_CAPABILITIES_CAN_PLACE_BELOW },
    { WNCK_WINDOW_ACTION_BELOW, WNCK_WINDOW_STATE_BELOW, TRUE, XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_BELOW },
};
static XfwWindowCapabilities
convert_capabilities(WnckWindow *wnck_window, WnckWindowActions wnck_actions)
{
    WnckWindowState wnck_state = wnck_window_get_state(wnck_window);
    XfwWindowCapabilities capabilities = XFW_WINDOW_CAPABILITIES_NONE;
    for (size_t i = 0; i < sizeof(capabilities_converters) / sizeof(*capabilities_converters); ++i) {
        if ((wnck_actions & capabilities_converters[i].wnck_actions_bits) != 0) {
            if ((
                    capabilities_converters[i].need_wnck_state_bits_present
                    && (wnck_state & capabilities_converters[i].wnck_state_bits) != 0
                )
                ||
                (
                    !capabilities_converters[i].need_wnck_state_bits_present
                    && (wnck_state & capabilities_converters[i].wnck_state_bits) == 0
                )
            ) {
                capabilities |= capabilities_converters[i].capabilities_bit;
            }
        }
    }
    return capabilities;
}
