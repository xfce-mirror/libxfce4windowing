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

#include <gtk/gtk.h>

#include "libxfce4windowing-private.h"
#include "xfw-screen-wayland.h"
#include "xfw-application.h"
#include "xfw-application-wayland.h"

enum {
    PROP0,
    PROP_APP_ID,
};

struct _XfwApplicationWaylandPrivate {
    gchar *app_id;
    gchar *name;
    GdkPixbuf *icon;
    gchar *icon_name;
    gint icon_size;
    GList *windows;
    GList *instances;
};

static GHashTable *app_ids = NULL;

static void xfw_application_wayland_iface_init(XfwApplicationIface *iface);
static void xfw_application_wayland_constructed(GObject *obj);
static void xfw_application_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_application_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_application_wayland_finalize(GObject *obj);
static guint64 xfw_application_wayland_get_id(XfwApplication *app);
static const gchar *xfw_application_wayland_get_name(XfwApplication *app);
static GdkPixbuf *xfw_application_wayland_get_icon(XfwApplication *app, gint size);
static GList *xfw_application_wayland_get_windows(XfwApplication *app);
static GList *xfw_application_wayland_get_instances(XfwApplication *app);
static XfwApplicationInstance *xfw_application_wayland_get_instance(XfwApplication *app, XfwWindow *window);

G_DEFINE_TYPE_WITH_CODE(XfwApplicationWayland, xfw_application_wayland, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwApplicationWayland)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_APPLICATION,
                                              xfw_application_wayland_iface_init))

static void
xfw_application_wayland_class_init(XfwApplicationWaylandClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_application_wayland_constructed;
    gklass->set_property = xfw_application_wayland_set_property;
    gklass->get_property = xfw_application_wayland_get_property;
    gklass->finalize = xfw_application_wayland_finalize;

    g_object_class_install_property(gklass,
                                    PROP_APP_ID,
                                    g_param_spec_string("app-id",
                                                        "app-id",
                                                        "app-id",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_application_install_properties(gklass);
}

static void
xfw_application_wayland_init(XfwApplicationWayland *app) {
    app->priv = xfw_application_wayland_get_instance_private(app);
}

static void xfw_application_wayland_constructed(GObject *obj) {
    XfwApplicationWaylandPrivate *priv = XFW_APPLICATION_WAYLAND(obj)->priv;
    GDesktopAppInfo *app_info;

    g_hash_table_insert(app_ids, priv->app_id, obj);

    app_info = xfw_g_desktop_app_info_get(priv->app_id);
    if (app_info != NULL) {
        gchar *name = g_desktop_app_info_get_string(app_info, G_KEY_FILE_DESKTOP_KEY_NAME);
        gchar *icon_name = g_desktop_app_info_get_string(app_info, G_KEY_FILE_DESKTOP_KEY_ICON);
        if (name != NULL) {
            priv->name = name;
            g_object_notify(obj, "name");
        }
        if (icon_name != NULL) {
            priv->icon_name = icon_name;
            g_signal_emit_by_name(obj, "icon-changed");
        }
        g_object_unref(app_info);
    }
    if (priv->name == NULL) {
        priv->name = g_strdup_printf("%c%s", g_unichar_totitle(*priv->app_id), priv->app_id + 1);
        g_object_notify(obj, "name");
    }
}

static void
xfw_application_wayland_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwApplicationWaylandPrivate *priv = XFW_APPLICATION_WAYLAND(obj)->priv;

    switch (prop_id) {
        case PROP_APP_ID:
            priv->app_id = g_value_dup_string(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_application_wayland_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwApplication *app = XFW_APPLICATION(obj);
    XfwApplicationWaylandPrivate *priv = XFW_APPLICATION_WAYLAND(obj)->priv;

    switch (prop_id) {
        case PROP_APP_ID:
            g_value_set_string(value, priv->app_id);
            break;

        case APPLICATION_PROP_ID:
            g_value_set_uint64(value, xfw_application_wayland_get_id(app));
            break;

        case APPLICATION_PROP_NAME:
            g_value_set_string(value, xfw_application_wayland_get_name(app));
            break;

        case APPLICATION_PROP_WINDOWS:
            g_value_set_pointer(value, xfw_application_wayland_get_windows(app));
            break;

        case APPLICATION_PROP_INSTANCES:
            g_value_set_pointer(value, xfw_application_wayland_get_instances(app));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_application_wayland_finalize(GObject *obj) {
    XfwApplicationWaylandPrivate *priv = XFW_APPLICATION_WAYLAND(obj)->priv;

    g_hash_table_remove(app_ids, priv->app_id);
    if (g_hash_table_size(app_ids) == 0) {
        g_hash_table_destroy(app_ids);
        app_ids = NULL;
    }

    g_free(priv->app_id);
    g_free(priv->name);
    if (priv->icon != NULL) {
        g_object_unref(priv->icon);
    }
    g_free(priv->icon_name);
    for (GList *lp = priv->windows; lp != NULL; lp = lp->next) {
        g_signal_handlers_disconnect_by_data(lp->data, obj);
    }
    g_list_free(priv->windows);
    g_list_free(priv->instances);

    G_OBJECT_CLASS(xfw_application_wayland_parent_class)->finalize(obj);
}

static void
xfw_application_wayland_iface_init(XfwApplicationIface *iface) {
    iface->get_id = xfw_application_wayland_get_id;
    iface->get_name = xfw_application_wayland_get_name;
    iface->get_icon = xfw_application_wayland_get_icon;
    iface->get_windows = xfw_application_wayland_get_windows;
    iface->get_instances = xfw_application_wayland_get_instances;
    iface->get_instance = xfw_application_wayland_get_instance;
}

static guint64
xfw_application_wayland_get_id(XfwApplication *app) {
    return xfw_window_get_id(XFW_APPLICATION_WAYLAND(app)->priv->windows->data);
}

static const gchar *
xfw_application_wayland_get_name(XfwApplication *app) {
    return XFW_APPLICATION_WAYLAND(app)->priv->name;
}

static GdkPixbuf *
xfw_application_wayland_get_icon(XfwApplication *app, gint size) {
    XfwApplicationWaylandPrivate *priv = XFW_APPLICATION_WAYLAND(app)->priv;

    if (priv->icon_name != NULL && (priv->icon == NULL || size != priv->icon_size)) {
        GdkScreen *screen = _xfw_screen_wayland_get_gdk_screen(XFW_SCREEN_WAYLAND(xfw_window_get_screen(priv->windows->data)));
        GtkIconTheme *itheme = gtk_icon_theme_get_for_screen(screen);
        GError *error = NULL;
        GdkPixbuf *icon = gtk_icon_theme_load_icon(itheme, priv->icon_name, size, 0, &error);
        priv->icon_size = size;
        if (icon != NULL) {
            if (priv->icon != NULL) {
                g_object_unref(priv->icon);
            }
            priv->icon = icon;
        } else {
            g_message("Failed to load icon for app '%s': %s", priv->app_id, error->message);
            g_error_free(error);
        }
    }

    return priv->icon;
}

static GList *
xfw_application_wayland_get_windows(XfwApplication *app) {
    return XFW_APPLICATION_WAYLAND(app)->priv->windows;
}

static GList *
xfw_application_wayland_get_instances(XfwApplication *app) {
    return XFW_APPLICATION_WAYLAND(app)->priv->instances;
}

static XfwApplicationInstance *
xfw_application_wayland_get_instance(XfwApplication *app, XfwWindow *window) {
    // TODO
    return NULL;
}

static void
window_closed(XfwWindowWayland *window, XfwApplicationWayland *app) {
    g_signal_handlers_disconnect_by_data(window, app);
    app->priv->windows = g_list_remove(app->priv->windows, window);
    g_object_notify(G_OBJECT(app), "windows");
}

static void
window_application_changed(XfwWindowWayland *window, GParamSpec *pspec, XfwApplicationWayland *app) {
    if (XFW_APPLICATION(app) != xfw_window_get_application(XFW_WINDOW(window))) {
        g_signal_handlers_disconnect_by_data(window, app);
        app->priv->windows = g_list_remove(app->priv->windows, window);
        g_object_notify(G_OBJECT(app), "windows");
    }
}

XfwApplicationWayland *
_xfw_application_wayland_get(XfwWindowWayland *window, const gchar *app_id) {
    XfwApplicationWayland *app = NULL;

    if (app_ids == NULL) {
        app_ids = g_hash_table_new(g_str_hash, g_str_equal);
    } else {
        app = g_hash_table_lookup(app_ids, app_id);
    }

    if (app == NULL) {
        app = g_object_new(XFW_TYPE_APPLICATION_WAYLAND,
                           "app-id", app_id,
                           NULL);
    } else {
        g_object_ref(app);
    }

    app->priv->windows = g_list_prepend(app->priv->windows, window);
    g_signal_connect(window, "closed", G_CALLBACK(window_closed), app);
    g_signal_connect(window, "notify::application", G_CALLBACK(window_application_changed), app);
    g_object_notify(G_OBJECT(app), "windows");

    return app;
}
