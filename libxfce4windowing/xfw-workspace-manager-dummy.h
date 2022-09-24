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

#ifndef __XFW_WORKSPACE_MANAGER_DUMMY_H__
#define __XFW_WORKSPACE_MANAGER_DUMMY_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <gdk/gdk.h>

#include "xfw-workspace-manager.h"

G_BEGIN_DECLS

#define XFW_TYPE_WORKSPACE_MANAGER_DUMMY    (xfw_workspace_manager_dummy_get_type())
#define XFW_WORKSPACE_MANAGER_DUMMY(obj)    (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WORKSPACE_MANAGER_DUMMY, XfwWorkspaceManagerDummy))
#define XFW_IS_WORKSPACE_MANAGER_DUMMY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WORKSPACE_MANAGER_DUMMY))

typedef struct _XfwWorkspaceManagerDummy XfwWorkspaceManagerDummy;
typedef struct _XfwWorkspaceManagerDummyPrivate XfwWorkspaceManagerDummyPrivate;
typedef struct _XfwWorkspaceManagerDummyClass XfwWorkspaceManagerDummyClass;

struct _XfwWorkspaceManagerDummy {
    GObject parent;

    /*< private >*/
    XfwWorkspaceManagerDummyPrivate *priv;
};

struct _XfwWorkspaceManagerDummyClass {
    GObjectClass parent_class;
};

GType xfw_workspace_manager_dummy_get_type() G_GNUC_CONST;

XfwWorkspaceManager *_xfw_workspace_manager_dummy_new(GdkScreen *screen);

G_END_DECLS

#endif  /* __XFW_WORKSPACE_MANAGER_DUMMY_H__ */
