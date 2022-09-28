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

#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-screen-wayland.h"
#include "xfw-util.h"
#include "xfw-window-wayland.h"
#include "xfw-window.h"

enum {
    PROP0,
    PROP_HANDLE,
};

struct _XfwWindowWaylandPrivate {
    XfwScreen *screen;
    struct zwlr_foreign_toplevel_handle_v1 *handle;
    gboolean created_emitted;

    guint64 id;
    gchar *app_id;
    gchar *name;
    gchar *icon_name;
    GdkPixbuf *icon;
    XfwWindowState state;
    XfwWindowCapabilities capabilities;
    GdkRectangle geometry;  // unfortunately unsupported
    GList *monitors;
};

static void xfw_window_wayland_window_init(XfwWindowIface *iface);
static void xfw_window_wayland_constructed(GObject *obj);
static void xfw_window_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_window_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_window_wayland_finalize(GObject *obj);
static guint64 xfw_window_wayland_get_id(XfwWindow *window);
static const gchar *xfw_window_wayland_get_name(XfwWindow *window);
static GdkPixbuf *xfw_window_wayland_get_icon(XfwWindow *window);
static XfwWindowType xfw_window_wayland_get_window_type(XfwWindow *window);
static XfwWindowState xfw_window_wayland_get_state(XfwWindow *window);
static XfwWindowCapabilities xfw_window_wayland_get_capabilities(XfwWindow *window);
static GdkRectangle *xfw_window_wayland_get_geometry(XfwWindow *window);
static XfwScreen *xfw_window_wayland_get_screen(XfwWindow *window);
static XfwWorkspace *xfw_window_wayland_get_workspace(XfwWindow *window);
static GList *xfw_window_wayland_get_monitors(XfwWindow *window);
static gboolean xfw_window_wayland_activate(XfwWindow *window, guint64 event_timestamp, GError **error);
static gboolean xfw_window_wayland_close(XfwWindow *window, guint64 event_timestamp, GError **error);
static gboolean xfw_window_wayland_start_move(XfwWindow *window, GError **error);
static gboolean xfw_window_wayland_start_resize(XfwWindow *window, GError **error);
static gboolean xfw_window_wayland_move_to_workspace(XfwWindow *window, XfwWorkspace *workspace, GError **error);
static gboolean xfw_window_wayland_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error);
static gboolean xfw_window_wayland_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error);
static gboolean xfw_window_wayland_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error);
static gboolean xfw_window_wayland_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error);
static gboolean xfw_window_wayland_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
static gboolean xfw_window_wayland_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error);
static gboolean xfw_window_wayland_set_shaded(XfwWindow *window, gboolean is_pinned, GError **error);
static gboolean xfw_window_wayland_set_above(XfwWindow *window, gboolean is_above, GError **error);
static gboolean xfw_window_wayland_set_below(XfwWindow *window, gboolean is_below, GError **error);

static void toplevel_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *app_id);
static void toplevel_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *title);
static void toplevel_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_array *wl_state);
static void toplevel_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct zwlr_foreign_toplevel_handle_v1 *wl_parent);
static void toplevel_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output);
static void toplevel_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output);
static void toplevel_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);
static void toplevel_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);

const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
    .app_id = toplevel_app_id,
    .title = toplevel_title,
    .state = toplevel_state,
    .parent = toplevel_parent,
    .output_enter = toplevel_output_enter,
    .output_leave = toplevel_output_leave,
    .closed = toplevel_closed,
    .done = toplevel_done,
};

G_DEFINE_TYPE_WITH_CODE(XfwWindowWayland, xfw_window_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwWindowWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_WINDOW,
                                              xfw_window_wayland_window_init))

static void
xfw_window_wayland_class_init(XfwWindowWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_window_wayland_constructed;
    gklass->set_property = xfw_window_wayland_set_property;
    gklass->get_property = xfw_window_wayland_get_property;
    gklass->finalize = xfw_window_wayland_finalize;

    g_object_class_install_property(gklass,
                                    PROP_HANDLE,
                                    g_param_spec_pointer("handle",
                                                         "handle",
                                                         "handle",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_window_install_properties(gklass);
}

static void
xfw_window_wayland_init(XfwWindowWayland *window) {
    window->priv = xfw_window_wayland_get_instance_private(window);
}

static void
xfw_window_wayland_constructed(GObject *obj) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(obj);
    zwlr_foreign_toplevel_handle_v1_add_listener(window->priv->handle, &toplevel_handle_listener, window);
}

static void
xfw_window_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(obj);

    switch (prop_id) {
        case PROP_HANDLE:
            window->priv->handle = g_value_get_pointer(value);
            break;

        case WINDOW_PROP_SCREEN:
            window->priv->screen = g_value_get_object(value);
            break;

        case WINDOW_PROP_ID:
        case WINDOW_PROP_NAME:
        case WINDOW_PROP_ICON:
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
xfw_window_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWindow *window = XFW_WINDOW(obj);

    switch (prop_id) {
        case PROP_HANDLE:
            g_value_set_pointer(value, XFW_WINDOW_WAYLAND(window)->priv->handle);
            break;

        case WINDOW_PROP_SCREEN:
            g_value_set_object(value, xfw_window_wayland_get_screen(window));
            break;

        case WINDOW_PROP_ID:
            g_value_set_uint64(value, xfw_window_wayland_get_id(window));
            break;

        case WINDOW_PROP_NAME:
            g_value_set_string(value, xfw_window_wayland_get_name(window));
            break;

        case WINDOW_PROP_ICON:
            g_value_set_object(value, xfw_window_wayland_get_icon(window));
            break;

        case WINDOW_PROP_TYPE:
            g_value_set_enum(value, xfw_window_wayland_get_window_type(window));
            break;

        case WINDOW_PROP_STATE:
            g_value_set_flags(value, xfw_window_wayland_get_state(window));
            break;

        case WINDOW_PROP_CAPABILITIES:
            g_value_set_flags(value, xfw_window_wayland_get_capabilities(window));
            break;

        case WINDOW_PROP_WORKSPACE:
            g_value_set_object(value, xfw_window_wayland_get_workspace(window));
            break;

        case WINDOW_PROP_MONITORS:
            g_value_set_pointer(value, xfw_window_wayland_get_monitors(window));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_wayland_finalize(GObject *obj) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(obj);

    zwlr_foreign_toplevel_handle_v1_destroy(window->priv->handle);
    g_free(window->priv->app_id);
    g_free(window->priv->icon_name);
    if (window->priv->icon) {
        g_object_unref(window->priv->icon);
    }
    g_list_free(window->priv->monitors);

    G_OBJECT_CLASS(xfw_window_wayland_parent_class)->finalize(obj);
}

static void
xfw_window_wayland_window_init(XfwWindowIface *iface) {
    iface->get_id = xfw_window_wayland_get_id;
    iface->get_name = xfw_window_wayland_get_name;
    iface->get_icon = xfw_window_wayland_get_icon;
    iface->get_window_type = xfw_window_wayland_get_window_type;
    iface->get_state = xfw_window_wayland_get_state;
    iface->get_capabilities = xfw_window_wayland_get_capabilities;
    iface->get_geometry = xfw_window_wayland_get_geometry;
    iface->get_screen = xfw_window_wayland_get_screen;
    iface->get_workspace = xfw_window_wayland_get_workspace;
    iface->get_monitors = xfw_window_wayland_get_monitors;
    iface->activate = xfw_window_wayland_activate;
    iface->close = xfw_window_wayland_close;
    iface->start_move = xfw_window_wayland_start_move;
    iface->start_resize = xfw_window_wayland_start_resize;
    iface->move_to_workspace = xfw_window_wayland_move_to_workspace;
    iface->set_minimized = xfw_window_wayland_set_minimized;
    iface->set_maximized = xfw_window_wayland_set_maximized;
    iface->set_fullscreen = xfw_window_wayland_set_fullscreen;
    iface->set_skip_pager = xfw_window_wayland_set_skip_pager;
    iface->set_skip_tasklist = xfw_window_wayland_set_skip_tasklist;
    iface->set_pinned = xfw_window_wayland_set_pinned;
    iface->set_shaded = xfw_window_wayland_set_shaded;
    iface->set_above = xfw_window_wayland_set_above;
    iface->set_below = xfw_window_wayland_set_below;
}

static guint64
xfw_window_wayland_get_id(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->id;
}

static const gchar *
xfw_window_wayland_get_name(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->name;
}

static GdkPixbuf *
xfw_window_wayland_get_icon(XfwWindow *window) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);

    if (wwindow->priv->icon == NULL && wwindow->priv->icon_name != NULL) {
        GtkIconTheme *itheme = gtk_icon_theme_get_for_screen(_xfw_screen_wayland_get_gdk_screen(XFW_SCREEN_WAYLAND(wwindow->priv->screen)));
        GError *error = NULL;
        GdkPixbuf *icon = gtk_icon_theme_load_icon(itheme, wwindow->priv->icon_name, 64, 0, &error);
        if (icon != NULL) {
            wwindow->priv->icon = icon;
        } else if (error != NULL) {
            g_message("Failed to load icon for app '%s': %s", wwindow->priv->app_id, error->message);
            g_error_free(error);
        } else {
            g_message("Failed to load icon for app '%s'", wwindow->priv->app_id);
        }
    }

    if (wwindow->priv->icon != NULL) {
        g_object_ref(wwindow->priv->icon);
    }
    return wwindow->priv->icon;
}

static XfwWindowType
xfw_window_wayland_get_window_type(XfwWindow *window) {
    g_message("Window types are not supported on Wayland");
    return XFW_WINDOW_TYPE_NORMAL;
}

static XfwWindowState
xfw_window_wayland_get_state(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->state;
}

static XfwWindowCapabilities
xfw_window_wayland_get_capabilities(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->capabilities;
}

static GdkRectangle *
xfw_window_wayland_get_geometry(XfwWindow *window) {
    g_message("xfw_window_get_geometry() unsupported on Wayland");
    return &XFW_WINDOW_WAYLAND(window)->priv->geometry;
}

static XfwScreen *
xfw_window_wayland_get_screen(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->screen;
}

static XfwWorkspace *
xfw_window_wayland_get_workspace(XfwWindow *window) {
    return _xfw_screen_wayland_get_window_workspace(XFW_SCREEN_WAYLAND(XFW_WINDOW_WAYLAND(window)->priv->screen), window);
}

static GList *
xfw_window_wayland_get_monitors(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->monitors;
}

static gboolean
xfw_window_wayland_activate(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);
    struct wl_seat *wl_seat = _xfw_screen_wayland_get_wl_seat(XFW_SCREEN_WAYLAND(wwindow->priv->screen));

    if (wl_seat != NULL) {
        zwlr_foreign_toplevel_handle_v1_activate(wwindow->priv->handle, wl_seat);
        return TRUE;
    } else {
        if (error != NULL) {
            *error = g_error_new(XFW_ERROR, XFW_ERROR_INTERNAL, "Cannot activate window as we do not have a wl_seat");
        }
        return FALSE;
    }
}

static gboolean
xfw_window_wayland_close(XfwWindow *window, guint64 event_timestamp, GError **error) {
    zwlr_foreign_toplevel_handle_v1_close(XFW_WINDOW_WAYLAND(window)->priv->handle);
    return TRUE;
}

static gboolean
xfw_window_wayland_start_move(XfwWindow *window, GError **error) {
    if (error != NULL) {
        *error = g_error_new(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Moving windows is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_start_resize(XfwWindow *window, GError **error) {
    if (error != NULL) {
        *error = g_error_new(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Resizing windows is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_move_to_workspace(XfwWindow *window, XfwWorkspace *workspace, GError **error) {
    if (error != NULL) {
        *error = g_error_new(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Moving windows between workspaces is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);

    if (is_minimized) {
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE) != 0) {
            zwlr_foreign_toplevel_handle_v1_set_minimized(wwindow->priv->handle);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being minimized");
            }
            return FALSE;
        }
    } else {
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE) != 0) {
            zwlr_foreign_toplevel_handle_v1_unset_minimized(wwindow->priv->handle);
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
xfw_window_wayland_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);

    if (is_maximized) {
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE) != 0) {
            zwlr_foreign_toplevel_handle_v1_set_maximized(wwindow->priv->handle);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being maximized");
            }
            return FALSE;
        }
    } else {
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE) != 0) {
            zwlr_foreign_toplevel_handle_v1_unset_maximized(wwindow->priv->handle);
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
xfw_window_wayland_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);

    if (is_fullscreen) {
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN) != 0) {
            zwlr_foreign_toplevel_handle_v1_set_fullscreen(wwindow->priv->handle, NULL);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being set fullscreen");
            }
            return FALSE;
        }
    } else {
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN) != 0) {
            zwlr_foreign_toplevel_handle_v1_unset_fullscreen(wwindow->priv->handle);
            return TRUE;
        } else {
            if (error != NULL) {
                *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "This window does not currently support being unset fullscreen");
            }
            return FALSE;
        }
    }
}

static gboolean
xfw_window_wayland_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Pager skipping is not supported in Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Tasklist skipping is not supported in Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Window pinning is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_shaded(XfwWindow *window, gboolean is_pinned, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Window shading is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_above(XfwWindow *window, gboolean is_above, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Placing windows above others is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_below(XfwWindow *window, gboolean is_below, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Placing windows below others is not supported on Wayland");
    }
    return FALSE;
}

static void
toplevel_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *app_id) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    GDesktopAppInfo *app_info;
    gchar *desktop_id;

    g_free(window->priv->app_id);
    window->priv->app_id = g_strdup(app_id);

    desktop_id = g_strdup_printf("%s.desktop", app_id);
    app_info = g_desktop_app_info_new(desktop_id);
    g_free(desktop_id);

    if (app_info != NULL) {
        gchar *icon_name = g_desktop_app_info_get_string(app_info, "Icon");
        if (icon_name != NULL) {
            g_free(window->priv->icon_name);
            window->priv->icon_name = icon_name;

            if (window->priv->icon != NULL) {
                g_object_unref(window->priv->icon);
                window->priv->icon = NULL;
            }

            g_object_notify(G_OBJECT(window), "icon");
            g_signal_emit_by_name(window, "icon-changed");
        }
    }
}

static void
toplevel_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *title) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    g_free(window->priv->name);
    window->priv->name = g_strdup(title);
    g_object_notify(G_OBJECT(window), "name");
}

static const struct {
    enum zwlr_foreign_toplevel_handle_v1_state wl_state;
    XfwWindowState state_bit;
} state_converters[] = {
    { ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED, XFW_WINDOW_STATE_ACTIVE },
    { ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED, XFW_WINDOW_STATE_MINIMIZED },
    { ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED, XFW_WINDOW_STATE_MAXIMIZED },
    { ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN, XFW_WINDOW_STATE_FULLSCREEN },
};

static const struct {
    XfwWindowState state_bit;
    XfwWindowCapabilities capabilities_bit_if_present;
    XfwWindowCapabilities capabilities_bit_if_absent;
} capabilities_converters[] = {
    { XFW_WINDOW_STATE_MINIMIZED, XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE, XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE },
    { XFW_WINDOW_STATE_MAXIMIZED, XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE, XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE },
    { XFW_WINDOW_STATE_FULLSCREEN, XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN, XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN },
};

static void
toplevel_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_array *wl_state) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    XfwWindowState old_state = window->priv->state;
    XfwWindowState new_state = XFW_WINDOW_STATE_NONE;
    enum zwlr_foreign_toplevel_handle_v1_state *item;
    XfwWindowState changed_mask;
    XfwWindowCapabilities old_capabilities = window->priv->capabilities;
    XfwWindowCapabilities capapbilities_changed_mask;
    XfwWindowCapabilities new_capabilities = XFW_WINDOW_CAPABILITIES_NONE;

    wl_array_for_each(item, wl_state) {
        for (size_t i = 0; i < sizeof(state_converters) / sizeof(*state_converters); ++i) {
            if (state_converters[i].wl_state == *item) {
                new_state |= state_converters[i].state_bit;
                break;
            }
        }
    }
    changed_mask = old_state ^ new_state;
    window->priv->state = new_state;

    g_object_notify(G_OBJECT(window), "state");
    g_signal_emit_by_name(window, "state-changed", changed_mask, new_state);

    for (size_t i = 0; i < sizeof(capabilities_converters) / sizeof(*capabilities_converters); ++i) {
        if ((new_state & capabilities_converters[i].state_bit) != 0) {
            new_capabilities |= capabilities_converters[i].capabilities_bit_if_present;
        } else {
            new_capabilities |= capabilities_converters[i].capabilities_bit_if_absent;
        }
    }
    capapbilities_changed_mask = old_capabilities ^ new_capabilities;
    if (capapbilities_changed_mask != 0) {
        window->priv->capabilities = new_capabilities;
        g_object_notify(G_OBJECT(window), "capabilities");
        g_signal_emit_by_name(window, "capabilities-changed", capapbilities_changed_mask, new_capabilities);
    }

    if ((old_state & XFW_WINDOW_STATE_ACTIVE) == 0 && (new_state & XFW_WINDOW_STATE_ACTIVE) != 0) {
        _xfw_screen_wayland_set_active_window(XFW_SCREEN_WAYLAND(window->priv->screen), XFW_WINDOW(window));
    }
}

static void
toplevel_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct zwlr_foreign_toplevel_handle_v1 *wl_parent) {

}

static void
toplevel_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor;
    gint n_monitors = gdk_display_get_n_monitors(display);

    for (gint n = 0; n < n_monitors; n++) {
        monitor = gdk_display_get_monitor(display, n);
        if (output == gdk_wayland_monitor_get_wl_output(monitor)) {
            window->priv->monitors = g_list_prepend(window->priv->monitors, monitor);
            break;
        }
    }

    g_object_notify(G_OBJECT(window), "monitors");
}

static void
toplevel_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor;
    gint n_monitors = gdk_display_get_n_monitors(display);

    for (gint n = 0; n < n_monitors; n++) {
        monitor = gdk_display_get_monitor(display, n);
        if (output == gdk_wayland_monitor_get_wl_output(monitor)) {
            window->priv->monitors = g_list_remove(window->priv->monitors, monitor);
            break;
        }
    }

    g_object_notify(G_OBJECT(window), "monitors");
}

static void
toplevel_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    g_signal_emit_by_name(window, "closed");
}

static void
toplevel_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    if (!window->priv->created_emitted) {
        window->priv->created_emitted = TRUE;
        g_signal_emit_by_name(window->priv->screen, "window-opened", window);
    }
}

struct zwlr_foreign_toplevel_handle_v1 *
_xfw_window_wayland_get_handle(XfwWindowWayland *window) {
    return window->priv->handle;
}
