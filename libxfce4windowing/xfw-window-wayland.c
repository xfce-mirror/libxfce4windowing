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
    gchar *name;
    XfwWindowState state;
    GdkRectangle geometry;  // unfortunately unsupported
};

static void xfw_window_wayland_window_init(XfwWindowIface *iface);
static void xfw_window_wayland_constructed(GObject *obj);
static void xfw_window_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_window_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_window_wayland_dispose(GObject *obj);
static guint64 xfw_window_wayland_get_id(XfwWindow *window);
static const gchar *xfw_window_wayland_get_name(XfwWindow *window);
static GdkPixbuf *xfw_window_wayland_get_icon(XfwWindow *window);
static XfwWindowState xfw_window_wayland_get_state(XfwWindow *window);
static GdkRectangle *xfw_window_wayland_get_geometry(XfwWindow *window);
static XfwScreen *xfw_window_wayland_get_screen(XfwWindow *window);
static XfwWorkspace *xfw_window_wayland_get_workspace(XfwWindow *window);
static void xfw_window_wayland_activate(XfwWindow *window, guint64 event_timestamp, GError **error);
static void xfw_window_wayland_close(XfwWindow *window, guint64 event_timestamp, GError **error);
static void xfw_window_wayland_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error);
static void xfw_window_wayland_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error);
static void xfw_window_wayland_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error);
static void xfw_window_wayland_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error);
static void xfw_window_wayland_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
static void xfw_window_wayland_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error);

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
    gklass->dispose = xfw_window_wayland_dispose;

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
        case WINDOW_PROP_STATE:
        case WINDOW_PROP_WORKSPACE:
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

        case WINDOW_PROP_STATE:
            g_value_set_flags(value, xfw_window_wayland_get_state(window));
            break;

        case WINDOW_PROP_WORKSPACE:
            g_value_set_object(value, xfw_window_wayland_get_workspace(window));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_wayland_dispose(GObject *obj) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(obj);
    zwlr_foreign_toplevel_handle_v1_destroy(window->priv->handle);
}

static void
xfw_window_wayland_window_init(XfwWindowIface *iface) {
    iface->get_id = xfw_window_wayland_get_id;
    iface->get_name = xfw_window_wayland_get_name;
    iface->get_icon = xfw_window_wayland_get_icon;
    iface->get_state = xfw_window_wayland_get_state;
    iface->get_geometry = xfw_window_wayland_get_geometry;
    iface->get_screen = xfw_window_wayland_get_screen;
    iface->get_workspace = xfw_window_wayland_get_workspace;
    iface->activate = xfw_window_wayland_activate;
    iface->close = xfw_window_wayland_close;
    iface->set_minimized = xfw_window_wayland_set_minimized;
    iface->set_maximized = xfw_window_wayland_set_maximized;
    iface->set_fullscreen = xfw_window_wayland_set_fullscreen;
    iface->set_skip_pager = xfw_window_wayland_set_skip_pager;
    iface->set_skip_tasklist = xfw_window_wayland_set_skip_tasklist;
    iface->set_pinned = xfw_window_wayland_set_pinned;
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
    return NULL;
}

static XfwWindowState
xfw_window_wayland_get_state(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->state;
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
    g_message("Window<->Workspace association is not available on Wayland");
    return NULL;
}

static void
xfw_window_wayland_activate(XfwWindow *window, guint64 event_timestamp, GError **error) {
    // FIXME: make sure NULL for seat means "compositor picks what seat", and doesn't crash or fail
    zwlr_foreign_toplevel_handle_v1_activate(XFW_WINDOW_WAYLAND(window)->priv->handle, NULL);
}

static void
xfw_window_wayland_close(XfwWindow *window, guint64 event_timestamp, GError **error) {
    zwlr_foreign_toplevel_handle_v1_close(XFW_WINDOW_WAYLAND(window)->priv->handle);
}

static void
xfw_window_wayland_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);
    if (is_minimized) {
        zwlr_foreign_toplevel_handle_v1_set_minimized(wwindow->priv->handle);
    } else {
        zwlr_foreign_toplevel_handle_v1_unset_minimized(wwindow->priv->handle);
    }
}

static void
xfw_window_wayland_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);
    if (is_maximized) {
        zwlr_foreign_toplevel_handle_v1_set_maximized(wwindow->priv->handle);
    } else {
        zwlr_foreign_toplevel_handle_v1_unset_maximized(wwindow->priv->handle);
    }
}

static void
xfw_window_wayland_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);
    if (is_fullscreen) {
        zwlr_foreign_toplevel_handle_v1_set_fullscreen(wwindow->priv->handle, NULL);
    } else {
        zwlr_foreign_toplevel_handle_v1_unset_fullscreen(wwindow->priv->handle);
    }
}

static void
xfw_window_wayland_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Pager skipping is not supported in Wayland");
    }
}

static void
xfw_window_wayland_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Tasklist skipping is not supported in Wayland");
    }
}

static void
xfw_window_wayland_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error) {
    if (error != NULL) {
        *error = g_error_new_literal(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Window pinning is not supported on Wayland");
    }
}

static void
toplevel_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *app_id) {

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

static void
toplevel_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_array *wl_state) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    XfwWindowState old_state = window->priv->state;
    XfwWindowState new_state = XFW_WINDOW_STATE_NONE;
    enum zwlr_foreign_toplevel_handle_v1_state *item;
    XfwWindowState changed_mask;

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
    if ((old_state & XFW_WINDOW_STATE_ACTIVE) == 0 && (new_state & XFW_WINDOW_STATE_ACTIVE) != 0) {
        _xfw_screen_wayland_set_active_window(XFW_SCREEN_WAYLAND(window->priv->screen), XFW_WINDOW(window));
    }
}

static void
toplevel_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct zwlr_foreign_toplevel_handle_v1 *wl_parent) {

}

static void
toplevel_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output) {

}

static void
toplevel_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output) {

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
