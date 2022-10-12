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

    GtkWidget *min_item;
    GtkWidget *max_item;
    GtkWidget *move_item;
    GtkWidget *resize_item;
    GtkWidget *above_item;
    GtkWidget *pin_item;
    GtkWidget *unpin_item;
    GtkWidget *move_left_item;
    GtkWidget *move_right_item;
    GtkWidget *move_up_item;
    GtkWidget *move_down_item;
    GtkWidget *move_ws_item;
    GtkWidget *move_ws_submenu;
    GtkWidget *close_item;
};

typedef struct {
    XfwWindow *window;
    union {
        XfwDirection direction;
        XfwWorkspace *new_workspace;
    } to;
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

static void update_menu_items(XfwWindowActionMenu *menu);

static void window_state_changed(XfwWindow *window, XfwWindowState changed_mask, XfwWindowState new_state, XfwWindowActionMenu *menu);
static void window_capabilities_changed(XfwWindow *window, XfwWindowCapabilities changed_mask, XfwWindowCapabilities new_capabilities, XfwWindowActionMenu *menu);
static void window_workspace_changed(XfwWindow *window, XfwWindowActionMenu *menu);


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
    GtkWidget *item;
    XfwWindowWorkspaceMoveData *mdata;

    menu->priv->min_item = item = gtk_menu_item_new_with_mnemonic("");
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_minimize_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->max_item = item = gtk_menu_item_new_with_mnemonic("");
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_maximize_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_item = item = gtk_menu_item_new_with_mnemonic(_("_Move"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(move_window), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->resize_item = item = gtk_menu_item_new_with_mnemonic(_("_Resize"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(resize_window), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->above_item = item = gtk_check_menu_item_new_with_mnemonic(_("Always on _Top"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_above_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->pin_item = item = gtk_radio_menu_item_new_with_mnemonic(NULL, _("_Always on Visible Workspace"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_pinned_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->unpin_item = item = gtk_radio_menu_item_new_with_mnemonic_from_widget(GTK_RADIO_MENU_ITEM(item), _("_Only on This Workspace"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(toggle_pinned_state), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_left_item = item = gtk_menu_item_new_with_mnemonic(_("Move to Workspace _Left"));
    mdata = g_new0(XfwWindowWorkspaceMoveData, 1);
    mdata->window = menu->priv->window;
    mdata->to.direction = XFW_DIRECTION_LEFT;
    g_signal_connect_data(G_OBJECT(item), "activate",
                          G_CALLBACK(move_window_workspace), mdata,
                          free_move_data, G_CONNECT_DEFAULT);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_right_item = item = gtk_menu_item_new_with_mnemonic(_("Move to Workspace R_ight"));
    mdata = g_new0(XfwWindowWorkspaceMoveData, 1);
    mdata->window = menu->priv->window;
    mdata->to.direction = XFW_DIRECTION_RIGHT;
    g_signal_connect_data(G_OBJECT(item), "activate",
                          G_CALLBACK(move_window_workspace), mdata,
                          free_move_data, G_CONNECT_DEFAULT);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_up_item = item = gtk_menu_item_new_with_mnemonic(_("Move to Workspace _Up"));
    mdata = g_new0(XfwWindowWorkspaceMoveData, 1);
    mdata->window = menu->priv->window;
    mdata->to.direction = XFW_DIRECTION_UP;
    g_signal_connect_data(G_OBJECT(item), "activate",
                          G_CALLBACK(move_window_workspace), mdata,
                          free_move_data, G_CONNECT_DEFAULT);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_down_item = item = gtk_menu_item_new_with_mnemonic(_("Move to Workspace _Down"));
    mdata = g_new0(XfwWindowWorkspaceMoveData, 1);
    mdata->window = menu->priv->window;
    mdata->to.direction = XFW_DIRECTION_DOWN;
    g_signal_connect_data(G_OBJECT(item), "activate",
                          G_CALLBACK(move_window_workspace), mdata,
                          free_move_data, G_CONNECT_DEFAULT);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_ws_item = item = gtk_menu_item_new_with_mnemonic(_("Move to Another _Workspace"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->move_ws_submenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu->priv->move_ws_submenu);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    menu->priv->close_item = item = gtk_menu_item_new_with_mnemonic(_("_Close"));
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(close_window), window);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(GTK_WIDGET(menu));
    gtk_widget_hide(GTK_WIDGET(menu));

    g_signal_connect(menu->priv->window, "state-changed", G_CALLBACK(window_state_changed), menu);
    g_signal_connect(menu->priv->window, "capabilities-changed", G_CALLBACK(window_capabilities_changed), menu);
    g_signal_connect(menu->priv->window, "workspace-changed", G_CALLBACK(window_workspace_changed), menu);

    update_menu_items(menu);
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
    g_signal_handlers_disconnect_by_func(menu->priv->window, window_state_changed, menu);
    g_signal_handlers_disconnect_by_func(menu->priv->window, window_capabilities_changed, menu);
    g_signal_handlers_disconnect_by_func(menu->priv->window, window_workspace_changed, menu);
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
    XfwWorkspace *new_workspace = NULL;

    if (data->to.direction != XFW_DIRECTION_UP
        && data->to.direction != XFW_DIRECTION_DOWN
        && data->to.direction != XFW_DIRECTION_LEFT
        && data->to.direction != XFW_DIRECTION_RIGHT)
    {
        new_workspace = data->to.new_workspace;
    } else {
        XfwWorkspace *workspace = xfw_window_get_workspace(data->window);
        if (workspace != NULL) {
            new_workspace = xfw_workspace_get_neighbor(workspace, data->to.direction);
        }
    }

    if (new_workspace != NULL) {
        xfw_window_set_pinned(data->window, FALSE, NULL);
        xfw_window_move_to_workspace(data->window, new_workspace, NULL);
    }
}

static void
close_window(GtkWidget *item, XfwWindow *window) {
    xfw_window_close(window, gtk_get_current_event_time(), NULL);
}

static void
free_move_data(gpointer data, GClosure *closure) {
    XfwWindowWorkspaceMoveData *mdata = data;
    if (mdata->to.direction != XFW_DIRECTION_UP
        && mdata->to.direction != XFW_DIRECTION_DOWN
        && mdata->to.direction != XFW_DIRECTION_LEFT
        && mdata->to.direction != XFW_DIRECTION_RIGHT)
    {
        g_object_unref(mdata->to.new_workspace);
    }
    g_free(mdata);
}

static void
set_item_mnemonic(GtkWidget *item, const gchar *text) {
    GtkLabel *label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(item)));
    gtk_label_set_text_with_mnemonic(label, text);
    gtk_label_set_use_underline(label, TRUE);
}

static void
update_move_submenu(XfwWindowActionMenu *menu) {
    GtkWidget *submenu = menu->priv->move_ws_submenu, *item;
    GList *children;
    XfwWindowCapabilities caps = xfw_window_get_capabilities(menu->priv->window);
    XfwWorkspace *workspace = xfw_window_get_workspace(menu->priv->window);
    XfwWorkspaceGroup *group;

    children = gtk_container_get_children(GTK_CONTAINER(submenu));
    for (GList *l = children; l != NULL; l = l->next) {
        gtk_container_remove(GTK_CONTAINER(submenu), GTK_WIDGET(l->data));
    }
    g_list_free(children);

    if (workspace != NULL) {
        group = xfw_workspace_get_workspace_group(workspace);
    } else {
        // FIXME: assumes a single group, might be something better we can do here
        XfwWorkspaceManager *manager = xfw_screen_get_workspace_manager(xfw_window_get_screen(menu->priv->window));
        group = XFW_WORKSPACE_GROUP(xfw_workspace_manager_list_workspace_groups(manager)->data);
    }

    if ((caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0 && xfw_workspace_group_get_workspace_count(group) > 1) {
        for (GList *l = xfw_workspace_group_list_workspaces(group);
             l != NULL;
             l = l->next)
        {
            XfwWorkspace *other_workspace = XFW_WORKSPACE(l->data);
            gchar *label, *free_label = NULL;
            XfwWindowWorkspaceMoveData *mdata = g_new0(XfwWindowWorkspaceMoveData, 1);
            mdata->window = menu->priv->window;
            mdata->to.new_workspace = g_object_ref(other_workspace);

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
                                  G_CALLBACK(move_window_workspace), mdata,
                                  free_move_data, G_CONNECT_DEFAULT);
            gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        }

        gtk_widget_show(menu->priv->move_ws_item);
        gtk_widget_show_all(menu->priv->move_ws_submenu);
    } else {
        gtk_widget_hide(menu->priv->move_ws_item);
    }
}

static void
update_menu_items(XfwWindowActionMenu *menu) {
    XfwWindowState state = xfw_window_get_state(menu->priv->window);
    XfwWindowCapabilities caps = xfw_window_get_capabilities(menu->priv->window);
    XfwWorkspace *workspace = xfw_window_get_workspace(menu->priv->window);

    set_item_mnemonic(menu->priv->min_item, (state & XFW_WINDOW_STATE_MINIMIZED) == 0 ? _("Mi_nimize") : _("Unmi_nimize"));
    gtk_widget_set_sensitive(menu->priv->min_item,
                             ((state & XFW_WINDOW_STATE_MINIMIZED) == 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_MINIMIZE))
                             || ((state & XFW_WINDOW_STATE_MINIMIZED) != 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_UNMINIMIZE)));

    set_item_mnemonic(menu->priv->max_item, (state & XFW_WINDOW_STATE_MAXIMIZED) == 0 ? _("Ma_ximize") : _("Unma_ximize"));
    gtk_widget_set_sensitive(menu->priv->max_item,
                             ((state & XFW_WINDOW_STATE_MAXIMIZED) == 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_MAXIMIZE))
                             || ((state & XFW_WINDOW_STATE_MAXIMIZED) != 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_UNMAXIMIZE)));

    gtk_widget_set_sensitive(menu->priv->move_item, (caps & XFW_WINDOW_CAPABILITIES_CAN_MOVE) != 0);
    gtk_widget_set_sensitive(menu->priv->resize_item, (caps & XFW_WINDOW_CAPABILITIES_CAN_RESIZE) != 0);

    g_signal_handlers_block_by_func(menu->priv->above_item, G_CALLBACK(toggle_above_state), menu->priv->window);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu->priv->above_item), (state & XFW_WINDOW_STATE_ABOVE) != 0);
    gtk_widget_set_sensitive(menu->priv->above_item,
                             ((state & XFW_WINDOW_STATE_ABOVE) == 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_PLACE_ABOVE))
                             || ((state & XFW_WINDOW_STATE_ABOVE) != 0 && (caps & XFW_WINDOW_CAPABILITIES_CAN_UNPLACE_ABOVE)));
    g_signal_handlers_unblock_by_func(menu->priv->above_item, G_CALLBACK(toggle_above_state), menu->priv->window);

    g_signal_handlers_block_by_func(menu->priv->pin_item, G_CALLBACK(toggle_pinned_state), menu->priv->window);
    g_signal_handlers_block_by_func(menu->priv->unpin_item, G_CALLBACK(toggle_pinned_state), menu->priv->window);
    if ((state & XFW_WINDOW_STATE_PINNED) != 0) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu->priv->pin_item), TRUE);
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu->priv->unpin_item), TRUE);
    }
    gtk_widget_set_sensitive(menu->priv->pin_item, (caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0);
    gtk_widget_set_sensitive(menu->priv->unpin_item, (caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0);
    g_signal_handlers_unblock_by_func(menu->priv->pin_item, G_CALLBACK(toggle_pinned_state), menu->priv->window);
    g_signal_handlers_unblock_by_func(menu->priv->unpin_item, G_CALLBACK(toggle_pinned_state), menu->priv->window);

    if ((caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0 && workspace != NULL && xfw_workspace_get_neighbor(workspace, XFW_DIRECTION_LEFT) != NULL) {
        gtk_widget_show(menu->priv->move_left_item);
    } else {
        gtk_widget_hide(menu->priv->move_left_item);
    }
    if ((caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0 && workspace != NULL && xfw_workspace_get_neighbor(workspace, XFW_DIRECTION_RIGHT) != NULL) {
        gtk_widget_show(menu->priv->move_right_item);
    } else {
        gtk_widget_hide(menu->priv->move_right_item);
    }
    if ((caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0 && workspace != NULL && xfw_workspace_get_neighbor(workspace, XFW_DIRECTION_UP) != NULL) {
        gtk_widget_show(menu->priv->move_up_item);
    } else {
        gtk_widget_hide(menu->priv->move_up_item);
    }
    if ((caps & XFW_WINDOW_CAPABILITIES_CAN_CHANGE_WORKSPACE) != 0 && workspace != NULL && xfw_workspace_get_neighbor(workspace, XFW_DIRECTION_DOWN) != NULL) {
        gtk_widget_show(menu->priv->move_down_item);
    } else {
        gtk_widget_hide(menu->priv->move_down_item);
    }

    update_move_submenu(menu);
}

static void
window_state_changed(XfwWindow *window, XfwWindowState changed_mask, XfwWindowState new_state, XfwWindowActionMenu *menu) {
    update_menu_items(menu);
}

static void
window_capabilities_changed(XfwWindow *window, XfwWindowCapabilities changed_mask, XfwWindowCapabilities new_capabilities, XfwWindowActionMenu *menu) {
    update_menu_items(menu);
}

static void
window_workspace_changed(XfwWindow *window, XfwWindowActionMenu *menu) {
    update_menu_items(menu);
}

GtkWidget *
xfw_window_action_menu_new(XfwWindow *window) {
    return g_object_new(XFW_TYPE_WINDOW_ACTION_MENU,
                        "window", window,
                        NULL);
}
