/*
 * Copyright (c) 2024 Brian Tarricone <brian@tarricone.org>
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
 * SECTION:xfw-monitor
 * @title: XfwMonitor
 * @short_description: An object representing a physical monitor
 * @stability: Unstable
 * @include: libxfce4windowing/libxfce4windowing.h
 *
 * #XfwMonitor represents a physical monitor connected to the #XfwScreen.
 *
 * In some virtual environments (e.g. a nested X11 or Wayland session), the
 * monitor might instead represent an on-screen window that contains the output
 * of the virtual environment.
 *
 * Since: 4.19.4
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxfce4windowing-private.h"
#include "xfw-monitor-private.h"

#define XFW_MONITOR_GET_PRIVATE(monitor) ((XfwMonitorPrivate *)xfw_monitor_get_instance_private(XFW_MONITOR(monitor)))

typedef struct _XfwMonitorPrivate {
    char *identifier;
    char *description;
    char *connector;
    char *make;
    char *model;
    char *serial;
    guint refresh;
    guint scale;
    GdkRectangle physical_geometry;
    GdkRectangle logical_geometry;
    guint width_mm;
    guint height_mm;
    XfwMonitorSubpixel subpixel;
    XfwMonitorTransform transform;

    MonitorPendingChanges pending_changes;
} XfwMonitorPrivate;

enum {
    PROP0,
    PROP_IDENTIFIER,
    PROP_DESCRIPTION,
    PROP_CONNECTOR,
    PROP_MAKE,
    PROP_MODEL,
    PROP_SERIAL,
    PROP_REFRESH,
    PROP_SCALE,
    PROP_PHYSICAL_GEOMETRY,
    PROP_LOGICAL_GEOMETRY,
    PROP_PHYSICAL_WIDTH,
    PROP_PHYSICAL_HEIGHT,
    PROP_SUBPIXEL,
    PROP_TRANSFORM,
};

static void xfw_monitor_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void xfw_monitor_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void xfw_monitor_finalize(GObject *object);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(XfwMonitor, xfw_monitor, G_TYPE_OBJECT)

G_DEFINE_ENUM_TYPE(
    XfwMonitorTransform,
    xfw_monitor_transform,
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_NORMAL, "normal"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_90, "90"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_180, "180"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_270, "270"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_FLIPPED, "flipped"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_FLIPPED_90, "flipped-90"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_FLIPPED_180, "flipped-180"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_TRANSFORM_FLIPPED_270, "flipped-270"))

G_DEFINE_ENUM_TYPE(
    XfwMonitorSubpixel,
    xfw_monitor_subpixel,
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_SUBPIXEL_UNKNOWN, "unknown"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_SUBPIXEL_NONE, "none"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_SUBPIXEL_HRGB, "hrgb"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_SUBPIXEL_HBGR, "hbgr"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_SUBPIXEL_VRGB, "vrgb"),
    G_DEFINE_ENUM_VALUE(XFW_MONITOR_SUBPIXEL_VBGR, "vbgr"))


static void
xfw_monitor_class_init(XfwMonitorClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = xfw_monitor_set_property;
    gobject_class->get_property = xfw_monitor_get_property;
    gobject_class->finalize = xfw_monitor_finalize;

    /**
     * XfwMonitor:identifier:
     *
     * Opaque, hopefully-unique monitor identifier.
     *
     * Since: 4.19.4
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_IDENTIFIER,
                                    g_param_spec_string("identifier",
                                                        "identifier",
                                                        "Opaque, hopefully-unique monitor identifier",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    /**
     * XfwMonitor:description:
     *
     * Human-readable description.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_DESCRIPTION,
                                    g_param_spec_string("description",
                                                        "description",
                                                        "Human-readable description",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:connector:
     *
     * Physical/virtual connector name.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_CONNECTOR,
                                    g_param_spec_string("connector",
                                                        "connector",
                                                        "Physical/virtual connector name",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:make:
     *
     * Manufacturer name.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_MAKE,
                                    g_param_spec_string("make",
                                                        "make",
                                                        "Manufacturer name",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:model:
     *
     * Product model name.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_MODEL,
                                    g_param_spec_string("model",
                                                        "model",
                                                        "Product model name",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:serial:
     *
     * Product serial number.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_SERIAL,
                                    g_param_spec_string("serial",
                                                        "serial",
                                                        "Product serial number",
                                                        NULL,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:refresh:
     *
     * Current refresh rate, in millihertz.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_REFRESH,
                                    g_param_spec_uint("refresh",
                                                      "refresh",
                                                      "Current refresh rate, in millihertz",
                                                      0, G_MAXUINT, 60000,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:scale:
     *
     * UI scaling factor.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_SCALE,
                                    g_param_spec_uint("scale",
                                                      "scale",
                                                      "UI scaling factor",
                                                      1, G_MAXUINT, 1,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:physical-geometry:
     *
     * Coordinates and size of the monitor in physical device pixels.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_PHYSICAL_GEOMETRY,
                                    g_param_spec_boxed("physical-geometry",
                                                       "physical-geometry",
                                                       "Coordinates and size of the monitor in physical device pixels",
                                                       GDK_TYPE_RECTANGLE,
                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:logical-geometry:
     *
     * Coordinates and size of the monitor in scaled logical pixels.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_LOGICAL_GEOMETRY,
                                    g_param_spec_boxed("logical-geometry",
                                                       "logical-geometry",
                                                       "Coordinates and size of the monitor in scaled logical pixels",
                                                       GDK_TYPE_RECTANGLE,
                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:width-mm:
     *
     * Physical width of the monitor in millimeters.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_PHYSICAL_WIDTH,
                                    g_param_spec_uint("width-mm",
                                                      "width-mm",
                                                      "Physical width of the monitor in millimeters",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:height-mm:
     *
     * Physical height of the monitor in millimeters.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_PHYSICAL_HEIGHT,
                                    g_param_spec_uint("height-mm",
                                                      "height-mm",
                                                      "Physical height of the monitor in millimeters",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:subpixel:
     *
     * Hardware subpixel layout.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_SUBPIXEL,
                                    g_param_spec_enum("subpixel",
                                                      "subpixel",
                                                      "Hardware subpixel layout",
                                                      XFW_TYPE_MONITOR_SUBPIXEL,
                                                      XFW_MONITOR_SUBPIXEL_UNKNOWN,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    /**
     * XfwMonitor:transorm
     *
     * Rotation and reflextion of the monitor's contents.
     *
     * Since: 4.19.4
     **/
    g_object_class_install_property(gobject_class,
                                    PROP_TRANSFORM,
                                    g_param_spec_enum("transform",
                                                      "transform",
                                                      "Rotation and reflection of the monitor's contents",
                                                      XFW_TYPE_MONITOR_TRANSFORM,
                                                      XFW_MONITOR_TRANSFORM_NORMAL,
                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
xfw_monitor_init(XfwMonitor *monitor) {
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    priv->refresh = 60000;
    priv->scale = 1;
}

static void
xfw_monitor_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void
xfw_monitor_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(object);

    switch (property_id) {
        case PROP_IDENTIFIER:
            g_value_set_string(value, priv->identifier);
            break;

        case PROP_DESCRIPTION:
            g_value_set_string(value, priv->description);
            break;

        case PROP_CONNECTOR:
            g_value_set_string(value, priv->connector);
            break;

        case PROP_MAKE:
            g_value_set_string(value, priv->make);
            break;

        case PROP_MODEL:
            g_value_set_string(value, priv->model);
            break;

        case PROP_SERIAL:
            g_value_set_string(value, priv->serial);
            break;

        case PROP_REFRESH:
            g_value_set_uint(value, priv->refresh);
            break;

        case PROP_SCALE:
            g_value_set_uint(value, priv->scale);
            break;

        case PROP_PHYSICAL_GEOMETRY:
            g_value_set_boxed(value, &priv->physical_geometry);
            break;

        case PROP_LOGICAL_GEOMETRY:
            g_value_set_boxed(value, &priv->logical_geometry);
            break;

        case PROP_PHYSICAL_WIDTH:
            g_value_set_uint(value, priv->width_mm);
            break;

        case PROP_PHYSICAL_HEIGHT:
            g_value_set_uint(value, priv->height_mm);
            break;

        case PROP_SUBPIXEL:
            g_value_set_enum(value, priv->subpixel);
            break;

        case PROP_TRANSFORM:
            g_value_set_enum(value, priv->transform);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void
xfw_monitor_finalize(GObject *object) {
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(object);

    g_free(priv->identifier);
    g_free(priv->description);
    g_free(priv->connector);
    g_free(priv->make);
    g_free(priv->model);
    g_free(priv->serial);

    G_OBJECT_CLASS(xfw_monitor_parent_class)->finalize(object);
}

/**
 * xfw_monitor_get_identifier:
 * @monitor: a #XfwMonitor.
 *
 * Retrieves an opaque identifier for this monitor.  The identifier can usually
 * be relied upon to uniquely identify this monitor (even if you have multiple
 * identical monitors of the same make and model), assuming the monitor's
 * hardware is set up properly.
 *
 * This identifier should also be stable across application and machine
 * restarts.
 *
 * If the monitor's hardware is not set up properly, the identifier may not be
 * unique.  Unfortunately, this library cannot determine when this is the case.
 *
 * Return value: (not nullable) (transfer none): A string owned by @monitor.
 *
 * Since: 4.19.4
 **/
const char *
xfw_monitor_get_identifier(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->identifier;
}

/**
 * xfw_monitor_get_description:
 * @monitor: a #XfwMonitor.
 *
 * Returns a human-readable description of this monitor, suitable for
 * displaying in a user interface.
 *
 * Return value: (not nullable) (transfer none): A string owned by @monitor.
 *
 * Since: 4.19.4
 **/
const char *
xfw_monitor_get_description(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->description;
}

/**
 * xfw_monitor_get_connector:
 * @monitor: a #XfwMonitor.
 *
 * Returns the name of the physical connector this monitor is connected to.
 *
 * This might be a string such as "eDP-1", "DP-3", or "HDMI-2".  Note that in
 * environments where the monitor is "virtual", a synthetic connector name may
 * be returned.
 *
 * Return value: (not nullable) (transfer none): A string owned by @monitor.
 *
 * Since: 4.19.4
 **/
const char *
xfw_monitor_get_connector(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->connector;
}

/**
 * xfw_monitor_get_make:
 * @monitor: a #XfwMonitor.
 *
 * Returns the monitor's manufacturer's name, if available.
 *
 * Return value: (nullable) (transfer none): A string owned by @monitor, or
 * %NULL.
 *
 * Since: 4.19.4
 **/
const char *
xfw_monitor_get_make(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->make;
}

/**
 * xfw_monitor_get_model:
 * @monitor: a #XfwMonitor.
 *
 * Returns the monitor's product model name, if available.
 *
 * Return value: (nullable) (transfer none): A string owned by @monitor, or
 * %NULL.
 *
 * Since: 4.19.4
 **/
const char *
xfw_monitor_get_model(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->model;
}

/**
 * xfw_monitor_get_serial:
 * @monitor: a #XfwMonitor.
 *
 * Returns the monitor's serial number, if available.  Note that some
 * manufacturers do not program their monitor's hardware with unique serial
 * numbers.
 *
 * Return value: (nullable) (transfer none): A string owned by @monitor, or
 * %NULL.
 *
 * Since: 4.19.4
 **/
const char *
xfw_monitor_get_serial(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->serial;
}

/**
 * xfw_monitor_get_refresh:
 * @monitor: a #XfwMonitor.
 *
 * Returns the monitor's current refresh rate, in millihertz.
 *
 * Return value: A non-negative integer in mHz.
 *
 * Since: 4.19.4
 **/
guint
xfw_monitor_get_refresh(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), 0);
    return XFW_MONITOR_GET_PRIVATE(monitor)->refresh;
}

/**
 * xfw_monitor_get_scale:
 * @monitor: a #XfwMonitor.
 *
 * Returns the monitor's scaling factor.
 *
 * Return value: A positive integer scale.
 *
 * Since: 4.19.4
 **/
guint
xfw_monitor_get_scale(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), 1);
    return XFW_MONITOR_GET_PRIVATE(monitor)->scale;
}

/**
 * xfw_monitor_get_physical_geometry:
 * @monitor: a #XfwMonitor.
 * @physical_geometry: (not nullable) (out caller-allocates): a #GdkRectangle.
 *
 * Retrieves the position and size of the monitor in physical device pixels.
 *
 * Since: 4.19.4
 **/
void
xfw_monitor_get_physical_geometry(XfwMonitor *monitor, GdkRectangle *physical_geometry) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(physical_geometry != NULL);
    *physical_geometry = XFW_MONITOR_GET_PRIVATE(monitor)->physical_geometry;
}

/**
 * xfw_monitor_get_logical_geometry:
 * @monitor: a #XfwMonitor.
 * @logical_geometry: (not nullable) (out caller-allocates): a #GdkRectangle.
 *
 * Retrieves the position and size of the monitor in logical application
 * pixels, which are affected by the monitor's scale factor.
 *
 * Since: 4.19.4
 **/
void
xfw_monitor_get_logical_geometry(XfwMonitor *monitor, GdkRectangle *logical_geometry) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(logical_geometry != NULL);
    *logical_geometry = XFW_MONITOR_GET_PRIVATE(monitor)->logical_geometry;
}

/**
 * xfw_monitor_get_physical_size:
 * @monitor: a #XfwMonitor.
 * @width_mm: (nullable) (out caller-allocates): an unsigned integer.
 * @height_mm: (nullable) (out caller-allocates): an unsigned integer.
 *
 * Retrieves the physical width and height of the monitor in millimeters.
 *
 * Since: 4.19.4
 **/
void
xfw_monitor_get_physical_size(XfwMonitor *monitor, guint *width_mm, guint *height_mm) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (width_mm != NULL) {
        *width_mm = priv->width_mm;
    }
    if (height_mm != NULL) {
        *height_mm = priv->height_mm;
    }
}

/**
 * xfw_monitor_get_subpixel:
 * @monitor: a #XfwMonitor.
 *
 * Returns the subpixel ordering of @monitor.
 *
 * Return value: A value from the #XfwMonitorSubpixel enum.
 *
 * Since: 4.19.4
 **/
XfwMonitorSubpixel
xfw_monitor_get_subpixel(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), XFW_MONITOR_SUBPIXEL_UNKNOWN);
    return XFW_MONITOR_GET_PRIVATE(monitor)->subpixel;
}

/**
 * xfw_monitor_get_transform:
 * @monitor: a #XfwMonitor.
 *
 * Returns the rotation and reflection transform set on @monitor.
 *
 * Return value: A value from the #XfwMonitorTransform enum.
 *
 * Since: 4.19.4
 **/
XfwMonitorTransform
xfw_monitor_get_transform(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), XFW_MONITOR_TRANSFORM_NORMAL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->transform;
}


void
_xfw_monitor_set_identifier(XfwMonitor *monitor, const char *identifier) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(identifier != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (g_strcmp0(identifier, priv->identifier) != 0) {
        g_free(priv->identifier);
        priv->identifier = g_strdup(identifier);
        priv->pending_changes |= MONITOR_PENDING_IDENTIFIER;
    }
}

void
_xfw_monitor_set_description(XfwMonitor *monitor, const char *description) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(description != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (g_strcmp0(description, priv->description) != 0) {
        g_free(priv->description);
        priv->description = g_strdup(description);
        priv->pending_changes |= MONITOR_PENDING_DESCRIPTION;
    }
}

void
_xfw_monitor_set_connector(XfwMonitor *monitor, const char *connector) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(connector != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (g_strcmp0(connector, priv->connector) != 0) {
        g_free(priv->connector);
        priv->connector = g_strdup(connector);
        priv->pending_changes |= MONITOR_PENDING_CONNECTOR;
    }
}

void
_xfw_monitor_set_make(XfwMonitor *monitor, const char *make) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(make != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (g_strcmp0(make, priv->make) != 0) {
        g_free(priv->make);
        priv->make = g_strdup(make);
        priv->pending_changes |= MONITOR_PENDING_MAKE;
    }
}

void
_xfw_monitor_set_model(XfwMonitor *monitor, const char *model) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(model != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (g_strcmp0(model, priv->model) != 0) {
        g_free(priv->model);
        priv->model = g_strdup(model);
        priv->pending_changes |= MONITOR_PENDING_MODEL;
    }
}

void
_xfw_monitor_set_serial(XfwMonitor *monitor, const char *serial) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(serial != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (g_strcmp0(serial, priv->serial) != 0) {
        g_free(priv->serial);
        priv->serial = g_strdup(serial);
        priv->pending_changes |= MONITOR_PENDING_SERIAL;
    }
}

void
_xfw_monitor_set_refresh(XfwMonitor *monitor, guint refresh_millihertz) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (priv->refresh != refresh_millihertz) {
        priv->refresh = refresh_millihertz;
        priv->pending_changes |= MONITOR_PENDING_REFRESH;
    }
}

void
_xfw_monitor_set_scale(XfwMonitor *monitor, guint scale) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (priv->scale != scale) {
        priv->scale = scale;
        priv->pending_changes |= MONITOR_PENDING_SCALE;
    }
}

void
_xfw_monitor_set_physical_geometry(XfwMonitor *monitor, GdkRectangle *physical_geometry) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(physical_geometry != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (!gdk_rectangle_equal(&priv->physical_geometry, physical_geometry)) {
        priv->physical_geometry = *physical_geometry;
        priv->pending_changes |= MONITOR_PENDING_PHYSICAL_GEOMETRY;
    }
}

void
_xfw_monitor_set_logical_geometry(XfwMonitor *monitor, GdkRectangle *logical_geometry) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(logical_geometry != NULL);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (!gdk_rectangle_equal(&priv->logical_geometry, logical_geometry)) {
        priv->logical_geometry = *logical_geometry;
        priv->pending_changes |= MONITOR_PENDING_LOGICAL_GEOMETRY;
    }
}

void
_xfw_monitor_set_physical_size(XfwMonitor *monitor, guint width_mm, guint height_mm) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (priv->width_mm != width_mm) {
        priv->width_mm = width_mm;
        priv->pending_changes |= MONITOR_PENDING_PHYSICAL_WIDTH;
    }
    if (priv->width_mm != width_mm) {
        priv->height_mm = height_mm;
        priv->pending_changes |= MONITOR_PENDING_PHYSICAL_HEIGHT;
    }
}

void
_xfw_monitor_set_subpixel(XfwMonitor *monitor, XfwMonitorSubpixel subpixel) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(subpixel >= XFW_MONITOR_SUBPIXEL_UNKNOWN && subpixel <= XFW_MONITOR_SUBPIXEL_VBGR);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (priv->subpixel != subpixel) {
        priv->subpixel = subpixel;
        priv->pending_changes |= MONITOR_PENDING_SUBPIXEL;
    }
}

void
_xfw_monitor_set_transform(XfwMonitor *monitor, XfwMonitorTransform transform) {
    g_return_if_fail(XFW_IS_MONITOR(monitor));
    g_return_if_fail(transform >= XFW_MONITOR_TRANSFORM_NORMAL && transform <= XFW_MONITOR_TRANSFORM_FLIPPED_270);

    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);
    if (priv->transform != transform) {
        priv->transform = transform;
        priv->pending_changes |= MONITOR_PENDING_TRANSFORM;
    }
}

MonitorPendingChanges
_xfw_monitor_notify_pending_changes(XfwMonitor *monitor) {
    static const struct {
        MonitorPendingChanges bit;
        const gchar *property;
    } change_map[] = {
        { MONITOR_PENDING_IDENTIFIER, "identifier" },
        { MONITOR_PENDING_CONNECTOR, "connector" },
        { MONITOR_PENDING_DESCRIPTION, "description" },
        { MONITOR_PENDING_MAKE, "make" },
        { MONITOR_PENDING_MODEL, "model" },
        { MONITOR_PENDING_SERIAL, "serial" },
        { MONITOR_PENDING_REFRESH, "refresh" },
        { MONITOR_PENDING_SCALE, "scale" },
        { MONITOR_PENDING_PHYSICAL_GEOMETRY, "physical-geometry" },
        { MONITOR_PENDING_LOGICAL_GEOMETRY, "logical-geometry" },
        { MONITOR_PENDING_PHYSICAL_WIDTH, "width-mm" },
        { MONITOR_PENDING_PHYSICAL_HEIGHT, "height-mm" },
        { MONITOR_PENDING_SUBPIXEL, "subpixel" },
        { MONITOR_PENDING_TRANSFORM, "transform" },
    };
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(monitor);

    g_object_freeze_notify(G_OBJECT(monitor));

    for (gsize i = 0; i < G_N_ELEMENTS(change_map); ++i) {
        if ((priv->pending_changes & change_map[i].bit) != 0) {
            g_object_notify(G_OBJECT(monitor), change_map[i].property);
        }
    }

    MonitorPendingChanges old_pending_changes = priv->pending_changes;
    priv->pending_changes = 0;

    g_object_thaw_notify(G_OBJECT(monitor));

    return old_pending_changes;
}
