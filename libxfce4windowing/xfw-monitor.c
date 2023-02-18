/*
 * Copyright (c) 2023 Brian Tarricone <brian@tarricone.org>
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
 * @short_description: An object representing a connected monitor
 * @stability: Unstable
 * @include: libxfce4windowing/libxfce4windowing.h
 *
 * #XfwMonitor represents a monitor connected to an #XfwScreen.
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <string.h>

#include <glib/gi18n-lib.h>

#include "xfw-monitor-private.h"
#include "xfw-screen.h"

#define XFW_MONITOR_GET_PRIVATE(monitor) ((XfwMonitorPrivate *)xfw_monitor_get_instance_private(XFW_MONITOR(monitor)))

typedef struct _XfwMonitorPrivate {
    XfwScreen *screen;
    GdkMonitor *gdk_monitor;

    guint number;
    gchar *friendly_name;
} XfwMonitorPrivate;

enum {
    PROP0,
    PROP_SCREEN,
    PROP_MONITOR,
    PROP_NUMBER,
};

static void xfw_monitor_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void xfw_monitor_get_property(GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec);

static GHashTable *load_pnp_ids(void);


G_DEFINE_TYPE_WITH_PRIVATE(XfwMonitor, xfw_monitor, G_TYPE_OBJECT)


static const gchar *unneeded_suffixes[] = {
    ", Inc.",
    ", INC.",
    ", Inc",
    " Inc.",
    " Inc",
    ", Pty Ltd",
    " Pty Ltd",
    " Pte Ltd",
    ", Ltd.",
    ", Ltd",
    ",Ltd",
    ",LTD",
    ", LTD.",
    " Ltd",
    ", N.V.",
    " L.P.",
    " GmbH",
    " LLC",
    " S.p.A.",
    " S.A.",
};

static GHashTable *pnp_ids = NULL;

static void
xfw_monitor_class_init(XfwMonitorClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = xfw_monitor_set_property;
    gobject_class->get_property = xfw_monitor_get_property;

    g_object_class_install_property(gobject_class,
                                    PROP_SCREEN,
                                    g_param_spec_object("screen",
                                                        "screen",
                                                        "screen",
                                                        XFW_TYPE_SCREEN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class,
                                    PROP_MONITOR,
                                    g_param_spec_object("monitor",
                                                        "monitor",
                                                        "monitor",
                                                        GDK_TYPE_MONITOR,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class,
                                    PROP_NUMBER,
                                    g_param_spec_uint("number",
                                                      "number",
                                                      "number",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    if (G_LIKELY(pnp_ids == NULL)) {
        pnp_ids = load_pnp_ids();
    }
}

static void
xfw_monitor_init(XfwMonitor *monitor) {}

static void
xfw_monitor_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(object);

    switch (prop_id) {
        case PROP_SCREEN:
            priv->screen = g_value_get_object(value);
            break;

        case PROP_MONITOR:
            priv->gdk_monitor = g_value_get_object(value);
            break;

        case PROP_NUMBER:
            priv->number = g_value_get_uint(value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}


static void
xfw_monitor_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwMonitorPrivate *priv = XFW_MONITOR_GET_PRIVATE(object);

    switch (prop_id) {
        case PROP_SCREEN:
            g_value_set_object(value, priv->screen);
            break;

        case PROP_MONITOR:
            g_value_set_object(value, priv->gdk_monitor);
            break;

        case PROP_NUMBER:
            g_value_set_uint(value, priv->number);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static inline gboolean
remove_suffix(gchar *str, const gchar *chop_str) {
    if (g_str_has_suffix(str, chop_str)) {
        gint str_len = strlen(str);
        gint chop_len = strlen(chop_str);

        if (str_len > chop_len) {
            str[str_len - chop_len] = '\0';
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

static gchar *
remove_unneeded_suffixes(gchar *str) {
    for (gsize j = 0; j < G_N_ELEMENTS(unneeded_suffixes) ; ++j) {
        if (remove_suffix(str, unneeded_suffixes[j])) {
            break;
        }
    }

    return str;
}

static GHashTable *
load_pnp_ids(void) {
    GHashTable *ids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    gchar *contents = NULL;
    GError *error = NULL;

    if (g_file_get_contents(PNP_IDS_PATH, &contents, NULL, &error)) {
        gchar **lines = g_strsplit(contents, "\n", -1);
        g_free(contents);

        for (gsize i = 0; lines[i] != NULL; ++i) {
            gchar *tab = strchr(lines[i], '\t');
            if (tab != NULL && tab[1] != '\0') {
                *tab = '\0';
                g_hash_table_insert(ids, lines[i], remove_unneeded_suffixes(tab + 1));
            }
        }

        // Don't g_strfreev() here, as the hash table owns the array elements now
        g_free(lines);
    } else if (error != NULL) {
        g_message("%s", error->message);
        g_error_free(error);
    }

    return ids;
}

static const gchar *
xfw_monitor_get_vendor_name(const gchar *vendor_code) {
    return g_hash_table_lookup(pnp_ids, vendor_code);
}

/**
 * xfw_monitor_get_gdk_monitor:
 * @monitor: an #XfwMonitor.
 *
 * Retrieves the underlying #GdkMonitor for @monitor.
 *
 * Return value: (not nullable) (transfer none): A #GdkMonitor,
 * unowned.
 *
 * Since: 4.19.2
 **/
GdkMonitor *
xfw_monitor_get_gdk_monitor(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);
    return XFW_MONITOR_GET_PRIVATE(monitor)->gdk_monitor;
}

/**
 * xfw_monitor_get_number:
 * @monitor: an #XfwMonitor.
 *
 * Retrieves the ordinal number for this monitor.  Note that this number
 * can change if monitors are detected in a different order.
 *
 * Return value: A 0-indexed integer.
 *
 * Since: 4.19.2
 **/
guint
xfw_monitor_get_number(XfwMonitor *monitor) {
    g_return_val_if_fail(XFW_IS_MONITOR(monitor), 0);
    return XFW_MONITOR_GET_PRIVATE(monitor)->number;
}

/**
 * xfw_monitor_get_friendly_name:
 * @monitor: an #XfwMonitor.
 *
 * Creates a human-displayable name for the monitor, if possible.
 *
 * Return value: (not nullable) (transfer none): A NUL-terminated string,
 * owned by @monitor.
 *
 * Since: 4.19.2
 **/
const gchar *
xfw_monitor_get_friendly_name(XfwMonitor *monitor) {
    XfwMonitorPrivate *priv;

    g_return_val_if_fail(XFW_IS_MONITOR(monitor), NULL);

    priv = XFW_MONITOR_GET_PRIVATE(monitor);

    if (priv->friendly_name == NULL) {
        const gchar *model = gdk_monitor_get_model(priv->gdk_monitor);

        if (model != NULL
            && (g_str_has_prefix(model, "LVDS")
                || g_str_has_prefix(model, "eDP")
                || g_strcmp0(model, "PANEL") == 0))
        {
            priv->friendly_name = g_strdup(_("Laptop Screen"));
        } else {
            const gchar *manufacturer_code = gdk_monitor_get_manufacturer(priv->gdk_monitor);
            const gchar *manufacturer_name = manufacturer_code != NULL ? xfw_monitor_get_vendor_name(manufacturer_code) : NULL;
            gint width_mm = gdk_monitor_get_width_mm(priv->gdk_monitor);
            gint height_mm = gdk_monitor_get_height_mm(priv->gdk_monitor);
            gchar *manufacturer = NULL;
            gint inches = -1;

            if (manufacturer_name != NULL) {
                manufacturer = g_strdup(manufacturer_name);
            } else if (manufacturer_code != NULL) {
                manufacturer = remove_unneeded_suffixes(g_strdup(manufacturer_code));
            } else {
                /* Translators: "Unknown" here is used to identify a monitor
                 * for which we don't know the manufacturer. When a
                 * manufacturer is known, the name of the manufacturer is
                 * used. */
                manufacturer = g_strdup(C_("Unknown monitor manufacturer name", "Unknown"));
            }

            if (width_mm > 0 && height_mm > 0) {
                gdouble d = sqrt(width_mm * width_mm + height_mm * height_mm);
                inches = (gint)(d / 25.4 + 0.5);
            }

            if (inches > 0) {
                priv->friendly_name = g_strdup_printf("%s %d\"", manufacturer, inches);
            } else {
                priv->friendly_name = g_strdup(manufacturer);
            }

            g_free(manufacturer);
        }
    }

    return priv->friendly_name;
}
