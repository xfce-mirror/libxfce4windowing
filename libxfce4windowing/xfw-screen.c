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
 * Note that #XfwScreen is actually an interface; when obtaining an instance,
 * an instance of a windowing-environment-specific object that implements
 * this interface will be returned.
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>

#include "libxfce4windowing-private.h"
#include "xfw-monitor-private.h"
#include "xfw-screen-private.h"
#include "xfw-util.h"
#include "xfw-window.h"

#ifdef ENABLE_X11
#include "xfw-screen-x11.h"
#include "xfw-workspace-manager-x11.h"
#endif

#ifdef ENABLE_WAYLAND
#include "xfw-screen-wayland.h"
#include "xfw-workspace-manager-wayland.h"
#endif

#define XFW_SCREEN_GET_PRIVATE(screen) ((XfwScreenPrivate *)xfw_screen_get_instance_private((XfwScreen *)screen))
#define GDK_SCREEN_XFW_SCREEN_KEY "libxfce4windowing-xfw-screen"

typedef struct _XfwXcreenPrivate {
    GdkScreen *gdk_screen;
    XfwWorkspaceManager *workspace_manager;
    GList *monitors;
    XfwWindow *active_window;
    guint32 show_desktop : 1;
} XfwScreenPrivate;

enum {
    PROP0,
    PROP_SCREEN,
    PROP_WORKSPACE_MANAGER,
    PROP_ACTIVE_WINDOW,
    PROP_SHOW_DESKTOP,
};

static void xfw_screen_set_property(GObject *object,
                                    guint property_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void xfw_screen_get_property(GObject *object,
                                    guint property_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void xfw_screen_finalize(GObject *object);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(XfwScreen, xfw_screen, G_TYPE_OBJECT)


static void
xfw_screen_class_init(XfwScreenClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = xfw_screen_set_property;
    gobject_class->get_property = xfw_screen_get_property;
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
     * Emitted when one of the following has changed about the monitors
     * attached to @screen:
     *
     *   * A monitor was added.
     *   * A monitor was removed.
     *   * A monitor's resolution changed (including if the scale factor changed).
     *   * A monitor was re-positioned in the global screen space.
     *
     * Note that changes such as the subpixel layout or model name will not
     * result in the emission of this signal.  Connect to the propert notify
     * signals on #XfwMonitor to be notified of changes in other aspects of the
     * monitor.
     *
     * Since: 4.19.4
     **/
    g_signal_new("monitors-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 0,
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
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

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
xfw_screen_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(object);

    switch (property_id) {
        case PROP_SCREEN:
            priv->gdk_screen = g_value_get_object(value);
            break;

        case PROP_WORKSPACE_MANAGER:
            priv->workspace_manager = g_value_get_object(value);
            break;

        case PROP_SHOW_DESKTOP:
            xfw_screen_set_show_desktop(XFW_SCREEN(object), g_value_get_boolean(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfw_screen_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(object);

    switch (property_id) {
        case PROP_SCREEN:
            g_value_set_object(value, priv->gdk_screen);
            break;

        case PROP_WORKSPACE_MANAGER:
            g_value_set_object(value, priv->workspace_manager);
            break;

        case PROP_ACTIVE_WINDOW:
            g_value_set_object(value, priv->active_window);
            break;

        case PROP_SHOW_DESKTOP:
            g_value_set_boolean(value, priv->show_desktop);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfw_screen_finalize(GObject *object) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(object);

    g_list_free_full(priv->monitors, g_object_unref);

    g_object_unref(priv->workspace_manager);

    G_OBJECT_CLASS(xfw_screen_parent_class)->finalize(object);
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
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    return XFW_SCREEN_GET_PRIVATE(screen)->workspace_manager;
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
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    XfwScreenClass *klass = XFW_SCREEN_GET_CLASS(screen);
    return klass->get_windows(screen);
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
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    XfwScreenClass *klass = XFW_SCREEN_GET_CLASS(screen);
    return klass->get_windows_stacked(screen);
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
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    return XFW_SCREEN_GET_PRIVATE(screen)->active_window;
}

/**
 * xfw_screen_get_monitors:
 * @screen: an #XfwScreen.
 *
 * Retrieves the list of monitors currently attached and enabled on @screen.
 *
 * Return value: (nullable) (element-type XfwMonitor) (transfer none): the list
 * of #XfwMonitor on @screen, or %NULL if there are no connected/enabled
 * monitors.  The list and its contents are owned by @screen.
 *
 * Since: 4.19.4
 **/
GList *
xfw_screen_get_monitors(XfwScreen *screen) {
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    return XFW_SCREEN_GET_PRIVATE(screen)->monitors;
}

/**
 * xfw_screen_get_show_desktop:
 * @screen: an #XfwScreen.
 *
 * Return value: %TRUE if the desktop is shown, %FALSE otherwise.
 **/
gboolean
xfw_screen_get_show_desktop(XfwScreen *screen) {
    g_return_val_if_fail(XFW_IS_SCREEN(screen), FALSE);
    return XFW_SCREEN_GET_PRIVATE(screen)->show_desktop;
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
    g_return_if_fail(XFW_IS_SCREEN(screen));
    if (XFW_SCREEN_GET_PRIVATE(screen)->show_desktop != !!show) {
        XfwScreenClass *klass = XFW_SCREEN_GET_CLASS(screen);
        klass->set_show_desktop(screen, show);
    }
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
            XfwWorkspaceManager *workspace_manager = _xfw_workspace_manager_x11_new(gdk_screen);
            screen = g_object_new(XFW_TYPE_SCREEN_X11,
                                  "screen", gdk_screen,
                                  "workspace-manager", workspace_manager,
                                  NULL);
        } else
#endif /* ENABLE_X11 */
#ifdef ENABLE_WAYLAND
            if (xfw_windowing_get() == XFW_WINDOWING_WAYLAND)
        {
            XfwWorkspaceManager *workspace_manager = _xfw_workspace_manager_wayland_new(gdk_screen);
            screen = g_object_new(XFW_TYPE_SCREEN_WAYLAND,
                                  "screen", gdk_screen,
                                  "workspace-manager", workspace_manager,
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

GdkScreen *
_xfw_screen_get_gdk_screen(XfwScreen *screen) {
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    return XFW_SCREEN_GET_PRIVATE(screen)->gdk_screen;
}


GList *
_xfw_screen_steal_monitors(XfwScreen *screen) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);
    GList *monitors = priv->monitors;
    priv->monitors = NULL;
    return monitors;
}

void
_xfw_screen_set_monitors(XfwScreen *screen, GList *monitors, guint n_added, guint n_removed) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);
    g_list_free_full(priv->monitors, g_object_unref);
    priv->monitors = monitors;

    MonitorPendingChanges changed = 0;
    for (GList *l = monitors; l != NULL; l = l->next) {
        changed |= _xfw_monitor_notify_pending_changes(XFW_MONITOR(l->data));
    }

    if ((changed & MONITORS_CHANGED_MASK) != 0 || n_added > 0 || n_removed > 0) {
        // Only notify if what has changed is relevant to positioning or size, or if
        // a monitor was added or removed.
        g_signal_emit_by_name(screen, "monitors-changed");
    }
}

void
_xfw_screen_set_active_window(XfwScreen *screen, XfwWindow *window) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);
    if (priv->active_window != window) {
        XfwWindow *previous_window = priv->active_window;
        priv->active_window = window;

        g_object_notify(G_OBJECT(screen), "active-window");
        g_signal_emit_by_name(screen, "active-window-changed", previous_window);
    }
}

void
_xfw_screen_set_show_desktop(XfwScreen *screen, gboolean show_desktop) {
    XfwScreenPrivate *priv = XFW_SCREEN_GET_PRIVATE(screen);
    if (priv->show_desktop != show_desktop) {
        priv->show_desktop = show_desktop;
        g_object_notify(G_OBJECT(screen), "show-desktop");
    }
}
