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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#include <X11/extensions/render.h>
#include <gdk/gdkx.h>
#include <libdisplay-info/info.h>
#include <stdlib.h>

#include "libxfce4windowing-private.h"
#include "xfw-monitor-private.h"
#include "xfw-monitor-x11.h"
#include "xfw-monitor.h"
#include "xfw-screen-x11.h"
#include "xfw-screen.h"

struct _XfwMonitorX11 {
    XfwMonitor parent;
};

static GOnce xrandr_init_once = G_ONCE_INIT;
static int xrandr_event_base = 0;


G_DEFINE_TYPE(XfwMonitorX11, xfw_monitor_x11, XFW_TYPE_MONITOR)


static void
xfw_monitor_x11_class_init(XfwMonitorX11Class *klass) {}

static void
xfw_monitor_x11_init(XfwMonitorX11 *monitor) {}

static gpointer
xrandr_init(gpointer data) {
    GdkScreen *gscreen = GDK_SCREEN(data);
    GdkDisplay *display = gdk_screen_get_display(gscreen);

    Display *dpy = gdk_x11_display_get_xdisplay(display);
    int errbase;
    if (!XRRQueryExtension(dpy, &xrandr_event_base, &errbase)) {
        return "extension not found";
    }

    int major, minor;
    if (!XRRQueryVersion(dpy, &major, &minor)) {
        return "version query failed";
    }
    if (major != 1 || minor < 5) {
        return "version 1.5 or better required";
    }

    gdk_x11_register_standard_event_type(display, xrandr_event_base, RRNumberEvents);

    return NULL;
}

static inline XfwMonitorSubpixel
xfw_subpixel_from_x11(SubpixelOrder x11_subpixel) {
    switch (x11_subpixel) {
        case SubPixelNone:
            return XFW_MONITOR_SUBPIXEL_NONE;
        case SubPixelHorizontalRGB:
            return XFW_MONITOR_SUBPIXEL_HRGB;
        case SubPixelHorizontalBGR:
            return XFW_MONITOR_SUBPIXEL_HBGR;
        case SubPixelVerticalRGB:
            return XFW_MONITOR_SUBPIXEL_VRGB;
        case SubPixelVerticalBGR:
            return XFW_MONITOR_SUBPIXEL_VBGR;
        default:
            return XFW_MONITOR_SUBPIXEL_UNKNOWN;
    }
}

static inline XfwMonitorTransform
xfw_transform_from_x11(Rotation x11_rotation) {
    switch (x11_rotation) {
        case RR_Rotate_90:
            return XFW_MONITOR_TRANSFORM_90;
        case RR_Rotate_180:
            return XFW_MONITOR_TRANSFORM_180;
        case RR_Rotate_270:
            return XFW_MONITOR_TRANSFORM_270;
        case (RR_Rotate_0 | RR_Reflect_X):
            return XFW_MONITOR_TRANSFORM_FLIPPED;
        case (RR_Rotate_90 | RR_Reflect_X):
            return XFW_MONITOR_TRANSFORM_FLIPPED_90;
        case (RR_Rotate_180 | RR_Reflect_X):
            return XFW_MONITOR_TRANSFORM_FLIPPED_180;
        case (RR_Rotate_270 | RR_Reflect_X):
            return XFW_MONITOR_TRANSFORM_FLIPPED_270;
        case (RR_Rotate_0 | RR_Reflect_Y):
            return XFW_MONITOR_TRANSFORM_FLIPPED_180;
        case (RR_Rotate_90 | RR_Reflect_Y):
            return XFW_MONITOR_TRANSFORM_FLIPPED_270;
        case (RR_Rotate_180 | RR_Reflect_Y):
            return XFW_MONITOR_TRANSFORM_FLIPPED;
        case (RR_Rotate_270 | RR_Reflect_Y):
            return XFW_MONITOR_TRANSFORM_FLIPPED_90;
        default:
            return XFW_MONITOR_TRANSFORM_NORMAL;
    }
}

static XfwMonitor *
steal_monitor_by_connector(GList **monitors, const char *connector) {
    for (GList *l = *monitors; l != NULL; l = l->next) {
        XfwMonitor *monitor = XFW_MONITOR(l->data);
        if (g_strcmp0(connector, xfw_monitor_get_connector(monitor)) == 0) {
            *monitors = g_list_remove_link(*monitors, l);
            return monitor;
        }
    }
    return NULL;
}

static GList *
enumerate_monitors(XfwScreen *screen, GList **previous_monitors, XfwMonitor **primary_monitor) {
    GdkScreen *gscreen = _xfw_screen_get_gdk_screen(screen);
    GdkDisplay *display = gdk_screen_get_display(gscreen);
    gint scale = gdk_monitor_get_scale_factor(gdk_display_get_monitor(display, 0));
    Display *dpy = gdk_x11_display_get_xdisplay(display);
    Window root = XDefaultRootWindow(dpy);

    XRRScreenResources *resources = XRRGetScreenResourcesCurrent(dpy, root);
    if (resources == NULL) {
        g_warning("XRRGetScreenResourcesCurrent() failed");
        return NULL;
    }

    int nmonitors = 0;
    XRRMonitorInfo *rrmonitors = XRRGetMonitors(dpy, root, True, &nmonitors);
    if (rrmonitors == NULL || nmonitors == 0) {
        g_warning("XRRMonitorInfo returned nothing");
        XRRFreeScreenResources(resources);
        return NULL;
    }

    GList *monitors = NULL;
    *primary_monitor = NULL;

    for (int i = 0; i < nmonitors; ++i) {
        RROutput output = rrmonitors[i].outputs[0];

        gdk_x11_display_error_trap_push(display);
        XRROutputInfo *oinfo = XRRGetOutputInfo(dpy, resources, output);
        if (gdk_x11_display_error_trap_pop(display) != 0
            || oinfo == NULL
            || oinfo->connection == RR_Disconnected
            || oinfo->crtc == None)
        {
            if (oinfo != NULL) {
                XRRFreeOutputInfo(oinfo);
            }
            continue;
        }

        gdk_x11_display_error_trap_push(display);
        XRRCrtcInfo *crtc = XRRGetCrtcInfo(dpy, resources, oinfo->crtc);
        if (gdk_x11_display_error_trap_pop(display) != 0 || crtc == NULL) {
            XRRFreeOutputInfo(oinfo);
            continue;
        }

        gchar *connector = g_strndup(oinfo->name, oinfo->nameLen);
        XfwMonitor *monitor = steal_monitor_by_connector(previous_monitors, connector);
        if (monitor == NULL) {
            monitor = g_object_new(XFW_TYPE_MONITOR_X11, NULL);
            _xfw_monitor_set_connector(monitor, connector);
        }

        _xfw_monitor_set_scale(monitor, scale);
        _xfw_monitor_set_physical_size(monitor, oinfo->mm_width, oinfo->mm_height);
        _xfw_monitor_set_subpixel(monitor, xfw_subpixel_from_x11(oinfo->subpixel_order));
        _xfw_monitor_set_transform(monitor, xfw_transform_from_x11(crtc->rotation));

        for (gint j = 0; j < resources->nmode; ++j) {
            XRRModeInfo *mode = &resources->modes[j];
            if (mode->id == crtc->mode) {
                if (mode->hTotal > 0 && mode->vTotal > 0) {
                    _xfw_monitor_set_refresh(monitor, (mode->dotClock * 1000) / (mode->hTotal * mode->vTotal));
                    break;
                }
            }
        }

        GdkRectangle geometry = {
            .x = rrmonitors[i].x,
            .y = rrmonitors[i].y,
            .width = rrmonitors[i].width,
            .height = rrmonitors[i].height,
        };
        _xfw_monitor_set_physical_geometry(monitor, &geometry);

        geometry.x /= scale;
        geometry.y /= scale;
        geometry.width /= scale;
        geometry.height /= scale;
        _xfw_monitor_set_logical_geometry(monitor, &geometry);

        XRRFreeCrtcInfo(crtc);

        Atom edid_atom = XInternAtom(dpy, RR_PROPERTY_RANDR_EDID, False);

        gdk_x11_display_error_trap_push(display);

        Atom actual_type = None;
        int actual_format = 0;
        unsigned long nbytes = 0;
        unsigned long bytes_left = 0;
        unsigned char *edid_data = NULL;

        XRRGetOutputProperty(dpy,
                             output,
                             edid_atom,
                             0,
                             256,
                             False,
                             False,
                             AnyPropertyType,
                             &actual_type,
                             &actual_format,
                             &nbytes,
                             &bytes_left,
                             &edid_data);

        if (gdk_x11_display_error_trap_pop(display) == 0 && edid_data != NULL && nbytes > 0) {
            struct di_info *edid_info = di_info_parse_edid(edid_data, nbytes);
            if (edid_info != NULL) {
                char *make = di_info_get_make(edid_info);
                if (make != NULL) {
                    _xfw_monitor_set_make(monitor, make);
                }
                free(make);

                char *model = di_info_get_model(edid_info);
                if (model != NULL) {
                    _xfw_monitor_set_model(monitor, model);
                }
                free(model);

                char *serial = di_info_get_serial(edid_info);
                if (serial != NULL) {
                    _xfw_monitor_set_serial(monitor, serial);
                }
                free(serial);

                di_info_destroy(edid_info);
            }
        }

        unsigned char *identifier_data;
        guint idenfifier_data_len;
        if (edid_data != NULL && nbytes > 0) {
            identifier_data = edid_data;
            idenfifier_data_len = nbytes;
        } else {
            identifier_data = (unsigned char *)connector;
            idenfifier_data_len = strlen(connector);
        }
        GChecksum *identifier_cksum = g_checksum_new(G_CHECKSUM_SHA1);
        g_checksum_update(identifier_cksum, identifier_data, idenfifier_data_len);
        _xfw_monitor_set_identifier(monitor, g_checksum_get_string(identifier_cksum));
        g_checksum_free(identifier_cksum);

        if (edid_data != NULL) {
            XFree(edid_data);
        }

        const char *make = xfw_monitor_get_make(monitor);
        const char *model = xfw_monitor_get_model(monitor);
        const char *serial = xfw_monitor_get_serial(monitor);

        char *description;
        if (make != NULL && model != NULL && serial != NULL) {
            description = g_strdup_printf("%s %s %s (%s)", make, model, serial, connector);
        } else if (make != NULL && model != NULL) {
            description = g_strdup_printf("%s %s (%s)", make, model, connector);
        } else if (make != NULL) {
            description = g_strdup_printf("%s (%s)", make, connector);
        } else {
            description = g_strdup(connector);
        }
        _xfw_monitor_set_description(monitor, description);
        g_free(description);

        _xfw_monitor_set_is_primary(monitor, !!rrmonitors[i].primary);
        if (rrmonitors[i].primary) {
            *primary_monitor = monitor;
        }

        monitors = g_list_prepend(monitors, monitor);

        g_free(connector);
        XRRFreeOutputInfo(oinfo);
    }

    XRRFreeScreenResources(resources);
    XRRFreeMonitors(rrmonitors);

    return g_list_reverse(monitors);
}

static void
refresh_monitors(XfwScreen *screen) {
    GList *previous_monitors = _xfw_screen_steal_monitors(screen);
    guint n_previous = g_list_length(previous_monitors);
    XfwMonitor *primary_monitor = NULL;
    GList *monitors = enumerate_monitors(screen, &previous_monitors, &primary_monitor);

    guint n_removed = g_list_length(previous_monitors);
    guint n_kept = n_previous - n_removed;
    guint n_added = g_list_length(monitors) - n_kept;
    _xfw_screen_set_monitors(screen, monitors, primary_monitor, n_added, n_removed);
    g_list_free_full(previous_monitors, g_object_unref);
}

static GdkFilterReturn
rootwin_event_filter(GdkXEvent *gxevent, GdkEvent *event, gpointer data) {
    XEvent *xevent = (XEvent *)gxevent;

    if (xevent->type - xrandr_event_base == RRScreenChangeNotify
        || xevent->type - xrandr_event_base == RRNotify)
    {
        XfwScreen *screen = XFW_SCREEN(data);
        refresh_monitors(screen);
    }

    return GDK_FILTER_CONTINUE;
}

static void
screen_destroyed(gpointer data, GObject *where_the_object_was) {
    gdk_window_remove_filter(GDK_WINDOW(data), rootwin_event_filter, where_the_object_was);
}

void
_xfw_monitor_x11_init(XfwScreenX11 *xscreen) {
    XfwScreen *screen = XFW_SCREEN(xscreen);
    GdkScreen *gscreen = _xfw_screen_get_gdk_screen(screen);
    const gchar *error = g_once(&xrandr_init_once, xrandr_init, gscreen);
    if (error != NULL) {
        g_message("XRandR initialization error: %s", error);
        g_message("Will advertise only a single monitor");

        const gchar *connector = "X11-1";

        XfwMonitor *monitor = g_object_new(XFW_TYPE_MONITOR_X11, NULL);
        _xfw_monitor_set_connector(monitor, connector);
        _xfw_monitor_set_description(monitor, "X11 Monitor (X11-1)");
        _xfw_monitor_set_refresh(monitor, 60000);

        Screen *x11screen = gdk_x11_screen_get_xscreen(gscreen);
        GdkRectangle geom = {
            .x = 0,
            .y = 0,
            .width = WidthOfScreen(x11screen),
            .height = HeightOfScreen(x11screen),
        };
        _xfw_monitor_set_physical_geometry(monitor, &geom);

        gint scale = gdk_monitor_get_scale_factor(gdk_display_get_monitor(gdk_screen_get_display(gscreen), 0));
        _xfw_monitor_set_scale(monitor, scale);
        geom.width /= scale;
        geom.height /= scale;
        _xfw_monitor_set_logical_geometry(monitor, &geom);

        GChecksum *identifier_cksum = g_checksum_new(G_CHECKSUM_SHA1);
        g_checksum_update(identifier_cksum, (guchar *)connector, strlen(connector));
        _xfw_monitor_set_identifier(monitor, g_checksum_get_string(identifier_cksum));
        g_checksum_free(identifier_cksum);

        GList *monitors = g_list_append(NULL, monitor);
        _xfw_screen_set_monitors(screen, monitors, monitor, 1, 0);
    } else {
        Display *dpy = gdk_x11_display_get_xdisplay(gdk_screen_get_display(gscreen));
        GdkWindow *rootwin = gdk_screen_get_root_window(gscreen);
        Window xrootwin = gdk_x11_window_get_xid(rootwin);
        XRRSelectInput(dpy,
                       xrootwin,
                       RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask | RROutputPropertyNotifyMask);

        gdk_window_add_filter(rootwin, rootwin_event_filter, screen);
        g_object_weak_ref(G_OBJECT(screen), screen_destroyed, rootwin);

        refresh_monitors(screen);
    }
}
