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

#ifndef __XFW_MONITOR_WAYLAND_H__
#define __XFW_MONITOR_WAYLAND_H__

#include "xfw-monitor-private.h"
#include "xfw-screen-wayland.h"

G_BEGIN_DECLS

#define XFW_TYPE_MONITOR_WAYLAND (xfw_monitor_wayland_get_type())
G_DECLARE_FINAL_TYPE(XfwMonitorWayland, xfw_monitor_wayland, XFW, MONITOR_WAYLAND, XfwMonitor)

void _xfw_monitor_wayland_init(XfwScreenWayland *screen);

G_END_DECLS

#endif /* __XFW_MONITOR_WAYLAND_H__ */
