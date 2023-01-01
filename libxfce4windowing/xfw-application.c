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

/**
 * SECTION:xfw-application
 * @title: XfwApplication
 * @short_description: An object representing a desktop application
 * @stability: Unstable
 * @include: libxfce4windowing/libxfce4windowing.h
 *
 * #XfwApplication represents an application in the common or abstract sense,
 * i.e. it can have several windows belonging to different instances, identified
 * by their process ID.
 *
 * Note that #XfwApplication is actually an interface; when obtaining an
 * instance, an instance of a windowing-environment-specific object that
 * implements this interface will be returned.
 **/

#include "config.h"

#include "libxfce4windowing-private.h"
#include "xfw-application-private.h"

G_DEFINE_INTERFACE(XfwApplication, xfw_application, G_TYPE_OBJECT)

static void
xfw_application_default_init(XfwApplicationIface *iface) {
    /**
     * XfwApplication::icon-changed:
     * @app: the object which received the signal.
     *
     * Emitted when @app's icon changes.
     **/
    g_signal_new("icon-changed",
                 XFW_TYPE_APPLICATION,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwApplicationIface, icon_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    /**
     * XfwApplication:id:
     *
     * The #XfwWindow:id of the first window in #XfwApplication:windows.
     **/
    g_object_interface_install_property(iface,
                                        g_param_spec_uint64("id",
                                                            "id",
                                                            "id",
                                                            0, G_MAXUINT64, 0,
                                                            G_PARAM_READABLE));

    /**
     * XfwApplication:name:
     *
     * The application name.
     **/
    g_object_interface_install_property(iface,
                                        g_param_spec_string("name",
                                                            "name",
                                                            "name",
                                                            NULL,
                                                            G_PARAM_READABLE));

    /**
     * XfwApplication:windows:
     *
     * The list of #XfwWindow belonging to the application.
     **/
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("windows",
                                                             "windows",
                                                             "windows",
                                                             G_PARAM_READABLE));

    /**
     * XfwApplication:instances:
     *
     * The list of #XfwApplicationInstance belonging to the application.
     **/
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("instances",
                                                             "instances",
                                                             "instances",
                                                             G_PARAM_READABLE));
}

/**
 * xfw_application_get_id:
 * @app: an #XfwApplication.
 *
 * Fetches this application's ID, which is the #XfwWindow:id of the first window
 * in #XfwApplication:windows.
 *
 * Return value: A unique integer identifying the application.
 **/
guint64
xfw_application_get_id(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), 0);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_id)(app);
}

/**
 * xfw_application_get_name:
 * @app: an #XfwApplication.
 *
 * Fetches this application's human-readable name.
 *
 * Return value: (not nullable) (transfer none): A UTF-8 formatted string,
 * owned by @app.
 **/
const gchar *
xfw_application_get_name(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_name)(app);
}

/**
 * xfw_application_get_icon:
 * @app: an #XfwApplication.
 * @size: the desired icon size.
 * @scale: the UI scale factor.
 *
 * Fetches @app's icon.
 *
 * Return value: (nullable) (transfer none): a #GdkPixbuf, owned by @app,
 * or %NULL if @app has no icon.
 **/
GdkPixbuf *
xfw_application_get_icon(XfwApplication *app, gint size, gint scale) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_icon)(app, size, scale);
}

/**
 * xfw_application_get_gicon:
 * @app: an #XfwApplication.
 *
 * Fetches @app's icon as a size-independent #GIcon.
 *
 * Return value: (nullable) (transfer none): a #GIcon, owned by @app,
 * or %NULL if @app has no icon.
 *
 * Since: 4.19.1
 **/
GIcon *
xfw_application_get_gicon(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_gicon)(app);
}

/**
 * xfw_application_get_windows:
 * @app: an #XfwApplication.
 *
 * Lists all windows belonging to the application.
 *
 * Return value: (not nullable) (element-type XfwWindow) (transfer none):
 * The list of #XfwWindow belonging to @app. The list and its contents are owned
 * by @app.
 **/
GList *
xfw_application_get_windows(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_windows)(app);
}

/**
 * xfw_application_get_instances:
 * @app: an #XfwApplication.
 *
 * Lists all instances of the application.
 *
 * Return value: (nullable) (element-type XfwApplicationInstance) (transfer none):
 * The list of #XfwApplicationInstance of @app, or %NULL if listing instances is
 * not supported on the windowing environment in use. The list and its contents
 * are owned by @app.
 **/
GList *
xfw_application_get_instances(XfwApplication *app) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_instances)(app);
}

/**
 * xfw_application_get_instance:
 * @app: an #XfwApplication.
 * @window: the application window you want to get the instance of.
 *
 * Finds the #XfwApplicationInstance to which @window belongs.
 *
 * Return value: (nullable) (transfer none):
 * The #XfwApplicationInstance to which @window belongs, or %NULL if @window
 * does not belong to @app, or if listing instances is not supported on the
 * windowing environment in use. The returned #XfwApplicationInstance is owned
 * by @app.
 **/
XfwApplicationInstance *
xfw_application_get_instance(XfwApplication *app, XfwWindow *window) {
    XfwApplicationIface *iface;
    g_return_val_if_fail(XFW_IS_APPLICATION(app), NULL);
    iface = XFW_APPLICATION_GET_IFACE(app);
    return (*iface->get_instance)(app, window);
}

/**
 * xfw_application_instance_get_pid:
 * @instance: an #XfwApplicationInstance.
 *
 * Fetches @instance's PID.
 *
 * Return value: The process ID of @instance, or 0 if none is available.
 *
 * Since: 4.19.1
 **/
gint
xfw_application_instance_get_pid(XfwApplicationInstance *instance) {
    g_return_val_if_fail(instance != NULL, 0);
    return instance->pid;
}

/**
 * xfw_application_instance_get_name:
 * @instance: an #XfwApplicationInstance.
 *
 * Fetches @instance's name, which can often be the same as the application name.
 *
 * Return value: (not nullable) (transfer none): A string owned by @instance.
 *
 * Since: 4.19.1
 **/
const gchar *
xfw_application_instance_get_name(XfwApplicationInstance *instance) {
    g_return_val_if_fail(instance != NULL, NULL);
    return instance->name;
}

/**
 * xfw_application_instance_get_windows:
 * @instance: an #XfwApplicationInstance.
 *
 * Lists all windows belonging to the application instance.
 *
 * Return value: (not nullable) (element-type XfwWindow) (transfer none):
 * The list of #XfwWindow belonging to @instance. The list and its contents are owned
 * by @instance.
 *
 * Since: 4.19.1
 **/
GList *
xfw_application_instance_get_windows(XfwApplicationInstance *instance) {
    g_return_val_if_fail(instance != NULL, NULL);
    return instance->windows;
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
