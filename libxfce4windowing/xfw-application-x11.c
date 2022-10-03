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
#include "xfw-application-x11.h"

enum {
    PROP0,
    PROP_WNCK_APP,
};

struct _XfwApplicationX11Private {
    WnckApplication *wnck_app;
};

static GHashTable *wnck_apps = NULL;

static void xfw_application_x11_iface_init(XfwApplicationIface *iface);
static void xfw_application_x11_constructed(GObject *obj);
static void xfw_application_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_application_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_application_x11_finalize(GObject *obj);
static guint64 xfw_application_x11_get_id(XfwApplication *app);
static const gchar *xfw_application_x11_get_name(XfwApplication *app);
static gint xfw_application_x11_get_pid(XfwApplication *app);
static GdkPixbuf *xfw_application_x11_get_icon(XfwApplication *app, gint size);
static GList *xfw_application_x11_get_windows(XfwApplication *app);

static void icon_changed(WnckApplication *wnck_app, XfwApplicationX11 *app);
static void name_changed(WnckApplication *wnck_app, XfwApplicationX11 *app);

G_DEFINE_TYPE_WITH_CODE(XfwApplicationX11, xfw_application_x11, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(XfwApplicationX11)
                        G_IMPLEMENT_INTERFACE(XFW_TYPE_APPLICATION,
                                              xfw_application_x11_iface_init))

static void
xfw_application_x11_class_init(XfwApplicationX11Class *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_application_x11_constructed;
    gklass->set_property = xfw_application_x11_set_property;
    gklass->get_property = xfw_application_x11_get_property;
    gklass->finalize = xfw_application_x11_finalize;

    g_object_class_install_property(gklass,
                                    PROP_WNCK_APP,
                                    g_param_spec_object("wnck-app",
                                                        "wnck-app",
                                                        "wnck-app",
                                                        WNCK_TYPE_APPLICATION,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    _xfw_application_install_properties(gklass);
}

static void
xfw_application_x11_init(XfwApplicationX11 *app) {
    app->priv = xfw_application_x11_get_instance_private(app);
}

static void xfw_application_x11_constructed(GObject *obj) {
    XfwApplicationX11Private *priv = XFW_APPLICATION_X11(obj)->priv;

    g_hash_table_insert(wnck_apps, priv->wnck_app, obj);

    g_signal_connect(priv->wnck_app, "icon-changed", G_CALLBACK(icon_changed), obj);
    g_signal_connect(priv->wnck_app, "name-changed", G_CALLBACK(name_changed), obj);
}

static void
xfw_application_x11_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwApplicationX11Private *priv = XFW_APPLICATION_X11(obj)->priv;

    switch (prop_id) {
        case PROP_WNCK_APP:
            priv->wnck_app = g_value_dup_object(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_application_x11_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwApplication *app = XFW_APPLICATION(obj);
    XfwApplicationX11Private *priv = XFW_APPLICATION_X11(obj)->priv;

    switch (prop_id) {
        case PROP_WNCK_APP:
            g_value_set_object(value, priv->wnck_app);
            break;

        case APPLICATION_PROP_ID:
            g_value_set_uint64(value, xfw_application_x11_get_id(app));
            break;

        case APPLICATION_PROP_NAME:
            g_value_set_string(value, xfw_application_x11_get_name(app));
            break;

        case APPLICATION_PROP_PID:
            g_value_set_int(value, xfw_application_x11_get_pid(app));
            break;

        case APPLICATION_PROP_WINDOWS:
            g_value_set_pointer(value, xfw_application_x11_get_windows(app));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_application_x11_finalize(GObject *obj) {
    XfwApplicationX11Private *priv = XFW_APPLICATION_X11(obj)->priv;

    g_hash_table_remove(wnck_apps, priv->wnck_app);
    if (g_hash_table_size(wnck_apps) == 0) {
        g_hash_table_destroy(wnck_apps);
        wnck_apps = NULL;
    }

    g_signal_handlers_disconnect_by_func(priv->wnck_app, icon_changed, obj);
    g_signal_handlers_disconnect_by_func(priv->wnck_app, name_changed, obj);

    // to be released last
    g_object_unref(priv->wnck_app);

    G_OBJECT_CLASS(xfw_application_x11_parent_class)->finalize(obj);
}

static void
xfw_application_x11_iface_init(XfwApplicationIface *iface) {
    iface->get_id = xfw_application_x11_get_id;
    iface->get_name = xfw_application_x11_get_name;
    iface->get_pid = xfw_application_x11_get_pid;
    iface->get_icon = xfw_application_x11_get_icon;
    iface->get_windows = xfw_application_x11_get_windows;
}

static guint64
xfw_application_x11_get_id(XfwApplication *app) {
    return wnck_application_get_xid(XFW_APPLICATION_X11(app)->priv->wnck_app);
}

static const gchar *
xfw_application_x11_get_name(XfwApplication *app) {
    return wnck_application_get_name(XFW_APPLICATION_X11(app)->priv->wnck_app);
}

static gint
xfw_application_x11_get_pid(XfwApplication *app) {
    return wnck_application_get_pid(XFW_APPLICATION_X11(app)->priv->wnck_app);
}

static GdkPixbuf *
xfw_application_x11_get_icon(XfwApplication *app, gint size) {
    XfwApplicationX11Private *priv = XFW_APPLICATION_X11(app)->priv;
    if (size < WNCK_DEFAULT_ICON_SIZE) {
        return wnck_application_get_mini_icon(priv->wnck_app);
    }
    return wnck_application_get_icon(priv->wnck_app);
}

static GList *
xfw_application_x11_get_windows(XfwApplication *app) {
    return wnck_application_get_windows(XFW_APPLICATION_X11(app)->priv->wnck_app);
}

static void
icon_changed(WnckApplication *wnck_app, XfwApplicationX11 *app) {
    g_signal_emit_by_name(app, "icon-changed");
}

static void
name_changed(WnckApplication *wnck_app, XfwApplicationX11 *app) {
    g_object_notify(G_OBJECT(app), "name");
}

XfwApplicationX11 *
_xfw_application_x11_get(WnckApplication *wnck_app) {
    XfwApplicationX11 *app = NULL;

    if (wnck_apps == NULL) {
        wnck_apps = g_hash_table_new(g_direct_hash, g_direct_equal);
    } else {
        app = g_hash_table_lookup(wnck_apps, wnck_app);
    }

    if (app == NULL) {
        app = g_object_new(XFW_TYPE_APPLICATION_X11,
                           "wnck-app", wnck_app,
                           NULL);
    } else {
        g_object_ref(app);
    }

    return app;
}
