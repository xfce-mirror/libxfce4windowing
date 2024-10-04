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

#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

#include "libxfce4windowing-private.h"
#include "xfw-application-wayland.h"
#include "xfw-monitor-wayland.h"
#include "xfw-screen-wayland.h"
#include "xfw-screen.h"
#include "xfw-seat-wayland.h"
#include "xfw-util.h"
#include "xfw-window-private.h"
#include "xfw-window-wayland.h"

enum {
    PROP0,
    PROP_HANDLE,
};

struct _XfwWindowWaylandPrivate {
    struct zwlr_foreign_toplevel_handle_v1 *handle;
    gboolean created_emitted;

    const gchar **class_ids;
    gchar *app_id;
    gchar *name;
    XfwWindowState state;
    XfwWindowCapabilities capabilities;
    GdkRectangle geometry;  // unfortunately unsupported
    GList *monitors;
    XfwApplication *app;
};

static void xfw_window_wayland_constructed(GObject *obj);
static void xfw_window_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_window_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_window_wayland_finalize(GObject *obj);

static const gchar *const *xfw_window_wayland_get_class_ids(XfwWindow *window);
static const gchar *xfw_window_wayland_get_name(XfwWindow *window);
static GIcon *xfw_window_wayland_get_gicon(XfwWindow *window);
static XfwWindowType xfw_window_wayland_get_window_type(XfwWindow *window);
static XfwWindowState xfw_window_wayland_get_state(XfwWindow *window);
static XfwWindowCapabilities xfw_window_wayland_get_capabilities(XfwWindow *window);
static GdkRectangle *xfw_window_wayland_get_geometry(XfwWindow *window);
static XfwWorkspace *xfw_window_wayland_get_workspace(XfwWindow *window);
static GList *xfw_window_wayland_get_monitors(XfwWindow *window);
static XfwApplication *xfw_window_wayland_get_application(XfwWindow *window);
static gboolean xfw_window_wayland_activate(XfwWindow *window, XfwSeat *seat, guint64 event_timestamp, GError **error);
static gboolean xfw_window_wayland_close(XfwWindow *window, guint64 event_timestamp, GError **error);
static gboolean xfw_window_wayland_start_move(XfwWindow *window, GError **error);
static gboolean xfw_window_wayland_start_resize(XfwWindow *window, GError **error);
static gboolean xfw_window_wayland_set_geometry(XfwWindow *window, const GdkRectangle *rect, GError **error);
static gboolean xfw_window_wayland_set_button_geometry(XfwWindow *window, GdkWindow *relative_to, const GdkRectangle *rect, GError **error);
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
static gboolean xfw_window_wayland_is_on_workspace(XfwWindow *window, XfwWorkspace *workspace);
static gboolean xfw_window_wayland_is_in_viewport(XfwWindow *window, XfwWorkspace *workspace);

static void toplevel_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *app_id);
static void toplevel_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *title);
static void toplevel_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_array *wl_state);
static void toplevel_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct zwlr_foreign_toplevel_handle_v1 *wl_parent);
static void toplevel_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output);
static void toplevel_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output);
static void toplevel_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);
static void toplevel_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);

static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
    .app_id = toplevel_app_id,
    .title = toplevel_title,
    .state = toplevel_state,
    .parent = toplevel_parent,
    .output_enter = toplevel_output_enter,
    .output_leave = toplevel_output_leave,
    .closed = toplevel_closed,
    .done = toplevel_done,
};

G_DEFINE_TYPE_WITH_PRIVATE(XfwWindowWayland, xfw_window_wayland, XFW_TYPE_WINDOW)

static void
xfw_window_wayland_class_init(XfwWindowWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);
    XfwWindowClass *window_class = XFW_WINDOW_CLASS(klass);

    gklass->constructed = xfw_window_wayland_constructed;
    gklass->set_property = xfw_window_wayland_set_property;
    gklass->get_property = xfw_window_wayland_get_property;
    gklass->finalize = xfw_window_wayland_finalize;

    window_class->get_class_ids = xfw_window_wayland_get_class_ids;
    window_class->get_name = xfw_window_wayland_get_name;
    window_class->get_gicon = xfw_window_wayland_get_gicon;
    window_class->get_window_type = xfw_window_wayland_get_window_type;
    window_class->get_state = xfw_window_wayland_get_state;
    window_class->get_capabilities = xfw_window_wayland_get_capabilities;
    window_class->get_geometry = xfw_window_wayland_get_geometry;
    window_class->get_workspace = xfw_window_wayland_get_workspace;
    window_class->get_monitors = xfw_window_wayland_get_monitors;
    window_class->get_application = xfw_window_wayland_get_application;
    window_class->activate = xfw_window_wayland_activate;
    window_class->close = xfw_window_wayland_close;
    window_class->start_move = xfw_window_wayland_start_move;
    window_class->start_resize = xfw_window_wayland_start_resize;
    window_class->set_geometry = xfw_window_wayland_set_geometry;
    window_class->set_button_geometry = xfw_window_wayland_set_button_geometry;
    window_class->move_to_workspace = xfw_window_wayland_move_to_workspace;
    window_class->set_minimized = xfw_window_wayland_set_minimized;
    window_class->set_maximized = xfw_window_wayland_set_maximized;
    window_class->set_fullscreen = xfw_window_wayland_set_fullscreen;
    window_class->set_skip_pager = xfw_window_wayland_set_skip_pager;
    window_class->set_skip_tasklist = xfw_window_wayland_set_skip_tasklist;
    window_class->set_pinned = xfw_window_wayland_set_pinned;
    window_class->set_shaded = xfw_window_wayland_set_shaded;
    window_class->set_above = xfw_window_wayland_set_above;
    window_class->set_below = xfw_window_wayland_set_below;
    window_class->is_on_workspace = xfw_window_wayland_is_on_workspace;
    window_class->is_in_viewport = xfw_window_wayland_is_in_viewport;

    g_object_class_install_property(gklass,
                                    PROP_HANDLE,
                                    g_param_spec_pointer("handle",
                                                         "handle",
                                                         "handle",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
xfw_window_wayland_init(XfwWindowWayland *window) {
    window->priv = xfw_window_wayland_get_instance_private(window);
    window->priv->class_ids = g_new0(const gchar *, 2);
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

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_wayland_finalize(GObject *obj) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(obj);

    zwlr_foreign_toplevel_handle_v1_destroy(window->priv->handle);
    g_free(window->priv->class_ids);
    g_free(window->priv->app_id);
    g_free(window->priv->name);
    g_list_free(window->priv->monitors);
    g_object_unref(window->priv->app);

    G_OBJECT_CLASS(xfw_window_wayland_parent_class)->finalize(obj);
}

static const gchar *const *
xfw_window_wayland_get_class_ids(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->class_ids;
}

static const gchar *
xfw_window_wayland_get_name(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->name;
}

static GIcon *
xfw_window_wayland_get_gicon(XfwWindow *window) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);
    GIcon *gicon = _xfw_application_wayland_get_gicon_no_fallback(XFW_APPLICATION_WAYLAND(wwindow->priv->app));

    if (gicon != NULL) {
        return gicon;
    } else {
        return g_themed_icon_new_with_default_fallbacks(XFW_WINDOW_FALLBACK_ICON_NAME);
    }
}

static XfwWindowType
xfw_window_wayland_get_window_type(XfwWindow *window) {
    _xfw_g_message_once("Window types are not supported on Wayland");
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
    _xfw_g_message_once("xfw_window_get_geometry() unsupported on Wayland");
    return &XFW_WINDOW_WAYLAND(window)->priv->geometry;
}

static XfwWorkspace *
xfw_window_wayland_get_workspace(XfwWindow *window) {
    XfwScreen *screen = _xfw_window_get_screen(window);
    return _xfw_screen_wayland_get_window_workspace(XFW_SCREEN_WAYLAND(screen), window);
}

static GList *
xfw_window_wayland_get_monitors(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->monitors;
}

static XfwApplication *
xfw_window_wayland_get_application(XfwWindow *window) {
    return XFW_WINDOW_WAYLAND(window)->priv->app;
}

static gboolean
xfw_window_wayland_activate(XfwWindow *window, XfwSeat *seat, guint64 event_timestamp, GError **error) {
    XfwWindowWayland *wwindow = XFW_WINDOW_WAYLAND(window);

    GList *seats = NULL;
    if (seat != NULL) {
        seats = g_list_prepend(seats, seat);
    } else {
        XfwScreen *screen = _xfw_window_get_screen(window);
        seats = g_list_copy(xfw_screen_get_seats(screen));
    }

    if (seats == NULL) {
        if (error != NULL) {
            *error = g_error_new(XFW_ERROR, XFW_ERROR_INTERNAL, "Cannot activate window as we do not have a wl_seat");
        }
        return FALSE;
    } else {
        for (GList *l = seats; l != NULL; l = l->next) {
            XfwSeatWayland *a_seat = XFW_SEAT_WAYLAND(l->data);
            struct wl_seat *wl_seat = _xfw_seat_wayland_get_wl_seat(a_seat);
            zwlr_foreign_toplevel_handle_v1_activate(wwindow->priv->handle, wl_seat);
        }
        g_list_free(seats);
        return TRUE;
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
xfw_window_wayland_set_geometry(XfwWindow *window, const GdkRectangle *rect, GError **error) {
    if (error != NULL) {
        *error = g_error_new(XFW_ERROR, XFW_ERROR_UNSUPPORTED, "Setting windows geometry is not supported on Wayland");
    }
    return FALSE;
}

static gboolean
xfw_window_wayland_set_button_geometry(XfwWindow *window, GdkWindow *relative_to, const GdkRectangle *rect, GError **error) {
    zwlr_foreign_toplevel_handle_v1_set_rectangle(XFW_WINDOW_WAYLAND(window)->priv->handle, gdk_wayland_window_get_wl_surface(relative_to), rect->x, rect->y, rect->width, rect->height);
    return TRUE;
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
        if ((wwindow->priv->capabilities & XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE) != 0) {
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

static gboolean
xfw_window_wayland_is_on_workspace(XfwWindow *window, XfwWorkspace *workspace) {
    return xfw_window_wayland_get_workspace(window) == workspace;
}

static gboolean
xfw_window_wayland_is_in_viewport(XfwWindow *window, XfwWorkspace *workspace) {
    return FALSE;
}

static void
toplevel_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *app_id) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);

    if (app_id == NULL || *app_id == '\0' || g_strcmp0(app_id, window->priv->app_id) == 0) {
        return;
    }

    _xfw_window_invalidate_icon(XFW_WINDOW(window));

    g_free(window->priv->app_id);
    window->priv->app_id = g_strdup(app_id);
    window->priv->class_ids[0] = window->priv->app_id;

    if (window->priv->app != NULL) {
        g_object_unref(window->priv->app);
    }
    window->priv->app = XFW_APPLICATION(_xfw_application_wayland_get(window, window->priv->app_id));
    g_object_notify(G_OBJECT(window), "application");

    g_signal_emit_by_name(window, "icon-changed");
    g_object_notify(G_OBJECT(window), "class-ids");
    g_signal_emit_by_name(window, "class-changed");
}

static void
toplevel_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, const char *title) {
    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    g_free(window->priv->name);
    window->priv->name = g_strdup(title);
    g_object_notify(G_OBJECT(window), "name");
    g_signal_emit_by_name(window, "name-changed");
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
    XfwWindowCapabilities capabilities_changed_mask;
    XfwWindowCapabilities new_capabilities = XFW_WINDOW_CAPABILITIES_NONE;

    wl_array_for_each(item, wl_state) {
        for (size_t i = 0; i < G_N_ELEMENTS(state_converters); ++i) {
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

    for (size_t i = 0; i < G_N_ELEMENTS(capabilities_converters); ++i) {
        if ((new_state & capabilities_converters[i].state_bit) != 0) {
            new_capabilities |= capabilities_converters[i].capabilities_bit_if_present;
        } else {
            new_capabilities |= capabilities_converters[i].capabilities_bit_if_absent;
        }
    }
    capabilities_changed_mask = old_capabilities ^ new_capabilities;
    if (capabilities_changed_mask != 0) {
        window->priv->capabilities = new_capabilities;
        g_object_notify(G_OBJECT(window), "capabilities");
        g_signal_emit_by_name(window, "capabilities-changed", capabilities_changed_mask, new_capabilities);
    }

    if (window->priv->created_emitted && (old_state & XFW_WINDOW_STATE_ACTIVE) != (new_state & XFW_WINDOW_STATE_ACTIVE)) {
        XfwScreen *screen = _xfw_window_get_screen(XFW_WINDOW(window));

        if (new_state & XFW_WINDOW_STATE_ACTIVE) {
            _xfw_screen_set_active_window(screen, XFW_WINDOW(window));
        } else if (xfw_screen_get_active_window(screen) == XFW_WINDOW(window)) {
            _xfw_screen_set_active_window(screen, NULL);
        }
    }
}

static void
toplevel_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct zwlr_foreign_toplevel_handle_v1 *wl_parent) {
}

static void
toplevel_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output) {
    g_debug("toplevel %u output_enter", wl_proxy_get_id((struct wl_proxy *)wl_toplevel));

    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    XfwScreen *screen = _xfw_window_get_screen(XFW_WINDOW(window));
    GList *monitors = xfw_screen_get_monitors(screen);

    for (GList *l = monitors; l != NULL; l = l->next) {
        XfwMonitorWayland *monitor = XFW_MONITOR_WAYLAND(l->data);
        if (output == _xfw_monitor_wayland_get_wl_output(monitor)
            && g_list_find(window->priv->monitors, monitor) == NULL)
        {
            window->priv->monitors = g_list_prepend(window->priv->monitors, monitor);
            g_object_notify(G_OBJECT(window), "monitors");
            break;
        }
    }
}

static void
toplevel_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel, struct wl_output *output) {
    g_debug("toplevel %u output_leave", wl_proxy_get_id((struct wl_proxy *)wl_toplevel));

    XfwWindowWayland *window = XFW_WINDOW_WAYLAND(data);
    XfwScreen *screen = _xfw_window_get_screen(XFW_WINDOW(window));
    GList *monitors = xfw_screen_get_monitors(screen);

    for (GList *l = monitors; l != NULL; l = l->next) {
        XfwMonitorWayland *monitor = XFW_MONITOR_WAYLAND(l->data);
        if (output == _xfw_monitor_wayland_get_wl_output(monitor)) {
            GList *lm = g_list_find(window->priv->monitors, monitor);
            if (lm != NULL) {
                window->priv->monitors = g_list_delete_link(window->priv->monitors, lm);
                g_object_notify(G_OBJECT(window), "monitors");
            }
            break;
        }
    }
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
        XfwScreen *screen = _xfw_window_get_screen(XFW_WINDOW(window));

        window->priv->created_emitted = TRUE;

        // nothing in the protocol ensures that the app id is set and we need an app
        if (window->priv->app == NULL) {
            toplevel_app_id(window, window->priv->handle, "UnknownAppID");
        }

        g_signal_emit_by_name(screen, "window-opened", window);
        if (window->priv->state & XFW_WINDOW_STATE_ACTIVE) {
            _xfw_screen_set_active_window(screen, XFW_WINDOW(window));
        }
    }
}

struct zwlr_foreign_toplevel_handle_v1 *
_xfw_window_wayland_get_handle(XfwWindowWayland *window) {
    return window->priv->handle;
}
