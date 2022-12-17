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

#ifndef __LIBXFCE4WINDOWING_PRIVATE_H__
#define __LIBXFCE4WINDOWING_PRIVATE_H__

#if !defined(__LIBXFCE4WINDOWING_H_INSIDE__) && !defined(LIBXFCE4WINDOWING_COMPILATION)
#error "Only libxfce4windowing.h can be included directly"
#endif

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gio/gdesktopappinfo.h>

// Support glib < 2.74

#ifndef G_DEFINE_ENUM_TYPE
#define G_DEFINE_ENUM_TYPE(TypeName, type_name, ...) \
GType \
type_name ## _get_type (void) { \
  static gsize g_define_type__static = 0; \
  if (g_once_init_enter (&g_define_type__static)) { \
    static const GEnumValue enum_values[] = { \
      __VA_ARGS__ , \
      { 0, NULL, NULL }, \
    }; \
    GType g_define_type = g_enum_register_static (g_intern_static_string (#TypeName), enum_values); \
    g_once_init_leave (&g_define_type__static, g_define_type); \
  } \
  return g_define_type__static; \
}
#endif

#ifndef G_DEFINE_FLAGS_TYPE
#define G_DEFINE_FLAGS_TYPE(TypeName, type_name, ...) \
GType \
type_name ## _get_type (void) { \
  static gsize g_define_type__static = 0; \
  if (g_once_init_enter (&g_define_type__static)) { \
    static const GFlagsValue flags_values[] = { \
      __VA_ARGS__ , \
      { 0, NULL, NULL }, \
    }; \
    GType g_define_type = g_flags_register_static (g_intern_static_string (#TypeName), flags_values); \
    g_once_init_leave (&g_define_type__static, g_define_type); \
  } \
  return g_define_type__static; \
}
#endif

#ifndef G_DEFINE_ENUM_VALUE
#define G_DEFINE_ENUM_VALUE(EnumValue, EnumNick) \
  { EnumValue, #EnumValue, EnumNick }
#endif

G_BEGIN_DECLS

typedef GdkPixbuf *(*XfwGetIconFunc)(GObject *wnck_object);

enum {
    SCREEN_PROP_SCREEN = 0x1000,
    SCREEN_PROP_WORKSPACE_MANAGER,
    SCREEN_PROP_ACTIVE_WINDOW,
    SCREEN_PROP_SHOW_DESKTOP,
};

enum {
    WORKSPACE_MANAGER_PROP_SCREEN = 0x2000,
};

enum {
    WORKSPACE_GROUP_PROP_SCREEN = 0x3000,
    WORKSPACE_GROUP_PROP_WORKSPACE_MANAGER,
    WORKSPACE_GROUP_PROP_CAPABILITIES,
    WORKSPACE_GROUP_PROP_WORKSPACES,
    WORKSPACE_GROUP_PROP_ACTIVE_WORKSPACE,
    WORKSPACE_GROUP_PROP_MONITORS,
};

enum {
    WORKSPACE_PROP_GROUP = 0x4000,
    WORKSPACE_PROP_ID,
    WORKSPACE_PROP_NAME,
    WORKSPACE_PROP_CAPABILITIES,
    WORKSPACE_PROP_STATE,
    WORKSPACE_PROP_NUMBER,
};

enum {
    WINDOW_PROP_SCREEN = 0x5000,
    WINDOW_PROP_ID,
    WINDOW_PROP_NAME,
    WINDOW_PROP_TYPE,
    WINDOW_PROP_STATE,
    WINDOW_PROP_CAPABILITIES,
    WINDOW_PROP_WORKSPACE,
    WINDOW_PROP_MONITORS,
    WINDOW_PROP_APPLICATION,
};

enum {
    APPLICATION_PROP_ID = 0x6000,
    APPLICATION_PROP_NAME,
    APPLICATION_PROP_WINDOWS,
    APPLICATION_PROP_INSTANCES,
};

void _libxfce4windowing_init(void);
GdkPixbuf *_xfw_wnck_object_get_icon(GObject *wnck_object, const gchar *icon_name, gint size, XfwGetIconFunc get_icon, XfwGetIconFunc get_mini_icon);
GDesktopAppInfo *_xfw_g_desktop_app_info_get(const gchar *app_id);

void _xfw_screen_install_properties(GObjectClass *gklass);
void _xfw_workspace_manager_install_properties(GObjectClass *gklass);
void _xfw_workspace_group_install_properties(GObjectClass *gklass);
void _xfw_workspace_install_properties(GObjectClass *gklass);
void _xfw_window_install_properties(GObjectClass *gklass);
void _xfw_application_install_properties(GObjectClass *gklass);
void _xfw_application_instance_free(gpointer data);

G_END_DECLS

#endif  /* __LIBXFCE4WINDOWING_PRIVATE_H__ */
