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

#ifndef __XFW_WINDOW_H__
#define __XFW_WINDOW_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "xfw-workspace.h"

G_BEGIN_DECLS

#define XFW_TYPE_WINDOW           (xfw_window_get_type())
#define XFW_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WINDOW, XfwWindow))
#define XFW_IS_WINDOW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WINDOW))
#define XFW_WINDOW_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFW_TYPE_WINDOW, XfwWindowIface))

#define XFW_TYPE_WINDOW_STATE     (xfw_window_state_get_type())

typedef struct _XfwWindow XfwWindow;
typedef struct _XfwWindowIface XfwWindowIface;

typedef enum {
    XFW_WINDOW_STATE_NONE = 0,
    XFW_WINDOW_STATE_ACTIVE = (1 << 0),
    XFW_WINDOW_STATE_MINIMIZED = (1 << 1),
    XFW_WINDOW_STATE_MAXIMIZED = (1 << 2),
    XFW_WINDOW_STATE_FULLSCREEN = (1 << 3),
    XFW_WINDOW_STATE_SKIP_PAGER = (1 << 4),
    XFW_WINDOW_STATE_SKIP_TASKLIST = (1 << 5),
    XFW_WINDOW_STATE_PINNED = (1 << 6),
} XfwWindowState;

struct _XfwWindowIface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*name_changed)(XfwWindow *window);
    void (*icon_changed)(XfwWindow *window);
    void (*state_changed)(XfwWindow *window, XfwWindowState changed_mask, XfwWindowState new_state);
    void (*workspace_changed)(XfwWindow* window, XfwWorkspace *old_workspace);
    void (*closed)(XfwWindow *window);

    /* Virtual Table */
    guint64 (*get_id)(XfwWindow *window);
    const gchar *(*get_name)(XfwWindow *window);
    GdkPixbuf *(*get_icon)(XfwWindow *window);
    XfwWindowState (*get_state)(XfwWindow *window);
    XfwWorkspace *(*get_workspace)(XfwWindow *window);

    void (*activate)(XfwWindow *window, guint64 event_timestamp, GError **error);
    void (*close)(XfwWindow *window, guint64 event_timestamp, GError **error);
    void (*set_minimized)(XfwWindow *window, gboolean is_minimized, GError **error);
    void (*set_maximized)(XfwWindow *window, gboolean is_maximized, GError **error);
    void (*set_fullscreen)(XfwWindow *window, gboolean is_fullscreen, GError **error);
    void (*set_skip_pager)(XfwWindow *window, gboolean is_skip_pager, GError **error);
    void (*set_skip_tasklist)(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
    void (*set_pinned)(XfwWindow *window, gboolean is_pinned, GError **error);
};

GType xfw_window_get_type(void) G_GNUC_CONST;
GType xfw_window_state_get_type(void) G_GNUC_CONST;

guint64 xfw_window_get_id(XfwWindow *window);
const gchar *xfw_window_get_name(XfwWindow *window);
GdkPixbuf *xfw_window_get_icon(XfwWindow *window);
XfwWindowState xfw_window_get_state(XfwWindow *window);
XfwWorkspace *xfw_window_get_workspace(XfwWindow *window);

void xfw_window_activate(XfwWindow *window, guint64 event_timestamp, GError **error);
void xfw_window_close(XfwWindow *window, guint64 event_timestamp, GError **error);

void xfw_window_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error);
void xfw_window_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error);
void xfw_window_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error);
void xfw_window_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error);
void xfw_window_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
void xfw_window_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error);

gboolean xfw_window_is_minimized(XfwWindow *window);
gboolean xfw_window_is_maximized(XfwWindow *window);
gboolean xfw_window_is_fullscreen(XfwWindow *window);
gboolean xfw_window_is_skip_pager(XfwWindow *window);
gboolean xfw_window_is_skip_tasklist(XfwWindow *window);
gboolean xfw_window_is_pinned(XfwWindow *window);

G_END_DECLS

#endif  /* !__XFW_WINDOW_H__ */
