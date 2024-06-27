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

#ifndef __XFW_MONITOR_PRIVATE_H__
#define __XFW_MONITOR_PRIVATE_H__

#include "xfw-monitor.h"

G_BEGIN_DECLS

struct _XfwMonitorClass {
    GObjectClass parent_class;
};

void _xfw_monitor_set_identifier(XfwMonitor *monitor,
                                 const char *identifier);
void _xfw_monitor_set_description(XfwMonitor *monitor,
                                  const char *description);
void _xfw_monitor_set_connector(XfwMonitor *monitor,
                                const char *connector);
void _xfw_monitor_set_make(XfwMonitor *monitor,
                           const char *make);
void _xfw_monitor_set_model(XfwMonitor *monitor,
                            const char *model);
void _xfw_monitor_set_serial(XfwMonitor *monitor,
                             const char *serial);
void _xfw_monitor_set_refresh(XfwMonitor *monitor,
                              guint refresh_millihertz);
void _xfw_monitor_set_scale(XfwMonitor *monitor,
                            guint scale);
void _xfw_monitor_set_physical_geometry(XfwMonitor *monitor,
                                        GdkRectangle *physical_geometry);
void _xfw_monitor_set_logical_geometry(XfwMonitor *monitor,
                                       GdkRectangle *logical_geometry);
void _xfw_monitor_set_physical_size(XfwMonitor *monitor,
                                    guint width_mm,
                                    guint height_mm);
void _xfw_monitor_set_subpixel(XfwMonitor *monitor,
                               XfwMonitorSubpixel subpixel);
void _xfw_monitor_set_transform(XfwMonitor *monitor,
                                XfwMonitorTransform transform);

G_END_DECLS

#endif /* __XFW_MONITOR_PRIVATE_H__ */
