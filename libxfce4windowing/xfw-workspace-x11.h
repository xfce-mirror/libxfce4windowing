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

#ifndef __XFW_WORKSPACE_X11_H__
#define __XFW_WORKSPACE_X11_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define XFW_TYPE_WORKSPACE_X11           (xfw_workspace_x11_get_type())
#define XFW_WORKSPACE_X11(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WORKSPACE_X11, XfwWorkspaceX11))
#define XFW_IS_WORKSPACE_X11(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WORKSPACE_X11))

typedef struct _XfwWorkspaceX11 XfwWorkspaceX11;
typedef struct _XfwWorkspaceX11Private XfwWorkspaceX11Private;
typedef struct _XfwWorkspaceX11Class XfwWorkspaceX11Class;

struct _XfwWorkspaceX11 {
    GObject parent;
    /*< private >*/
    XfwWorkspaceX11Private *priv;
};

struct _XfwWorkspaceX11Class {
    GObjectClass parent_class;
};

GType xfw_workspace_x11_get_type(void) G_GNUC_CONST;

WnckWorkspace *_xfw_workspace_x11_get_wnck_workspace(XfwWorkspaceX11 *workspace);

#endif  /* __XFW_WORKSPACE_X11_H__ */
