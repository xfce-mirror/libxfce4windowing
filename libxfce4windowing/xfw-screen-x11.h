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

#ifndef __XFW_SCREEN_X11_H__
#define __XFW_SCREEN_X11_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>
#include <libwnck/libwnck.h>

#include "xfw-workspace.h"

G_BEGIN_DECLS

#define XFW_TYPE_SCREEN_X11 (xfw_screen_x11_get_type())
G_DECLARE_FINAL_TYPE(XfwScreenX11, xfw_screen_x11, XFW, SCREEN_X11, GObject)

typedef struct _XfwScreenX11Private XfwScreenX11Private;

struct _XfwScreenX11 {
    GObject parent;
    /*< private >*/
    XfwScreenX11Private *priv;
};

XfwWorkspace *_xfw_screen_x11_workspace_for_wnck_workspace(XfwScreenX11 *screen, WnckWorkspace *wnck_workspace);

GList *_xfw_screen_x11_steal_monitors(XfwScreenX11 *screen);
void _xfw_screen_x11_set_monitors(XfwScreenX11 *screen, GList *monitors);

G_END_DECLS

#endif /* __XFW_SCREEN_X11_H__ */
