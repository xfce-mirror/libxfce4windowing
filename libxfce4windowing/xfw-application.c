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

#include "config.h"

#include "libxfce4windowing-private.h"
#include "xfw-application.h"

G_DEFINE_INTERFACE(XfwApplication, xfw_application, G_TYPE_OBJECT)

static void
xfw_application_default_init(XfwApplicationIface *iface) {
    g_signal_new("icon-changed",
                 XFW_TYPE_APPLICATION,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwApplicationIface, icon_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    g_object_interface_install_property(iface,
                                        g_param_spec_uint64("id",
                                                            "id",
                                                            "id",
                                                            0, G_MAXUINT64, 0,
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_string("name",
                                                            "name",
                                                            "name",
                                                            NULL,
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("windows",
                                                             "windows",
                                                             "windows",
                                                             G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("instances",
                                                             "instances",
                                                             "instances",
                                                             G_PARAM_READABLE));
}

guint64
xfw_application_get_id(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), 0);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_id)(app);
}

const gchar *
xfw_application_get_name(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_name)(app);
}

GdkPixbuf *
xfw_application_get_icon(XfwApplication *app, gint size) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_icon)(app, size);
}

GList *
xfw_application_get_windows(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_windows)(app);
}

GList *
xfw_application_get_instances(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_instances)(app);
}

XfwApplicationInstance *
xfw_application_get_instance(XfwApplication *app, XfwWindow *window) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_instance)(app, window);
}

void
_xfw_application_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, APPLICATION_PROP_ID, "id");
    g_object_class_override_property(gklass, APPLICATION_PROP_NAME, "name");
    g_object_class_override_property(gklass, APPLICATION_PROP_WINDOWS, "windows");
    g_object_class_override_property(gklass, APPLICATION_PROP_WINDOWS, "instances");
}

void
_xfw_application_instance_free(gpointer data) {
    XfwApplicationInstance *instance = data;
    g_free(instance->name);
    g_list_free(instance->windows);
    g_free(instance);
}
