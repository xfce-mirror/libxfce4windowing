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

#ifndef __XFW_SCREEN_PRIVATE_H__
#define __XFW_SCREEN_PRIVATE_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include "xfw-screen.h"

G_BEGIN_DECLS

struct _XfwScreenInterface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*window_opened)(XfwScreen *screen, XfwWindow *window);
    void (*active_window_changed)(XfwScreen *screen, XfwWindow *previous_active_window);
    void (*window_stacking_changed)(XfwScreen *screen);
    void (*window_closed)(XfwScreen *screen, XfwWindow *window);
    void (*window_manager_changed)(XfwScreen *screen);

    /* Virtual Table */
    XfwWorkspaceManager *(*get_workspace_manager)(XfwScreen *screen);

    GList *(*get_windows)(XfwScreen *screen);
    GList *(*get_windows_stacked)(XfwScreen *screen);
    XfwWindow *(*get_active_window)(XfwScreen *screen);

    GList *(*get_monitors)(XfwScreen *screen);

    gboolean (*get_show_desktop)(XfwScreen *screen);
    void (*set_show_desktop)(XfwScreen *screen, gboolean show);
};

G_END_DECLS

#endif  /* !__XFW_SCREEN_PRIVATE_H__ */
