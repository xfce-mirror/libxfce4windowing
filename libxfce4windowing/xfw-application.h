/*
 * Copyright (c) 2022 Gaël Bonithon <gael@xfce.org>
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

#ifndef __XFW_APPLICATION_H__
#define __XFW_APPLICATION_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

/* fwd decl */
typedef struct _XfwWindow XfwWindow;

#define XFW_TYPE_APPLICATION (xfw_application_get_type())
G_DECLARE_INTERFACE(XfwApplication, xfw_application, XFW, APPLICATION, GObject)

typedef struct _XfwApplicationInterface XfwApplicationIface;
typedef struct _XfwApplicationInstance XfwApplicationInstance;

struct _XfwApplicationInterface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*icon_changed)(XfwApplication *app);

    /* Virtual Table */
    guint64 (*get_id)(XfwApplication *app);
    const gchar *(*get_name)(XfwApplication *app);
    GdkPixbuf *(*get_icon)(XfwApplication *app, gint size);
    GList *(*get_windows)(XfwApplication *app);
    GList *(*get_instances)(XfwApplication *app);
    XfwApplicationInstance *(*get_instance)(XfwApplication *app, XfwWindow *window);
};

/**
 * XfwApplicationInstance:
 * @pid: the process ID.
 * @name: the instance name, which can often be the same as the application name.
 * @windows: the list of #XfwWindow belonging to the instance.
 *
 * A structure representing an instance of an #XfwApplication.
 **/
struct _XfwApplicationInstance {
    gint pid;
    gchar *name;
    GList *windows;
};

guint64 xfw_application_get_id(XfwApplication *app);
const gchar *xfw_application_get_name(XfwApplication *app);
GdkPixbuf *xfw_application_get_icon(XfwApplication *app, gint size);
GList *xfw_application_get_windows(XfwApplication *app);
GList *xfw_application_get_instances(XfwApplication *app);
XfwApplicationInstance *xfw_application_get_instance(XfwApplication *app, XfwWindow *window);

G_END_DECLS

#endif  /* !__XFW_APPLICATION_H__ */
