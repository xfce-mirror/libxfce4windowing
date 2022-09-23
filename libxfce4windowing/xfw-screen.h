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

#ifndef __XFW_SCREEN_H__
#define __XFW_SCREEN_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <gdk/gdk.h>

#include "xfw-window.h"
#include "xfw-workspace-manager.h"

G_BEGIN_DECLS

#define XFW_TYPE_SCREEN           (xfw_screen_get_type())
#define XFW_SCREEN(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_SCREEN, XfwScreen))
#define XFW_IS_SCREEN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_SCREEN))
#define XFW_SCREEN_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFW_TYPE_SCREEN, XfwScreenIface))

typedef struct _XfwScreen XfwScreen;
typedef struct _XfwScreenIface XfwScreenIface;

struct _XfwScreenIface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*window_opened)(XfwScreen *screen, XfwWindow *window);
    void (*active_window_changed)(XfwScreen *screen, XfwWindow *previous_active_window);
    void (*window_stacking_changed)(XfwScreen *screen);
    void (*window_closed)(XfwScreen *screen, XfwWindow *window);

    /* Virtual Table */
    XfwWorkspaceManager *(*get_workspace_manager)(XfwScreen *screen);
    GList *(*get_windows)(XfwScreen *screen);
    GList *(*get_windows_stacked)(XfwScreen *screen);
    XfwWindow *(*get_active_window)(XfwScreen *screen);
};

GType xfw_screen_get_type(void) G_GNUC_CONST;

XfwScreen * xfw_screen_get(GdkScreen *gdk_screen);

XfwWorkspaceManager *xfw_screen_get_workspace_manager(XfwScreen *screen);
GList *xfw_screen_get_windows(XfwScreen *screen);
GList *xfw_screen_get_windows_stacked(XfwScreen *screen);
XfwWindow *xfw_screen_get_active_window(XfwScreen *screen);


G_END_DECLS

#endif  /* !__XFW_SCREEN_H__ */
