/*
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

#ifndef __XFW_WORKAREA_X11_H__
#define __XFW_WORKAREA_X11_H__

#include <gdk/gdk.h>

/* Values have already been checked for CARDINAL/32 format by the X11 reader. */
gboolean _xfw_workarea_x11_parse_strut(const gulong *values, gsize nvalues,
                                       gint root_width, gint root_height,
                                       GdkRectangle edges[4]);
void _xfw_workarea_x11_apply(const GdkRectangle *geometry,
                             const GdkRectangle *edges, gsize nedges, GdkRectangle *workarea);
void _xfw_workarea_x11_to_logical(const GdkRectangle *geometry,
                                  const GdkRectangle *logical_geometry,
                                  gint scale, GdkRectangle *workarea);
gboolean _xfw_workarea_x11_client_applies(gboolean minimized, gboolean pinned,
                                          gint workspace, gint current_workspace,
                                          gboolean visible, const GdkRectangle edges[4]);
/* Only an absent PARTIAL property permits legacy fallback. */
gboolean _xfw_workarea_x11_select_strut(gboolean partial_present,
                                        const gulong *partial, gsize npartial,
                                        const gulong *legacy, gsize nlegacy,
                                        gint root_width, gint root_height, GdkRectangle edges[4]);

void _xfw_workarea_x11_fallback(const GdkRectangle *geometry,
                                const GdkRectangle *workareas, gsize nworkareas,
                                gint workspace, GdkRectangle *workarea);

#endif
