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

#include <gdk/gdkwayland.h>
#include <libxfce4util/libxfce4util.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "libxfce4windowing-private.h"
#include "protocols/xdg-output-unstable-v1-client.h"
#include "xfw-monitor.h"
#include "xfw-monitor-private.h"
#include "xfw-monitor-wayland.h"
#include "xfw-screen.h"
#include "xfw-screen-wayland.h"

typedef struct {
    XfwScreenWayland *screen;
    GdkDisplay *display;

    GHashTable *outputs_to_monitors;
    GHashTable *xdg_outputs_to_monitors;

    struct wl_registry *registry;
    struct zxdg_output_manager_v1 *xdg_output_manager;

    GList *async_roundtrips;
} MonitorsData;

struct _XfwMonitorWayland {
    XfwMonitor parent;

    struct wl_output *output;
    struct zxdg_output_v1 *xdg_output;

    GdkRectangle physical_geometry;
    GdkRectangle logical_geometry;

    guint32 output_dones:4,
            xdg_output_done:1;
};

static void xfw_monitor_wayland_finalize(GObject *object);


G_DEFINE_TYPE(XfwMonitorWayland, xfw_monitor_wayland, XFW_TYPE_MONITOR)


static void
xfw_monitor_wayland_class_init(XfwMonitorWaylandClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = xfw_monitor_wayland_finalize;
}

static void
xfw_monitor_wayland_init(XfwMonitorWayland *monitor) {}

static void
xfw_monitor_wayland_finalize(GObject *object) {
    XfwMonitorWayland *monitor = XFW_MONITOR_WAYLAND(object);

    if (monitor->xdg_output != NULL) {
        zxdg_output_v1_destroy(monitor->xdg_output);
    }

    if (monitor->output != NULL) {
        if (wl_proxy_get_version((struct wl_proxy *)monitor->output) >= WL_OUTPUT_RELEASE_SINCE_VERSION) {
            wl_output_release(monitor->output);
        } else {
            wl_output_destroy(monitor->output);
        }
    }

    G_OBJECT_CLASS(xfw_monitor_wayland_parent_class)->finalize(object);
}

static XfwMonitorSubpixel
xfw_subpixel_from_wayland(enum wl_output_subpixel subpixel_wl) {
    switch (subpixel_wl) {
        case WL_OUTPUT_SUBPIXEL_NONE:
            return XFW_MONITOR_SUBPIXEL_NONE;
        case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
            return XFW_MONITOR_SUBPIXEL_HRGB;
        case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
            return XFW_MONITOR_SUBPIXEL_HBGR;
        case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
            return XFW_MONITOR_SUBPIXEL_VRGB;
        case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
            return XFW_MONITOR_SUBPIXEL_VBGR;
        default:
            return XFW_MONITOR_SUBPIXEL_UNKNOWN;
    }
}

static XfwMonitorTransform
xfw_transform_from_wayland(enum wl_output_transform transform_wl) {
    switch (transform_wl) {
        case WL_OUTPUT_TRANSFORM_90:
            return XFW_MONITOR_TRANSFORM_90;
        case WL_OUTPUT_TRANSFORM_180:
            return XFW_MONITOR_TRANSFORM_180;
        case WL_OUTPUT_TRANSFORM_270:
            return XFW_MONITOR_TRANSFORM_270;
        case WL_OUTPUT_TRANSFORM_FLIPPED:
            return XFW_MONITOR_TRANSFORM_FLIPPED;
        case WL_OUTPUT_TRANSFORM_FLIPPED_90:
            return XFW_MONITOR_TRANSFORM_FLIPPED_90;
        case WL_OUTPUT_TRANSFORM_FLIPPED_180:
            return XFW_MONITOR_TRANSFORM_FLIPPED_180;
        case WL_OUTPUT_TRANSFORM_FLIPPED_270:
            return XFW_MONITOR_TRANSFORM_FLIPPED_270;
        default:
            return XFW_MONITOR_TRANSFORM_NORMAL;
    }
}

static void
callback_done(void *data, struct wl_callback *callback, uint32_t callback_id) {
    MonitorsData *msdata = data;
    msdata->async_roundtrips = g_list_remove(msdata->async_roundtrips, callback);
    wl_callback_destroy(callback);
}

static const struct wl_callback_listener callback_listener = {
    .done = callback_done,
};

static void
finalize_output(MonitorsData *msdata, XfwMonitorWayland *monitor_wl) {
    XfwMonitor *monitor = XFW_MONITOR(monitor_wl);

    const char *make = xfw_monitor_get_make(monitor);
    const char *model = xfw_monitor_get_model(monitor);
    const char *serial = xfw_monitor_get_serial(monitor);
    const char *description = xfw_monitor_get_description(monitor);

    if (serial == NULL && make != NULL && model != NULL && description != NULL) {
        // On the DRM backend, wlroots formats the description like so:
        //     $MAKE $MODEL $SERIAL ($CONNECTOR via $SUBCONNECTOR)
        // ... so we can try to extract the serial number if it's present.

        const char *connector = xfw_monitor_get_connector(monitor);
        size_t make_len = strlen(make);
        size_t model_len = strlen(model);
        size_t description_len = strlen(description);

        if (description_len > make_len + model_len + 2) {
            const char *maybe_serial_start = description + make_len + model_len + 2;

            gchar *connector_start_str = g_strconcat(" (", connector, NULL);
            const char *open_paren = strstr(maybe_serial_start, connector_start_str);
            g_free(connector_start_str);

            if (open_paren != NULL && open_paren > maybe_serial_start) {
                gchar *found_serial = g_strndup(maybe_serial_start, open_paren - maybe_serial_start);
                _xfw_monitor_set_serial(monitor, found_serial);
                g_free(found_serial);
                serial = xfw_monitor_get_serial(monitor);
            }
        }
    }

    GChecksum *identifier_cksum = g_checksum_new(G_CHECKSUM_SHA1);
    if ((make != NULL && model != NULL) || serial != NULL) {
        if (make != NULL) {
            g_checksum_update(identifier_cksum, (guchar *)make, strlen(make));
        }
        if (model != NULL) {
            g_checksum_update(identifier_cksum, (guchar *)model, strlen(model));
        }
        if (serial != NULL) {
            g_checksum_update(identifier_cksum, (guchar *)serial, strlen(serial));
        }
    } else {
        const char *connector = xfw_monitor_get_connector(monitor);
        g_checksum_update(identifier_cksum, (guchar *)connector, strlen(connector));
    }
    _xfw_monitor_set_identifier(monitor, g_checksum_get_string(identifier_cksum));
    g_checksum_free(identifier_cksum);

    // TODO: attempt to figure out physical x/y
    _xfw_monitor_set_physical_geometry(monitor, &monitor_wl->physical_geometry);
    _xfw_monitor_set_logical_geometry(monitor, &monitor_wl->logical_geometry);

    GList *monitors = _xfw_screen_wayland_steal_monitors(msdata->screen);
    if (!g_list_find(monitors, monitor)) {
        monitors = g_list_append(monitors, monitor);
    }
    _xfw_screen_wayland_set_monitors(msdata->screen, monitors);
}

static void
output_name(void *data, struct wl_output *output, const char *name) {
    DBG("output name for ID %d", wl_proxy_get_id((struct wl_proxy *)output));
    MonitorsData *msdata = data;
    XfwMonitor *monitor = g_hash_table_lookup(msdata->outputs_to_monitors, output);
    _xfw_monitor_set_connector(monitor, name);
}

static void
output_description(void *data, struct wl_output *output, const char *description) {
    DBG("output desc for ID %d", wl_proxy_get_id((struct wl_proxy *)output));
    MonitorsData *msdata = data;
    XfwMonitor *monitor = g_hash_table_lookup(msdata->outputs_to_monitors, output);
    _xfw_monitor_set_description(monitor, description);
}

static void
output_mode(void *data, struct wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    DBG("output mode for ID %d", wl_proxy_get_id((struct wl_proxy *)output));
    if ((flags & WL_OUTPUT_MODE_CURRENT) != 0) {
        MonitorsData *msdata = data;
        XfwMonitorWayland *monitor = g_hash_table_lookup(msdata->outputs_to_monitors, output);
        monitor->physical_geometry.width = width;
        monitor->physical_geometry.height = height;
        _xfw_monitor_set_refresh(XFW_MONITOR(monitor), refresh);
    }
}

static void
output_geometry(void *data,
                struct wl_output *output,
                int32_t x,
                int32_t y,
                int32_t physical_width,
                int32_t physical_height,
                int32_t subpixel,
                const char *make,
                const char *model,
                int32_t transform)
{
    DBG("output geom for ID %d", wl_proxy_get_id((struct wl_proxy *)output));
    MonitorsData *msdata = data;
    XfwMonitor *monitor = g_hash_table_lookup(msdata->outputs_to_monitors, output);

    XFW_MONITOR_WAYLAND(monitor)->physical_geometry.x = x;
    XFW_MONITOR_WAYLAND(monitor)->physical_geometry.y = y;

    _xfw_monitor_set_physical_size(monitor, physical_width, physical_height);
    _xfw_monitor_set_make(monitor, make);
    _xfw_monitor_set_model(monitor, model);
    _xfw_monitor_set_subpixel(monitor, xfw_subpixel_from_wayland(subpixel));
    _xfw_monitor_set_transform(monitor, xfw_transform_from_wayland(transform));
}

static void
output_scale(void *data, struct wl_output *output, int32_t scale) {
    DBG("output scale for ID %d", wl_proxy_get_id((struct wl_proxy *)output));
    MonitorsData *msdata = data;
    XfwMonitor *monitor = g_hash_table_lookup(msdata->outputs_to_monitors, output);
    _xfw_monitor_set_scale(monitor, scale);
}

static void
output_done(void *data, struct wl_output *output) {
    DBG("output done for ID %d", wl_proxy_get_id((struct wl_proxy *)output));
    MonitorsData *msdata = data;
    XfwMonitorWayland *monitor = g_hash_table_lookup(msdata->outputs_to_monitors, output);
    monitor->output_dones++;

    if (msdata->xdg_output_manager == NULL ||
        (wl_proxy_get_version((struct wl_proxy *)msdata->xdg_output_manager) >= 3 && monitor->output_dones >= 2) ||
        monitor->xdg_output_done)
    {
        DBG("finalizing output because: xdg_op_mgr=%p, xdg_op_mgr_vers=%d, xdg_op_done=%d",
            msdata->xdg_output_manager,
            msdata->xdg_output_manager != NULL ? (int)wl_proxy_get_version((struct wl_proxy *)msdata->xdg_output_manager) : -1,
            monitor->xdg_output_done);
        finalize_output(msdata, monitor);
    }
}

static const struct wl_output_listener output_listener = {
    .name = output_name,
    .description = output_description,
    .mode = output_mode,
    .geometry = output_geometry,
    .scale = output_scale,
    .done = output_done,
};

static void
xdg_output_name(void *data, struct zxdg_output_v1 *xdg_output, const char *name) {
    DBG("xdg output name for ID %d", wl_proxy_get_id((struct wl_proxy *)xdg_output));
    MonitorsData *msdata = data;
    XfwMonitor *monitor = g_hash_table_lookup(msdata->xdg_outputs_to_monitors, xdg_output);
    _xfw_monitor_set_connector(monitor, name);
}

static void
xdg_output_description(void *data, struct zxdg_output_v1 *xdg_output, const char *description) {
    DBG("xdg output desc for ID %d", wl_proxy_get_id((struct wl_proxy *)xdg_output));
    MonitorsData *msdata = data;
    XfwMonitor *monitor = g_hash_table_lookup(msdata->xdg_outputs_to_monitors, xdg_output);
    _xfw_monitor_set_description(monitor, description);
}

static void
xdg_output_logical_position(void *data, struct zxdg_output_v1 *xdg_output, int32_t x, int32_t y) {
    DBG("xdg output logpos for ID %d", wl_proxy_get_id((struct wl_proxy *)xdg_output));
    MonitorsData *msdata = data;
    XfwMonitorWayland *monitor = g_hash_table_lookup(msdata->xdg_outputs_to_monitors, xdg_output);
    monitor->logical_geometry.x = x;
    monitor->logical_geometry.y = y;
}

static void
xdg_output_logical_size(void *data, struct zxdg_output_v1 *xdg_output, int32_t width, int32_t height) {
    DBG("xdg output logsize for ID %d", wl_proxy_get_id((struct wl_proxy *)xdg_output));
    MonitorsData *msdata = data;
    XfwMonitorWayland *monitor = g_hash_table_lookup(msdata->xdg_outputs_to_monitors, xdg_output);
    monitor->logical_geometry.width = width;
    monitor->logical_geometry.height = height;
}

static void
xdg_output_done(void *data, struct zxdg_output_v1 *xdg_output) {
    DBG("xdg output done for ID %d", wl_proxy_get_id((struct wl_proxy *)xdg_output));
    MonitorsData *msdata = data;
    XfwMonitorWayland *monitor = g_hash_table_lookup(msdata->xdg_outputs_to_monitors, xdg_output);
    monitor->xdg_output_done = TRUE;

    if (monitor->output_dones > 0 && wl_proxy_get_version((struct wl_proxy *)msdata->xdg_output_manager) < 3) {
        finalize_output(msdata, monitor);
    }
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
    .name = xdg_output_name,
    .description = xdg_output_description,
    .logical_position = xdg_output_logical_position,
    .logical_size = xdg_output_logical_size,
    .done = xdg_output_done,
};

static void
init_xdg_output(MonitorsData *msdata, struct wl_output *output, XfwMonitorWayland *monitor) {
    struct zxdg_output_v1 *xdg_output = zxdg_output_manager_v1_get_xdg_output(msdata->xdg_output_manager, output);
    DBG("got xdg_output ID %d", wl_proxy_get_id((struct wl_proxy *)xdg_output));
    zxdg_output_v1_add_listener(xdg_output, &xdg_output_listener, msdata);
    g_hash_table_insert(msdata->xdg_outputs_to_monitors, xdg_output, g_object_ref(monitor));
}

static void
registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    MonitorsData *msdata = data;
    gboolean add_roundtrip = FALSE;

    if (g_strcmp0(interface, wl_output_interface.name) == 0) {
        XfwMonitorWayland *monitor = g_object_new(XFW_TYPE_MONITOR_WAYLAND, NULL);

        struct wl_output *output = wl_registry_bind(registry, name, &wl_output_interface, MIN(version, 4));
        DBG("got output ID %d", wl_proxy_get_id((struct wl_proxy *)output));
        wl_output_add_listener(output, &output_listener, msdata);
        g_hash_table_insert(msdata->outputs_to_monitors, output, g_object_ref(monitor));

        if (msdata->xdg_output_manager != NULL) {
            init_xdg_output(msdata, output, monitor);
        }

        add_roundtrip = TRUE;
    } else if (g_strcmp0(interface, zxdg_output_manager_v1_interface.name) == 0) {
        msdata->xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, MIN(version, 3));

        GHashTableIter iter;
        g_hash_table_iter_init(&iter, msdata->outputs_to_monitors);
        struct wl_output *output;
        XfwMonitorWayland *monitor;
        while (g_hash_table_iter_next(&iter, (gpointer)&output, (gpointer)&monitor)) {
            init_xdg_output(msdata, output, monitor);
        }

        add_roundtrip = TRUE;
    }

    if (add_roundtrip) {
        struct wl_callback *callback = wl_display_sync(gdk_wayland_display_get_wl_display(msdata->display));
        wl_callback_add_listener(callback, &callback_listener, msdata);
        msdata->async_roundtrips = g_list_prepend(msdata->async_roundtrips, callback);
    }
}

static void
registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    MonitorsData *msdata = data;

    GHashTableIter iter;
    g_hash_table_iter_init(&iter, msdata->outputs_to_monitors);

    struct wl_output *output;
    XfwMonitorWayland *monitor;
    while (g_hash_table_iter_next(&iter, (gpointer)&output, (gpointer)&monitor)) {
        if (wl_proxy_get_id((struct wl_proxy *)output) == name) {
            if (monitor->xdg_output != NULL) {
                g_hash_table_remove(msdata->xdg_outputs_to_monitors, monitor->xdg_output);
            }
            g_hash_table_remove(msdata->outputs_to_monitors, output);
            g_object_unref(monitor);

            GList *monitors = _xfw_screen_wayland_steal_monitors(msdata->screen);
            monitors = g_list_remove(monitors, monitor);
            _xfw_screen_wayland_set_monitors(msdata->screen, monitors);

            break;
        }
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static void
screen_destroyed(gpointer data, GObject *where_the_object_was) {
    MonitorsData *msdata = data;

    g_list_free_full(msdata->async_roundtrips, (GDestroyNotify)wl_callback_destroy);

    g_hash_table_destroy(msdata->outputs_to_monitors);
    g_hash_table_destroy(msdata->xdg_outputs_to_monitors);
    
    if (msdata->xdg_output_manager != NULL) {
        zxdg_output_manager_v1_destroy(msdata->xdg_output_manager);
    }

    if (msdata->registry != NULL) {
        wl_registry_destroy(msdata->registry);
    }

    g_free(msdata);
}

void
_xfw_monitor_wayland_init(XfwScreenWayland *screen) {
    GdkScreen *gscreen = NULL;
    g_object_get(screen,
                 "screen", &gscreen,
                 NULL);

    GdkDisplay *display = gdk_screen_get_display(gscreen);
    struct wl_display *wldpy = gdk_wayland_display_get_wl_display(display);
    struct wl_registry *registry = wl_display_get_registry(wldpy);

    MonitorsData *msdata = g_new0(MonitorsData, 1);
    msdata->screen = screen;
    msdata->display = display;
    msdata->outputs_to_monitors = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    msdata->xdg_outputs_to_monitors = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    msdata->registry = registry;

    g_object_weak_ref(G_OBJECT(screen), screen_destroyed, msdata);

    wl_registry_add_listener(registry, &registry_listener, msdata);
    wl_display_roundtrip(wldpy);

    while (msdata->async_roundtrips != NULL) {
        wl_display_dispatch(wldpy);
    }
}
