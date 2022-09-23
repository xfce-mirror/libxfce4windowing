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

#include <limits.h>

#include "libxfce4windowing-private.h"
//#include "xfw-screen-wayland.h"
#include "xfw-screen-x11.h"
#include "xfw-screen.h"
#include "xfw-util.h"
#include "xfw-window.h"

#define GDK_SCREEN_XFW_SCREEN_KEY "libxfce4windowing-xfw-screen"

typedef struct _XfwScreenIface XfwScreenInterface;
G_DEFINE_INTERFACE(XfwScreen, xfw_screen, G_TYPE_OBJECT)

static void
xfw_screen_default_init(XfwScreenIface *iface) {
    g_signal_new("window-created",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenIface, window_created),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);
    g_signal_new("active-window-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenIface, active_window_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);
    g_signal_new("window-stacking-changed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenIface, window_stacking_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);
    g_signal_new("window-closed",
                 XFW_TYPE_SCREEN,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwScreenIface, window_closed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW);

    g_object_interface_install_property(iface,
                                        g_param_spec_object("screen",
                                                            "screen",
                                                            "screen",
                                                            GDK_TYPE_SCREEN,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_interface_install_property(iface,
                                        g_param_spec_object("workspace-manager",
                                                            "workspace-manager",
                                                            "workspace-manager",
                                                            XFW_TYPE_WORKSPACE_MANAGER,
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_object("active-window",
                                                            "active-window",
                                                            "active-window",
                                                            XFW_TYPE_WINDOW,
                                                            G_PARAM_READABLE));
}

XfwWorkspaceManager *
xfw_screen_get_workspace_manager(XfwScreen *screen) {
    XfwScreenIface *iface;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    iface = XFW_SCREEN_GET_IFACE(screen);
    return (*iface->get_workspace_manager)(screen);
}

GList *
xfw_screen_get_windows(XfwScreen *screen) {
    XfwScreenIface *iface;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    iface = XFW_SCREEN_GET_IFACE(screen);
    return (*iface->get_windows)(screen);
}

GList *
xfw_screen_get_windows_stacked(XfwScreen *screen) {
    XfwScreenIface *iface;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    iface = XFW_SCREEN_GET_IFACE(screen);
    return (*iface->get_windows_stacked)(screen);
}

XfwWindow *
xfw_screen_get_active_window(XfwScreen *screen) {
    XfwScreenIface *iface;
    g_return_val_if_fail(XFW_IS_SCREEN(screen), NULL);
    iface = XFW_SCREEN_GET_IFACE(screen);
    return (*iface->get_active_window)(screen);
}

static void
screen_destroyed(GdkScreen *gdk_screen, XfwScreen *screen) {
    g_object_steal_data(G_OBJECT(gdk_screen), GDK_SCREEN_XFW_SCREEN_KEY);
}

XfwScreen *
xfw_screen_get(GdkScreen *gdk_screen) {
    XfwScreen *screen = XFW_SCREEN(g_object_get_data(G_OBJECT(gdk_screen), GDK_SCREEN_XFW_SCREEN_KEY));

    if (screen == NULL) {
#ifdef ENABLE_X11
        if (xfw_windowing_get() == XFW_WINDOWING_X11) {
            screen = g_object_new(XFW_TYPE_SCREEN_X11,
                                  "screen", gdk_screen,
                                  NULL);
        } else
#endif  /* ENABLE_X11 */
#ifdef ENABLE_WAYLAND
        if (xfw_windowing_get() == XFW_WINDOWING_WAYLAND) {
            g_critical("Wayland screen backend unimplemented");
//            screen = g_object_new(XFW_TYPE_SCREEN_WAYLAND,
//                                  "screen", gdk_screen,
//                                  NULL);
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

void
_xfw_screen_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, SCREEN_PROP_SCREEN, "screen");
    g_object_class_override_property(gklass, SCREEN_PROP_WORKSPACE_MANAGER, "workspace-manager");
    g_object_class_override_property(gklass, SCREEN_PROP_ACTIVE_WINDOW, "active-window");
}
