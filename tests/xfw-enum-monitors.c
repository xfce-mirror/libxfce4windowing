#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

int
main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    XfwScreen *screen = xfw_screen_get_default();
    GList *monitors = xfw_screen_get_monitors(screen);
    for (GList *l = monitors; l != NULL; l = l->next) {
        XfwMonitor *monitor = XFW_MONITOR(l->data);

        GdkRectangle phys, log;
        xfw_monitor_get_physical_geometry(monitor, &phys);
        xfw_monitor_get_logical_geometry(monitor, &log);
        guint wmm, hmm;
        xfw_monitor_get_physical_size(monitor, &wmm, &hmm);

        g_print("Monitor: %s, ID=%s, serial=%s, scale=%d, size=%dx%dmm phys=%dx%d+%d+%d, log=%dx%d+%d+%d, gdkmonitor=%p\n",
                xfw_monitor_get_description(monitor),
                xfw_monitor_get_identifier(monitor),
                xfw_monitor_get_serial(monitor),
                xfw_monitor_get_scale(monitor),
                wmm, hmm,
                phys.width, phys.height, phys.x, phys.y,
                log.width, log.height, log.x, log.y,
                xfw_monitor_get_gdk_monitor(monitor));
    }
    g_object_unref(screen);

    return 0;
}
