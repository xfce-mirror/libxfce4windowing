#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

static void
print_window_monitors(XfwWindow *window) {
    g_print("    Monitors: [");
    GList *monitors = xfw_window_get_monitors(window);
    for (GList *lm = monitors; lm != NULL; lm = lm->next) {
        XfwMonitor *monitor = XFW_MONITOR(lm->data);
        if (lm != monitors) {
            g_print(", ");
        }
        g_print("%s", xfw_monitor_get_connector(monitor));
    }
    g_print("]\n");
}

static void
window_monitors_changed(XfwWindow *window) {
    g_print("Window '%s' monitors changed:\n", xfw_window_get_name(window));
    print_window_monitors(window);
}

static void
window_closed(XfwWindow *window) {
    g_print("Window closed: '%s'\n", xfw_window_get_name(window));
    g_signal_handlers_disconnect_by_data(window, NULL);
}

static void
window_opened(XfwScreen *screen, XfwWindow *window) {
    (void)screen;

    g_print("New window: %s\n", xfw_window_get_name(window));
    print_window_monitors(window);

    g_signal_connect(window, "notify::monitors", G_CALLBACK(window_monitors_changed), NULL);
    g_signal_connect(window, "closed", G_CALLBACK(window_closed), NULL);
}

int
main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    XfwScreen *screen = xfw_screen_get_default();
    g_signal_connect(screen, "window-opened", G_CALLBACK(window_opened), NULL);

    g_print("Windows on startup:\n");
    GList *windows = xfw_screen_get_windows(screen);
    for (GList *lw = windows; lw != NULL; lw = lw->next) {
        XfwWindow *window = XFW_WINDOW(lw->data);
        window_opened(screen, window);
    }
    g_print("(end of windows on startup)\n");

    gtk_main();

    return 0;
}
