/*
 * Copyright (c) 2022 Brian Tarricone <brian@tarricone.org>
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

#include <limits.h>
#include <stdint.h>

#include "libxfce4windowing-private.h"
#include "xfw-marshal.h"
#include "xfw-screen.h"
#include "xfw-window.h"

typedef struct _XfwWindowIface XfwWindowInterface;
G_DEFINE_INTERFACE(XfwWindow, xfw_window, G_TYPE_OBJECT)

G_DEFINE_FLAGS_TYPE(XfwWindowState, xfw_window_state,
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_NONE, "none"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_ACTIVE, "active"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_MINIMIZED, "minimized"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_MAXIMIZED, "maximized"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_FULLSCREEN, "fullscreen"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_SKIP_PAGER, "skip-pager"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_SKIP_TASKLIST, "skip-tasklist"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_PINNED, "pinned"))

G_DEFINE_FLAGS_TYPE(XfwWindowCapabilities, xfw_window_capabilities,
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_NONE, "none"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE, "can-minimize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE, "can-unminimize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE, "can-maximize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE, "can-unmaximize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN, "can-unfullscreen"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN, "can-fullscreen"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_PIN, "can-pin"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNPIN, "can-unpin"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_SHADE, "can-shade"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNSHADE, "can-unshade"));

G_DEFINE_ENUM_TYPE(XfwWindowType, xfw_window_type,
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_NORMAL, "normal"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_DESKTOP, "desktop"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_DOCK, "dock"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_DIALOG, "dialog"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_TOOLBAR, "toolbar"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_MENU, "menu"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_UTILITY, "utility"),
  G_DEFINE_ENUM_VALUE(XFW_WINDOW_TYPE_SPLASHSCREEN, "splashscreen"))

static void
xfw_window_default_init(XfwWindowIface *iface) {
    g_signal_new("name-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, name_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);
    g_signal_new("icon-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, icon_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);
    g_signal_new("type-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, type_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__ENUM,
                 G_TYPE_NONE, 1,
                 XFW_TYPE_WINDOW_TYPE);
    g_signal_new("state-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, state_changed),
                 NULL, NULL,
                 xfw_marshal_VOID__FLAGS_FLAGS,
                 G_TYPE_NONE, 2,
                 XFW_TYPE_WINDOW_STATE,
                 XFW_TYPE_WINDOW_STATE);
    g_signal_new("capabilities-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, capabilities_changed),
                 NULL, NULL,
                 xfw_marshal_VOID__FLAGS_FLAGS,
                 G_TYPE_NONE, 2,
                 XFW_TYPE_WINDOW_CAPABILITIES,
                 XFW_TYPE_WINDOW_CAPABILITIES);
    g_signal_new("geometry-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, geometry_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);
    g_signal_new("workspace-changed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, state_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);
    g_signal_new("closed",
                 XFW_TYPE_WINDOW,
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(XfwWindowIface, closed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

    g_object_interface_install_property(iface,
                                        g_param_spec_object("screen",
                                                            "screen",
                                                            "screen",
                                                            XFW_TYPE_SCREEN,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_interface_install_property(iface,
                                        g_param_spec_uint64("id",
                                                            "id",
                                                            "id",
                                                            0, UINT64_MAX, 0,
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_string("name",
                                                            "name",
                                                            "name",
                                                            "",
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_object("icon",
                                                            "icon",
                                                            "icon",
                                                            GDK_TYPE_PIXBUF,
                                                            G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_enum("type",
                                                          "type",
                                                          "type",
                                                          XFW_TYPE_WINDOW_TYPE,
                                                          XFW_WINDOW_TYPE_NORMAL,
                                                          G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_flags("state",
                                                           "state",
                                                           "state",
                                                           XFW_TYPE_WINDOW_STATE,
                                                           XFW_WINDOW_STATE_NONE,
                                                           G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_flags("capabilities",
                                                           "capabilities",
                                                           "capabilities",
                                                           XFW_TYPE_WINDOW_CAPABILITIES,
                                                           XFW_WINDOW_CAPABILITIES_NONE,
                                                           G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_object("workspace",
                                                            "workspace",
                                                            "workspace",
                                                            XFW_TYPE_WORKSPACE,
                                                            G_PARAM_READABLE));
}

guint64
xfw_window_get_id(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), 0);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_id)(window);
}

const gchar *
xfw_window_get_name(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_name)(window);
}

GdkPixbuf *
xfw_window_get_icon(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_icon)(window);
}

XfwWindowType
xfw_window_get_window_type(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), XFW_WINDOW_TYPE_NORMAL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_window_type)(window);
}

XfwWindowState
xfw_window_get_state(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), XFW_WINDOW_STATE_NONE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_state)(window);
}

XfwWindowCapabilities
xfw_window_get_capabilities(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), XFW_WINDOW_CAPABILITIES_NONE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_capabilities)(window);
}

GdkRectangle *
xfw_window_get_geometry(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_geometry)(window);
}

XfwScreen *
xfw_window_get_screen(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_screen)(window);
}

XfwWorkspace *
xfw_window_get_workspace(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_workspace)(window);
}

void
xfw_window_activate(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowIface *iface;
    g_return_if_fail(XFW_IS_WINDOW(window));
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->activate)(window, event_timestamp, error);
}

void
xfw_window_close(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowIface *iface;
    g_return_if_fail(XFW_IS_WINDOW(window));
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->close)(window, event_timestamp, error);
}

#define STATE_SETTER(state) \
    void \
    xfw_window_set_ ## state(XfwWindow *window, gboolean is_ ## state, GError **error) { \
        XfwWindowIface *iface; \
        g_return_if_fail(XFW_IS_WINDOW(window)); \
        iface = XFW_WINDOW_GET_IFACE(window); \
        return (*iface->set_ ## state)(window, is_ ## state, error); \
    }

STATE_SETTER(minimized)
STATE_SETTER(maximized)
STATE_SETTER(fullscreen)
STATE_SETTER(skip_pager)
STATE_SETTER(skip_tasklist)
STATE_SETTER(pinned)
STATE_SETTER(shaded)

#undef STATE_SETTER

#define STATE_GETTER(state_lower, state_upper) \
    gboolean \
    xfw_window_is_ ## state_lower(XfwWindow *window) { \
        XfwWindowState state; \
        g_return_val_if_fail(XFW_IS_WINDOW(window), XFW_WINDOW_STATE_NONE); \
        state = xfw_window_get_state(window); \
        return (state & XFW_WINDOW_STATE_ ## state_upper) != 0; \
    }

STATE_GETTER(minimized, MINIMIZED)
STATE_GETTER(maximized, MAXIMIZED)
STATE_GETTER(fullscreen, FULLSCREEN)
STATE_GETTER(skip_pager, SKIP_PAGER)
STATE_GETTER(skip_tasklist, SKIP_TASKLIST)
STATE_GETTER(pinned, PINNED)
STATE_GETTER(shaded, SHADED)

#undef STATE_GETTER

void
_xfw_window_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, WINDOW_PROP_SCREEN, "screen");
    g_object_class_override_property(gklass, WINDOW_PROP_ID, "id");
    g_object_class_override_property(gklass, WINDOW_PROP_NAME, "name");
    g_object_class_override_property(gklass, WINDOW_PROP_ICON, "icon");
    g_object_class_override_property(gklass, WINDOW_PROP_TYPE, "type");
    g_object_class_override_property(gklass, WINDOW_PROP_STATE, "state");
    g_object_class_override_property(gklass, WINDOW_PROP_CAPABILITIES, "capabilities");
    g_object_class_override_property(gklass, WINDOW_PROP_WORKSPACE, "workspace");
}
