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

/**
 * SECTION:xfw-screen
 * @title: XfwScreen
 * @short_description: An object representing a logical screen
 * @stability: Unstable
 * @include: libxfce4windowing/libxfce4windowing.h
 *
 * #XfwScreen represents a logical screen.  On most windowing environments,
 * this doesn't necessarily correspond to a single monitor, but might span
 * multiple monitors.  These days, most windowing environments will only
 * have a single screen, even if (API-wise) more than one can be represented.
 *
 * The #XfwScreen instance is the main entry point into this library.  You
 * can obtain an instance using #xfw_screen_get_default().  From there, you can
 * enumerate toplevel windows, or examine workspace groups and workspaces.
 *
 * Note that #XfwScreen is an abstract class; when obtaining an instance,
 * an instance of a windowing-environment-specific object that implements
 * this interface will be returned.
 **/

#include "config.h"

#include <limits.h>

#include "libxfce4windowing-private.h"
#include "xfw-screen-private.h"
#include "xfw-util.h"
#include "xfw-window.h"

#ifdef ENABLE_X11
#include "xfw-screen-x11.h"
#endif

#ifdef ENABLE_WAYLAND
#include "xfw-screen-wayland.h"
#endif

#define XFW_SCREEN_GET_PRIVATE(screen) ((XfwScreenPrivate *)xfw_screen_get_instance_private(XFW_SCREEN(screen)))
#define GDK_SCREEN_XFW_SCREEN_KEY "libxfce4windowing-xfw-screen"

typedef struct _XfwScreenPrivate {
    GdkScreen *gscreen;
    GList *monitors;
} XfwScreenPrivate;

enum {
    PROP0,
    PROP_SCREEN,
    PROP_MONITORS,
    PROP_WORKSPACE_MANAGER,
    PROP_ACTIVE_WINDOW,
    PROP_SHOW_DESKTOP,
};

static void xfw_screen_constructed(GObject *object);
static void xfw_screen_set_property(GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void xfw_screen_get_property(GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void xfw_screen_dispose(GObject *object);
static void xfw_screen_finalize(GObject *object);

static void xfw_screen_gdk_screen_monitors_changed(GdkScreen *gscreen,
                                                   XfwScreen *screen);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(XfwScreen, xfw_screen, G_TYPE_OBJECT)


static void
xfw_screen_class_init(XfwScreenClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->constructed = xfw_screen_constructed;
    gobject_class->set_property = xfw_screen_set_property;
    gobject_class->get_property = xfw_screen_get_property;
    gobject_class->dispose = xfw_screen_dispose;
    gobject_class->finalize = xfw_screen_finalize;

    /**
     * XfwScreen::window-opened:
     * @screen: the object which received the signal.
     * @window: the new window that was opened.
     *
     * Emitted when a new window is opened on the screen.
     **/
    g_signal_new("window-opened",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenClass, window_opened),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);

    /**
     * XfwScreen::active-window-changed:
     * @screen: the object which received the signal.
     * @window: the previously-active window.
     *
     * Emitted when a new window becomes the active window.  Often the
     * active window will receive keyboard focus.  While @window is
     * the previously-active window (if any, and may be %NULL), the
     * newly-active window can be retrieved via
     * #xfw_screen_get_active_window().
     **/
    g_signal_new("active-window-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenClass, active_window_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);

    /**
     * XfwScreen::window-stacking-changed:
     * @screen: the object which received the signal.
     *
     * Emitted when the order of the windows as displayed on the screen has
     * changed.  Windows, in stacking order, can be retrieved via
     * #xfw_screen_get_windows_stacked().
     *
     * Note that currently this signal is not emitted on Wayland.
     **/
    g_signal_new("window-stacking-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenClass, window_stacking_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    /**
     * XfwScreen::window-closed:
     * @screen: the object that received the signal.
     * @window: the window that has been closed.
     *
     * Emitted when a window is closed on the screen.
     **/
    g_signal_new("window-closed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenClass, window_closed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);

    /**
     * XfwScreen::window-manager-changed:
     * @screen: the object which received the signal.
     *
     * Emitted when the window manager on @screen has changed.
     *
     * Note that currently this signal is not emitted on Wayland.
     **/
    g_signal_new("window-manager-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenClass, window_manager_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    /**
     * XfwScreen::monitors-changed:
     * @screen: the object which received the signal.
     *
     * Emitted when the number, size, or position of the monitors attached
     * to @screen changes.
     **/
    g_signal_new("monitors-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenClass, monitors_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    /**
     * XfwScreen:screen:
     *
     * The #GdkScreen instance used to construct this #XfwScreen.
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_SCREEN,
                                    g_param_spec_object("screen",
                                                        "screen",
                                                        "screen",
                                                        GDK_TYPE_SCREEN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /**
     * XfwScreen:monitors:
     *
     * The #XfwMonitor instances connected to this #XfwScreen.
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_MONITORS,
                                    g_param_spec_pointer("monitors",
                                                         "monitors",
                                                         "monitors",
                                                         G_PARAM_READABLE));

    /**
     * XfwScreen:workspace-manager:
     *
     * The #XfwWorkspaceManager that manages and describes workspace groups
     * and workspaces on this screen instance.
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_WORKSPACE_MANAGER,
                                    g_param_spec_object("workspace-manager",
                                                        "workspace-manager",
                                                        "workspace-manager",
                                                        XFW_TYPE_WORKSPACE_MANAGER,
                                                        G_PARAM_READABLE));

    /**
     * XfwScreen:active-window:
     *
     * The currently-active window.
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_ACTIVE_WINDOW,
                                    g_param_spec_object("active-window",
                                                        "active-window",
                                                        "active-window",
                                                        XFW_TYPE_WINDOW,
                                                        G_PARAM_READABLE));

    /**
     * XfwScreen:show-desktop:
     *
     * Whether or not to show the desktop.
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_SHOW_DESKTOP,
                                    g_param_spec_boolean("show-desktop",
                                                         "show-desktop",
                                                         "show-desktop",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY));
}

static void
xfw_screen_init(XfwScreen *screen) {}

static void
xfw_screen_constructed(GObject *object) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(object);

    G_OBJECT_CLASS(xfw_screen_parent_class)->constructed(object);

    g_signal_connect(priv->gscreen, "monitors-changed",
                     G_CALLBACK(xfw_screen_gdk_screen_monitors_changed), object);
}

static void
xfw_screen_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwScreen *screen = XFW_SCREEN(object);
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);

    switch (prop_id) {
        case PROP_SCREEN:
            priv->gscreen = g_value_get_object(value);
            break;

        case PROP_SHOW_DESKTOP:
            xfw_screen_set_show_desktop(screen, g_value_get_boolean(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwScreen *screen = XFW_SCREEN(object);
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);

    switch (prop_id) {
        case PROP_SCREEN:
            g_value_set_object(value, priv->gscreen);
            break;

        case PROP_MONITORS:
            g_value_set_pointer(value, xfw_screen_get_monitors(screen));
            break;

        case PROP_WORKSPACE_MANAGER:
            g_value_set_object(value, xfw_screen_get_workspace_manager(screen));
            break;

        case PROP_ACTIVE_WINDOW:
            g_value_set_object(value, xfw_screen_get_active_window(screen));
            break;

        case PROP_SHOW_DESKTOP:
            g_value_set_boolean(value, xfw_screen_get_show_desktop(screen));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void
xfw_screen_dispose(GObject *object) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(object);

    if (priv->gscreen != NULL) {
        g_signal_handlers_disconnect_by_func(priv->gscreen,
                                             xfw_screen_gdk_screen_monitors_changed,
                                             object);
        priv->gscreen = NULL;
    }

    G_OBJECT_CLASS(xfw_screen_parent_class)->dispose(object);
}

static void
xfw_screen_finalize(GObject *object) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(object);

    g_list_free_full(priv->monitors, g_object_unref);

    G_OBJECT_CLASS(xfw_screen_parent_class)->finalize(object);
}

static void
xfw_screen_gdk_screen_monitors_changed(GdkScreen *gscreen, XfwScreen *screen) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);

    g_list_free_full(priv->monitors, g_object_unref);
    priv->monitors = NULL;
    g_signal_emit_by_name(screen, "monitors-changed");
}

/**
 * xfw_screen_get_workspace_manager:
 * @screen: an #XfwScreen.
 *
 * Retrieves this screen's #XfwWorkspaceManager instance, which can be used
 * to inspect and interact with @screen's workspace groups and workspaces.
 *
 * Return value: (not nullable) (transfer none): a #XfwWorkspaceManager
 * instance.  This instance is a singleton and is owned by @screen.
 **/
XfwWorkspaceManager *
xfw_screen_get_workspace_manager(XfwScreen *screen) {
    XfwScreenClass *klass;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    klass = XFW_SCREEN_GET_CLASS(screen);
    return (*klass->get_workspace_manager)(screen);
}

/**
 * xfw_screen_get_windows:
 * @screen: an #XfwScreen.
 *
 * Retrieves the list of windows currently displayed on @screen.
 *
 * The list and its contents are owned by @screen.
 *
 * Return value: (nullable) (element-type XfwWindow) (transfer none): the list
 * of #XfwWindow on @screen, or %NULL if there are no windows.  The list
 * and its contents are owned by @screen.
 **/
GList *
xfw_screen_get_windows(XfwScreen *screen) {
    XfwScreenClass *klass;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    klass = XFW_SCREEN_GET_CLASS(screen);
    return (*klass->get_windows)(screen);
}

/**
 * xfw_screen_get_windows_stacked:
 * @screen: an #XfwScreen.
 *
 * Retrieves the list of windows currently displayed on @screen, in stacking
 * order, with the bottom-most window first in the returned list.
 *
 * Return value: (nullable) (element-type XfwWindow) (transfer none): the list
 * of #XfwWindow on @screen, in stacking order, or %NULL if there are no
 * windows.  The list and its contents are owned by @screen.
 **/
GList *
xfw_screen_get_windows_stacked(XfwScreen *screen) {
    XfwScreenClass *klass;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    klass = XFW_SCREEN_GET_CLASS(screen);
    return (*klass->get_windows_stacked)(screen);
}

/**
 * xfw_screen_get_active_window:
 * @screen: an #XfwScreen.
 *
 * Retrieves the window on @screen that is currently active.
 *
 * Return value: (nullable) (transfer none): an #XfwWindow, or %NULL if no
 * window is active on @screen.
 **/
XfwWindow *
xfw_screen_get_active_window(XfwScreen *screen) {
    XfwScreenClass *klass;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    klass = XFW_SCREEN_GET_CLASS(screen);
    return (*klass->get_active_window)(screen);
}

/**
 * xfw_screen_get_show_desktop:
 * @screen: an #XfwScreen.
 *
 * Return value: %TRUE if the desktop is shown, %FALSE otherwise.
 **/
gboolean
xfw_screen_get_show_desktop(XfwScreen *screen) {
    XfwScreenClass *klass;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), FALSE);
    klass = XFW_SCREEN_GET_CLASS(screen);
    return (*klass->get_show_desktop)(screen);
}

/**
 * xfw_screen_set_show_desktop:
 * @screen: an #XfwScreen.
 * @show: %TRUE to show the desktop, %FALSE to restore the previous state.
 *
 * Showing the desktop minimizes the windows not minimized at the time of the query.
 * The reverse process unminimizes those same windows, if they have not already been
 * unminimized or destroyed. The desktop show state can be tracked via
 * #XfwScreen:show-desktop.
 *
 * The state of the previously active window is always restored upon unminimization,
 * but there is no guarantee for the rest of the window stacking order on Wayland.
 *
 * A request to switch to the current state is silently ignored.
 **/
void
xfw_screen_set_show_desktop(XfwScreen *screen, gboolean show) {
    XfwScreenClass *klass;
    g_return_if_fail(XFW_IS_SCREEN(screen));
    klass = XFW_SCREEN_GET_CLASS(screen);
    (*klass->set_show_desktop)(screen, show);
}

static void
screen_destroyed(GdkScreen *gdk_screen, XfwScreen *screen) {
    g_object_steal_data(G_OBJECT(gdk_screen), GDK_SCREEN_XFW_SCREEN_KEY);
}

static XfwScreen *
xfw_screen_get(GdkScreen *gdk_screen) {
    XfwScreen *screen = XFW_SCREEN(g_object_get_data(G_OBJECT(gdk_screen), GDK_SCREEN_XFW_SCREEN_KEY));

    if (screen == NULL) {
        _libxfce4windowing_init();

#ifdef ENABLE_X11
        if (xfw_windowing_get() == XFW_WINDOWING_X11) {
            screen = g_object_new(XFW_TYPE_SCREEN_X11,
                                  "screen", gdk_screen,
                                  NULL);
        } else
#endif  /* ENABLE_X11 */
#ifdef ENABLE_WAYLAND
        if (xfw_windowing_get() == XFW_WINDOWING_WAYLAND) {
            screen = g_object_new(XFW_TYPE_SCREEN_WAYLAND,
                                  "screen", gdk_screen,
                                  NULL);
        } else
#endif
        {
            g_critical("Unknown/unsupported windowing environment");
        }

        if (screen != NULL) {
            g_object_set_data_full(G_OBJECT(gdk_screen), GDK_SCREEN_XFW_SCREEN_KEY, screen, g_object_unref);
            g_object_weak_ref(G_OBJECT(screen), (GWeakNotify)screen_destroyed, gdk_screen);
        }
    } else {
        g_object_ref(screen);
    }

    return screen;
}

/**
 * xfw_screen_get_default: (constructor)
 *
 * Retrieves the #XfwScreen instance corresponding to the default #GdkScreen.
 *
 * Return value: (not nullable) (transfer full): an #XfwScreen instance, with
 * a reference owned by the caller.
 **/
XfwScreen *
xfw_screen_get_default(void) {
    return xfw_screen_get(gdk_screen_get_default());
}

/**
 * xfw_screen_get_gdk_screen:
 * @screen: a #XfwScreen.
 *
 * Retrieves the underlying #GdkScreen of @screen.
 *
 * Return value: (not nullable) (transfer none): A #GdkScreen instance,
 * unowned.
 *
 * Since: 4.19.2
 **/
GdkScreen *
xfw_screen_get_gdk_screen(XfwScreen *screen) {
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    return XFW_SCREEN_GET_PRIVATE(screen)->gscreen;
}

/**
 * xfw_screen_get_monitors:
 * @screen: a #XfwScreen.
 *
 * Fetches the list of #XfwMonitor attached to @screen.
 *
 * Return value: (not nullable) (transfer none) (element-type XfwMonitor):
 * A #GList of #XfwMonitor, owned by @screen.  The list should not be modified
 * or freed.
 *
 * Since: 4.19.2
 **/
GList *
xfw_screen_get_monitors(XfwScreen *screen) {
    XfwScreenPrivate *priv;

    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);

    priv = XFW_SCREEN_GET_PRIVATE(screen);

    if (priv->monitors == NULL) {
        GdkDisplay *display = gdk_screen_get_display(priv->gscreen);
        gint n_monitors = gdk_display_get_n_monitors(display);

        for (gint i = 0; i < n_monitors; ++i) {
            GdkMonitor *monitor = gdk_display_get_monitor(display, i);
            priv->monitors = g_list_prepend(priv->monitors,
                                            g_object_new(XFW_TYPE_MONITOR,
                                                         "screen", screen,
                                                         "monitor", monitor,
                                                         "number", (guint)i,
                                                         NULL));
        }

        priv->monitors = g_list_reverse(priv->monitors);
    }

    return priv->monitors;
}

static inline XfwMonitor *
find_monitor_fuzzy(GList *monitors, GdkMonitor *monitor) {
    GdkRectangle geom;

    gdk_monitor_get_geometry(monitor, &geom);

    for (GList *l = monitors; l != NULL; l = l->next) {
        XfwMonitor *xmonitor = XFW_MONITOR(l->data);
        GdkRectangle this_geom;

        gdk_monitor_get_geometry(xfw_monitor_get_gdk_monitor(xmonitor), &this_geom);
        if (gdk_rectangle_equal(&geom, &this_geom)) {
            return xmonitor;
        }
    }

    return NULL;
}

static inline XfwMonitor *
find_monitor(GList *monitors, GdkMonitor *monitor) {
    for (GList *l = monitors; l != NULL; l = l->next) {
        XfwMonitor *xmonitor = XFW_MONITOR(l->data);

        if (xfw_monitor_get_gdk_monitor(xmonitor) == monitor) {
            return xmonitor;
        }
    }

    return NULL;
}

XfwMonitor *
_xfw_screen_get_monitor_for_gdk_monitor(XfwScreen *screen, GdkMonitor *monitor) {
    XfwMonitor *xmonitor = find_monitor(xfw_screen_get_monitors(screen), monitor);

    if (G_UNLIKELY(xmonitor == NULL)) {
        // This shouldn't happen, but I think sometimes
        // GdkScreen::monitors-changed fails to fire in time.
        XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);

        g_list_free(priv->monitors);
        priv->monitors = NULL;
        xmonitor = find_monitor(xfw_screen_get_monitors(screen), monitor);
    }

    if (G_UNLIKELY(xmonitor == NULL)) {
        // I really don't get how it's possible to get here, but this will be
        // our last-ditch effort.
        xmonitor = find_monitor_fuzzy(xfw_screen_get_monitors(screen), monitor);
    }

    if (G_UNLIKELY(xmonitor == NULL)) {
        g_critical("Unable to match GdkMonitor with XfwMonitor");
    }

    return xmonitor;
}
