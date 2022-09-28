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

#include <gdk/gdk.h>

#include "xfw-workspace.h"

G_BEGIN_DECLS

/* fwd decl */
typedef struct _XfwScreen XfwScreen;

#define XFW_TYPE_WINDOW           (xfw_window_get_type())
#define XFW_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), XFW_TYPE_WINDOW, XfwWindow))
#define XFW_IS_WINDOW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), XFW_TYPE_WINDOW))
#define XFW_WINDOW_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), XFW_TYPE_WINDOW, XfwWindowIface))

#define XFW_TYPE_WINDOW_TYPE         (xfw_window_type_get_type())
#define XFW_TYPE_WINDOW_STATE        (xfw_window_state_get_type())
#define XFW_TYPE_WINDOW_CAPABILITIES (xfw_window_capabilities_get_type())

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
    XFW_WINDOW_STATE_SHADED = (1 << 7),
    XFW_WINDOW_STATE_ABOVE = (1 << 8),
    XFW_WINDOW_STATE_BELOW = (1 << 9),
} XfwWindowState;

typedef enum {
    XFW_WINDOW_CAPABILITIES_NONE = 0,
    XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE = (1 << 0),
    XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE = (1 << 1),
    XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE = (1 << 2),
    XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE = (1 << 3),
    XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN = (1 << 4),
    XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN = (1 << 5),
    XFW_WINDOW_CAPABILITIES_CAN_PIN = (1 << 6),
    XFW_WINDOW_CAPABILITIES_CAN_UNPIN = (1 << 7),
    XFW_WINDOW_CAPABILITIES_CAN_SHADE = (1 << 8),
    XFW_WINDOW_CAPABILITIES_CAN_UNSHADE = (1 << 9),
    XFW_WINDOW_CAPABILITIES_CAN_MOVE = (1 << 10),
    XFW_WINDOW_CAPABILITIES_CAN_RESIZE = (1 << 11),
    XFW_WINDOW_CAPABILITIES_CAN_PLACE_ABOVE = (1 << 12),
    XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_ABOVE = (1 << 13),
    XFW_WINDOW_CAPABILITIES_CAN_PLACE_BELOW = (1 << 14),
    XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_BELOW = (1 << 15),
} XfwWindowCapabilities;

typedef enum {
  XFW_WINDOW_TYPE_NORMAL = 0,
  XFW_WINDOW_TYPE_DESKTOP = 1,
  XFW_WINDOW_TYPE_DOCK = 2,
  XFW_WINDOW_TYPE_DIALOG = 3,
  XFW_WINDOW_TYPE_TOOLBAR = 4,
  XFW_WINDOW_TYPE_MENU = 5,
  XFW_WINDOW_TYPE_UTILITY = 6,
  XFW_WINDOW_TYPE_SPLASHSCREEN = 7,
} XfwWindowType;

struct _XfwWindowIface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*name_changed)(XfwWindow *window);
    void (*icon_changed)(XfwWindow *window);
    void (*type_changed)(XfwWindow *window, XfwWindowType old_type);
    void (*state_changed)(XfwWindow *window, XfwWindowState changed_mask, XfwWindowState new_state);
    void (*capabilities_changed)(XfwWindow *window, XfwWindowCapabilities changed_mask, XfwWindowCapabilities new_capabilities);
    void (*geometry_changed)(XfwWindow *window);
    void (*workspace_changed)(XfwWindow *window);
    void (*closed)(XfwWindow *window);

    /* Virtual Table */
    guint64 (*get_id)(XfwWindow *window);
    const gchar *(*get_name)(XfwWindow *window);
    GdkPixbuf *(*get_icon)(XfwWindow *window);
    XfwWindowType (*get_window_type)(XfwWindow *window);
    XfwWindowState (*get_state)(XfwWindow *window);
    XfwWindowCapabilities (*get_capabilities)(XfwWindow *window);
    GdkRectangle *(*get_geometry)(XfwWindow *window);
    XfwScreen *(*get_screen)(XfwWindow *window);
    XfwWorkspace *(*get_workspace)(XfwWindow *window);
    GList *(*get_monitors)(XfwWindow *window);

    gboolean (*activate)(XfwWindow *window, guint64 event_timestamp, GError **error);
    gboolean (*close)(XfwWindow *window, guint64 event_timestamp, GError **error);
    gboolean (*start_move)(XfwWindow *window, GError **error);
    gboolean (*start_resize)(XfwWindow *window, GError **error);
    gboolean (*move_to_workspace)(XfwWindow *window, XfwWorkspace *workspace, GError **error);

    gboolean (*set_minimized)(XfwWindow *window, gboolean is_minimized, GError **error);
    gboolean (*set_maximized)(XfwWindow *window, gboolean is_maximized, GError **error);
    gboolean (*set_fullscreen)(XfwWindow *window, gboolean is_fullscreen, GError **error);
    gboolean (*set_skip_pager)(XfwWindow *window, gboolean is_skip_pager, GError **error);
    gboolean (*set_skip_tasklist)(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
    gboolean (*set_pinned)(XfwWindow *window, gboolean is_pinned, GError **error);
    gboolean (*set_shaded)(XfwWindow *window, gboolean is_shaded, GError **error);
    gboolean (*set_above)(XfwWindow *window, gboolean is_above, GError **error);
    gboolean (*set_below)(XfwWindow *window, gboolean is_below, GError **error);
};

GType xfw_window_get_type(void) G_GNUC_CONST;
GType xfw_window_type_get_type(void) G_GNUC_CONST;
GType xfw_window_state_get_type(void) G_GNUC_CONST;
GType xfw_window_capabilities_get_type(void) G_GNUC_CONST;

guint64 xfw_window_get_id(XfwWindow *window);
const gchar *xfw_window_get_name(XfwWindow *window);
GdkPixbuf *xfw_window_get_icon(XfwWindow *window);
XfwWindowType xfw_window_get_window_type(XfwWindow *window);
XfwWindowState xfw_window_get_state(XfwWindow *window);
XfwWindowCapabilities xfw_window_get_capabilities(XfwWindow *window);
GdkRectangle *xfw_window_get_geometry(XfwWindow *window);
XfwScreen *xfw_window_get_screen(XfwWindow *window);
XfwWorkspace *xfw_window_get_workspace(XfwWindow *window);
GList *xfw_window_get_monitors(XfwWindow *window);

gboolean xfw_window_activate(XfwWindow *window, guint64 event_timestamp, GError **error);
gboolean xfw_window_close(XfwWindow *window, guint64 event_timestamp, GError **error);
gboolean xfw_window_start_move(XfwWindow *window, GError **error);
gboolean xfw_window_start_resize(XfwWindow *window, GError **error);
gboolean xfw_window_move_to_workspace(XfwWindow *window, XfwWorkspace *workspace, GError **error);

gboolean xfw_window_set_minimized(XfwWindow *window, gboolean is_minimized, GError **error);
gboolean xfw_window_set_maximized(XfwWindow *window, gboolean is_maximized, GError **error);
gboolean xfw_window_set_fullscreen(XfwWindow *window, gboolean is_fullscreen, GError **error);
gboolean xfw_window_set_skip_pager(XfwWindow *window, gboolean is_skip_pager, GError **error);
gboolean xfw_window_set_skip_tasklist(XfwWindow *window, gboolean is_skip_tasklist, GError **error);
gboolean xfw_window_set_pinned(XfwWindow *window, gboolean is_pinned, GError **error);
gboolean xfw_window_set_shaded(XfwWindow *window, gboolean is_shaded, GError **error);
gboolean xfw_window_set_above(XfwWindow *window, gboolean is_above, GError **error);
gboolean xfw_window_set_below(XfwWindow *window, gboolean is_below, GError **error);

gboolean xfw_window_is_minimized(XfwWindow *window);
gboolean xfw_window_is_maximized(XfwWindow *window);
gboolean xfw_window_is_fullscreen(XfwWindow *window);
gboolean xfw_window_is_skip_pager(XfwWindow *window);
gboolean xfw_window_is_skip_tasklist(XfwWindow *window);
gboolean xfw_window_is_pinned(XfwWindow *window);
gboolean xfw_window_is_shaded(XfwWindow *window);
gboolean xfw_window_is_above(XfwWindow *window);
gboolean xfw_window_is_below(XfwWindow *window);

G_END_DECLS

#endif  /* !__XFW_WINDOW_H__ */
