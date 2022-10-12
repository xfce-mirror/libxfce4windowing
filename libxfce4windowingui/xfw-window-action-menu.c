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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include "xfw-window-action-menu.h"

enum {
    PROP0,
    PROP_WINDOW,
};

struct _XfwWindowActionMenuPrivate {
    XfwWindow *window;
};

typedef struct {
    XfwWindow *window;
    XfwWorkspace *new_workspace;
} XfwWindowWorkspaceMoveData;

static void xfw_window_action_menu_constructed(GObject *obj);
static void xfw_window_action_menu_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfw_window_action_menu_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfw_window_action_menu_finalize(GObject *obj);

static void toggle_minimize_state(GtkWidget *item, XfwWindow *window);
static void toggle_maximize_state(GtkWidget *item, XfwWindow *window);
static void move_window(GtkWidget *item, XfwWindow *window);
static void resize_window(GtkWidget *item, XfwWindow *window);
static void toggle_above_state(GtkWidget *item, XfwWindow *window);
static void toggle_pinned_state(GtkWidget *item, XfwWindow *window);
static void move_window_workspace(GtkWidget *item, XfwWindowWorkspaceMoveData *data);
static void close_window(GtkWidget *item, XfwWindow *window);

static void free_move_data(gpointer data, GClosure *closure);

static const struct {
    gchar *label;
    XfwDirection direction;
} workspace_move_item_data[] = {
    { "Move to Workspace _Left", XFW_DIRECTION_LEFT },
    { "Move to Workspace R_ight", XFW_DIRECTION_RIGHT },
    { "Move to Workspace _Up", XFW_DIRECTION_UP },
    { "Move to Workspace _Down", XFW_DIRECTION_DOWN },
};

G_DEFINE_TYPE_WITH_PRIVATE(XfwWindowActionMenu, xfw_window_action_menu, GTK_TYPE_MENU)

static void
xfw_window_action_menu_class_init(XfwWindowActionMenuClass *klass) {
    GObjectClass *gklass = G_OBJECT_CLASS(klass);

    gklass->constructed = xfw_window_action_menu_constructed;
    gklass->set_property = xfw_window_action_menu_set_property;
    gklass->get_property = xfw_window_action_menu_get_property;
    gklass->finalize = xfw_window_action_menu_finalize;

    g_object_class_install_property(gklass,
                                    PROP_WINDOW,
                                    g_param_spec_object("window",
                                                        "window",
                                                        "window",
                                                        XFW_TYPE_WINDOW,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
xfw_window_action_menu_init(XfwWindowActionMenu *menu) {
    menu->priv = xfw_window_action_menu_get_instance_private(menu);
}

static void
xfw_window_action_menu_constructed(GObject *obj) {
    XfwWindowActionMenu *menu = XFW_WINDOW_ACTION_MENU(obj);
    XfwWindow *window = menu->priv->window;
    XfwWindowState state = xfw_window_get_state(window);
    XfwWindowCapabilities caps = xfw_window_get_capabilities(window);
    GtkWidget *item, *submenu;

    item = gtk_menu_item_new_with_mnemonic((state & XFW_WINDOW_STATE_MINIMIZED) == 0 ? _("Mi_nimize") : _("Unmi_nimize"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_minimize_state), window);
    gtk_widget_set_sensitive(item,
                             ((state & XFW_WINDOW_STATE_MINIMIZED) == 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE))
                             || ((state & XFW_WINDOW_STATE_MINIMIZED) != 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE)));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic((state & XFW_WINDOW_STATE_MAXIMIZED) == 0 ? _("Ma_ximize") : _("Unma_ximize"));
    gtk_widget_set_sensitive(item,
                             ((state & XFW_WINDOW_STATE_MAXIMIZED) == 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE))
                             || ((state & XFW_WINDOW_STATE_MAXIMIZED) != 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE)));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_maximize_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("_Move"));
    gtk_widget_set_sensitive(item, FALSE);  // TODO: add support for this operation
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(move_window), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_mnemonic(_("_Resize"));
    gtk_widget_set_sensitive(item, FALSE);  // TODO: add support for this operation
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(resize_window), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_check_menu_item_new_with_mnemonic(_("Always on _Top"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (state & XFW_WINDOW_STATE_ABOVE) != 0);
    gtk_widget_set_sensitive(item,
                             ((state & XFW_WINDOW_STATE_ABOVE) == 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_PLACE_ABOVE))
                             || ((state & XFW_WINDOW_STATE_ABOVE) != 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_ABOVE)));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_above_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_radio_menu_item_new_with_mnemonic(NULL, _("_Always on Visible Workspace"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (state & XFW_WINDOW_STATE_PINNED) != 0);
    gtk_widget_set_sensitive(item, (caps & (XFW_WINDOW_CAPABILITIES_CAN_PIN | XFW_WINDOW_CAPABILITIES_CAN_UNPIN)) != 0);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_pinned_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_radio_menu_item_new_with_mnemonic_from_widget(GTK_RADIO_MENU_ITEM(item), _("_Only on This Workspace"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (state & XFW_WINDOW_STATE_PINNED) == 0);
    gtk_widget_set_sensitive(item, (caps & (XFW_WINDOW_CAPABILITIES_CAN_PIN | XFW_WINDOW_CAPABILITIES_CAN_UNPIN)) != 0);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_pinned_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if ((caps & XFW_WINDOW_CAPABILITIES_CAN_MOVE) != 0) {
        XfwWorkspace *workspace = xfw_window_get_workspace(window);

        if (workspace != NULL) {
            XfwWorkspaceGroup *group;

            if ((state & XFW_WINDOW_STATE_PINNED) == 0) {
                for (size_t i = 0; i < sizeof(workspace_move_item_data) / sizeof(*workspace_move_item_data); ++i) {
                    XfwWorkspace *neighbor = xfw_workspace_get_neighbor(workspace, workspace_move_item_data[i].direction);
                    if (neighbor != NULL) {
                        XfwWindowWorkspaceMoveData *data = g_new0(XfwWindowWorkspaceMoveData, 1);
                        data->window = window;
                        data->new_workspace = g_object_ref(neighbor);

                        item = gtk_menu_item_new_with_mnemonic(_(workspace_move_item_data[i].label));
                        g_signal_connect_data(G_OBJECT(item), "activate",
                                              G_CALLBACK(move_window_workspace), data,
                                              free_move_data, G_CONNECT_DEFAULT);
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
                    }
                }
            }

            // TODO: maybe allow moving to any workspace in any group?
            // FIXME: also workspace can be NULL if the window is pinned
            group = xfw_workspace_get_workspace_group(workspace);

            item = gtk_menu_item_new_with_mnemonic(_("Move to Another _Workspace"));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

            submenu = gtk_menu_new();
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

            for (GList *l = xfw_workspace_group_list_workspaces(group);
                 l != NULL;
                 l = l->next)
            {
                XfwWorkspace *other_workspace = XFW_WORKSPACE(l->data);
                gchar *label, *free_label = NULL;
                XfwWindowWorkspaceMoveData *data = g_new0(XfwWindowWorkspaceMoveData, 1);

                label = (gchar *)xfw_workspace_get_name(other_workspace);
                if (label == NULL) {
                    label = g_strdup_printf(_("Workspace %d"), xfw_workspace_get_number(other_workspace));
                    free_label = label;
                }

                item = gtk_menu_item_new_with_label(label);
                g_free(free_label);
                if (other_workspace == workspace) {
                    gtk_widget_set_sensitive(item, FALSE);
                }
                g_signal_connect_data(G_OBJECT(item), "activate",
                                      G_CALLBACK(move_window_workspace), data,
                                      free_move_data, G_CONNECT_DEFAULT);
                gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
            }

            item = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }

    item = gtk_menu_item_new_with_mnemonic(_("_Close"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(close_window), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(GTK_WIDGET(menu));
    gtk_widget_hide(GTK_WIDGET(menu));
}

static void
xfw_window_action_menu_set_property(GObject *obj, guint prop_id, const GValue *value, GParamSpec *pspec) {
    XfwWindowActionMenu *menu = XFW_WINDOW_ACTION_MENU(obj);

    switch (prop_id) {
        case PROP_WINDOW:
            menu->priv->window = g_object_ref(g_value_get_object(value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void
xfw_window_action_menu_get_property(GObject *obj, guint prop_id, GValue *value, GParamSpec *pspec) {
    XfwWindowActionMenu *menu = XFW_WINDOW_ACTION_MENU(obj);

    switch (prop_id) {
        case PROP_WINDOW:
            g_value_set_object(value, menu->priv->window);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static void xfw_window_action_menu_finalize(GObject *obj) {
    XfwWindowActionMenu *menu = XFW_WINDOW_ACTION_MENU(obj);
    g_object_unref(menu->priv->window);
}

static void
toggle_minimize_state(GtkWidget *item, XfwWindow *window) {
    xfw_window_set_minimized(window, (xfw_window_get_state(window) & XFW_WINDOW_STATE_MINIMIZED) == 0, NULL);
}

static void
toggle_maximize_state(GtkWidget *item, XfwWindow *window) {
    xfw_window_set_maximized(window, (xfw_window_get_state(window) & XFW_WINDOW_STATE_MAXIMIZED) == 0, NULL);
}
static void
move_window(GtkWidget *item, XfwWindow *window) {
    xfw_window_start_move(window, NULL);
}

static void
resize_window(GtkWidget *item, XfwWindow *window) {
    xfw_window_start_resize(window, NULL);
}

static void
toggle_above_state(GtkWidget *item, XfwWindow *window) {
    xfw_window_set_above(window, (xfw_window_get_state(window) & XFW_WINDOW_STATE_ABOVE) == 0, NULL);
}

static void
toggle_pinned_state(GtkWidget *item, XfwWindow *window) {
    xfw_window_set_pinned(window, (xfw_window_get_state(window) & XFW_WINDOW_STATE_PINNED) == 0, NULL);
}

static void
move_window_workspace(GtkWidget *item, XfwWindowWorkspaceMoveData *data) {
    xfw_window_move_to_workspace(data->window, data->new_workspace, NULL);
}

static void
close_window(GtkWidget *item, XfwWindow *window) {
    xfw_window_close(window, gtk_get_current_event_time(), NULL);
}

static void
free_move_data(gpointer data, GClosure *closure) {
    g_free(data);
}


GtkWidget *
xfw_window_action_menu_new(XfwWindow *window) {
    return g_object_new(XFW_TYPE_WINDOW_ACTION_MENU,
                        "window", window,
                        NULL);
}
