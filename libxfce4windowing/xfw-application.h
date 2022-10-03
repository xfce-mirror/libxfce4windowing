/*
 * Copyright (c) 2022 Gaël Bonithon <gael@xfce.org>
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

#ifndef __XFW_APPLICATION_H__
#define __XFW_APPLICATION_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define XFW_TYPE_APPLICATION (xfw_application_get_type())
G_DECLARE_INTERFACE(XfwApplication, xfw_application, XFW, APPLICATION, GObject)

typedef struct _XfwApplicationInterface XfwApplicationIface;

struct _XfwApplicationInterface {
    /*< private >*/
    GTypeInterface g_iface;

    /*< public >*/

    /* Signals */
    void (*icon_changed)(XfwApplication *app);

    /* Virtual Table */
    guint64 (*get_id)(XfwApplication *app);
    const gchar *(*get_name)(XfwApplication *app);
    gint (*get_pid)(XfwApplication *app);
    GdkPixbuf *(*get_icon)(XfwApplication *app, gint size);
    GList *(*get_windows)(XfwApplication *app);
};

guint64 xfw_application_get_id(XfwApplication *app);
const gchar *xfw_application_get_name(XfwApplication *app);
gint xfw_application_get_pid(XfwApplication *app);
GdkPixbuf *xfw_application_get_icon(XfwApplication *app, gint size);
GList *xfw_application_get_windows(XfwApplication *app);

G_END_DECLS

#endif  /* !__XFW_APPLICATION_H__ */
