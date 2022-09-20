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

#ifndef __XFW_WORKSPACE_H__
#define __XFW_WORKSPACE_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFW_TYPE_WORKSPACE           (xfw_workspace_get_type())
#define XFW_WORKSPACE(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WORKSPACE, XfwWorkspace))
#define XFW_IS_WORKSPACE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WORKSPACE))
#define XFW_WORKSPACE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFW_TYPE_WORKSPACE, XfwWorkspaceIface))

typedef struct _XfwWorkspace XfwWorkspace;
typedef struct _XfwWorkspaceIface XfwWorkspaceIface;

typedef enum {
    XFW_WORKSPACE_STATE_NONE = 0,
    XFW_WORKSPACE_STATE_ACTIVE = (1 << 0),
    XFW_WORKSPACE_STATE_URGENT = (1 << 1),
    XFW_WORKSPACE_STATE_HIDDEN = (1 << 2)
} XfwWorkspaceState;

struct _XfwWorkspaceIface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*name_changed)(XfwWorkspace *workspace);
    void (*state_changed)(XfwWorkspace *workspace);

    /* Virtual Table */
    const gchar *(*get_id)(XfwWorkspace *workspace);
    const gchar *(*get_name)(XfwWorkspace *workspace);
    XfwWorkspaceState (*get_state)(XfwWorkspace *workspace);
    void (*activate)(XfwWorkspace *workspace, GError **error);
    void (*remove)(XfwWorkspace *workspace, GError **error);
};

GType xfw_workspace_get_type(void) G_GNUC_CONST;

const gchar *xfw_workspace_get_id(XfwWorkspace *workspace);
const gchar *xfw_workspace_get_name(XfwWorkspace *workspace);
XfwWorkspaceState xfw_workspace_get_state(XfwWorkspace *workspace);

void xfw_workspace_activate(XfwWorkspace *workspace, GError **error);
void xfw_workspace_remove(XfwWorkspace *workspace, GError **error);

G_END_DECLS

#endif  /* !__XFW_WORKSPACE_H__ */
