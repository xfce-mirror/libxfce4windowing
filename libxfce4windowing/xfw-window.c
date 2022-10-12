/*
 * Copyright (c) 2022 Brian Tarricone <brian@tarricone.org>
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
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_PINNED, "pinned"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_SHADED, "shaded"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_ABOVE, "above"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_BELOW, "below"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_STATE_URGENT, "urgent"))

G_DEFINE_FLAGS_TYPE(XfwWindowCapabilities, xfw_window_capabilities,
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_NONE, "none"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE, "can-minimize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE, "can-unminimize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE, "can-maximize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE, "can-unmaximize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_FULLSCREEN, "can-fullscreen"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNFULLSCREEN, "can-unfullscreen"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_SHADE, "can-shade"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNSHADE, "can-unshade"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_MOVE, "can-move"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_RESIZE, "can-resize"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_PLACE_ABOVE, "can-place-above"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_ABOVE, "can-unplace-above"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_PLACE_BELOW, "can-place-below"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_BELOW, "can-unplace-below"),
    G_DEFINE_ENUM_VALUE(XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE, "can-change-workspace"))

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
    g_object_interface_install_property(iface,
                                        g_param_spec_pointer("monitors",
                                                             "monitors",
                                                             "monitors",
                                                             G_PARAM_READABLE));
    g_object_interface_install_property(iface,
                                        g_param_spec_object("application",
                                                            "application",
                                                            "application",
                                                            XFW_TYPE_APPLICATION,
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
xfw_window_get_icon(XfwWindow *window, gint size) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_icon)(window, size);
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

GList *
xfw_window_get_monitors(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_monitors)(window);
}

XfwApplication *
xfw_window_get_application(XfwWindow *window) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), NULL);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->get_application)(window);
}

gboolean
xfw_window_activate(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->activate)(window, event_timestamp, error);
}

gboolean
xfw_window_close(XfwWindow *window, guint64 event_timestamp, GError **error) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->close)(window, event_timestamp, error);
}

gboolean
xfw_window_start_move(XfwWindow *window, GError **error){
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->start_move)(window, error);
}

gboolean
xfw_window_start_resize(XfwWindow *window, GError **error) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->start_resize)(window, error);
}

gboolean
xfw_window_set_geometry(XfwWindow *window, const GdkRectangle *rect, GError **error) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->set_geometry)(window, rect, error);
}

gboolean
xfw_window_set_button_geometry(XfwWindow *window, GdkWindow *relative_to, const GdkRectangle *rect, GError **error) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->set_button_geometry)(window, relative_to, rect, error);
}

gboolean
xfw_window_move_to_workspace(XfwWindow *window, XfwWorkspace *workspace, GError **error) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->move_to_workspace)(window, workspace, error);
}

#define STATE_SETTER(state) \
    gboolean \
    xfw_window_set_ ## state(XfwWindow *window, gboolean is_ ## state, GError **error) { \
        XfwWindowIface *iface; \
        g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE); \
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
STATE_SETTER(above)
STATE_SETTER(below)

#undef STATE_SETTER

#define STATE_GETTER(state_lower, state_upper) \
    gboolean \
    xfw_window_is_ ## state_lower(XfwWindow *window) { \
        XfwWindowState state; \
        g_return_val_if_fail(XFW_IS_WINDOW(window), XFW_WINDOW_STATE_NONE); \
        state = xfw_window_get_state(window); \
        return (state & XFW_WINDOW_STATE_ ## state_upper) != 0; \
    }

STATE_GETTER(active, ACTIVE)
STATE_GETTER(minimized, MINIMIZED)
STATE_GETTER(maximized, MAXIMIZED)
STATE_GETTER(fullscreen, FULLSCREEN)
STATE_GETTER(skip_pager, SKIP_PAGER)
STATE_GETTER(skip_tasklist, SKIP_TASKLIST)
STATE_GETTER(pinned, PINNED)
STATE_GETTER(shaded, SHADED)
STATE_GETTER(above, ABOVE)
STATE_GETTER(below, BELOW)
STATE_GETTER(urgent, URGENT)

#undef STATE_GETTER

gboolean
xfw_window_is_on_workspace(XfwWindow *window, XfwWorkspace *workspace) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->is_on_workspace)(window, workspace);
}

gboolean
xfw_window_is_in_viewport(XfwWindow *window, XfwWorkspace *workspace) {
    XfwWindowIface *iface;
    g_return_val_if_fail(XFW_IS_WINDOW(window), FALSE);
    iface = XFW_WINDOW_GET_IFACE(window);
    return (*iface->is_in_viewport)(window, workspace);
}

void
_xfw_window_install_properties(GObjectClass *gklass) {
    g_object_class_override_property(gklass, WINDOW_PROP_SCREEN, "screen");
    g_object_class_override_property(gklass, WINDOW_PROP_ID, "id");
    g_object_class_override_property(gklass, WINDOW_PROP_NAME, "name");
    g_object_class_override_property(gklass, WINDOW_PROP_TYPE, "type");
    g_object_class_override_property(gklass, WINDOW_PROP_STATE, "state");
    g_object_class_override_property(gklass, WINDOW_PROP_CAPABILITIES, "capabilities");
    g_object_class_override_property(gklass, WINDOW_PROP_WORKSPACE, "workspace");
    g_object_class_override_property(gklass, WINDOW_PROP_MONITORS, "monitors");
    g_object_class_override_property(gklass, WINDOW_PROP_APPLICATION, "application");
}
