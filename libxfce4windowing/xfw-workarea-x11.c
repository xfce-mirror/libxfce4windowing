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

#include "xfw-workarea-x11.h"

/* EWMH coordinates are unsigned root pixels; range ends are inclusive. */
gboolean
_xfw_workarea_x11_parse_strut(const gulong *values, gsize nvalues,
                              gint root_width, gint root_height,
                              GdkRectangle edges[4]) {
    if (values == NULL || (nvalues != 4 && nvalues != 12)
        || root_width <= 0 || root_height <= 0) {
        return FALSE;
    }

    for (guint i = 0; i < 4; ++i) {
        gint depth_limit = i < 2 ? root_width : root_height;
        gint range_limit = i < 2 ? root_height : root_width;
        gulong start = nvalues == 12 ? values[4 + 2 * i] : 0;
        gulong end = nvalues == 12 ? values[5 + 2 * i] : (gulong)range_limit - 1;
        if (values[i] > (gulong)depth_limit
            || (values[i] != 0 && (start > end || end >= (gulong)range_limit))) {
            return FALSE;
        }
    }

    for (guint i = 0; i < 4; ++i) {
        gint start = nvalues == 12 && values[i] != 0 ? values[4 + 2 * i] : 0;
        gint end = nvalues == 12 && values[i] != 0 ? (gint)values[5 + 2 * i]
                                                   : (i < 2 ? root_height : root_width) - 1;
        gint depth = values[i];
        if (i < 2) {
            edges[i] = (GdkRectangle){ i == 0 ? 0 : root_width - depth, start, depth, end - start + 1 };
        } else {
            edges[i] = (GdkRectangle){ start, i == 2 ? 0 : root_height - depth, end - start + 1, depth };
        }
    }
    return TRUE;
}

void
_xfw_workarea_x11_apply(const GdkRectangle *geometry,
                        const GdkRectangle *edges, gsize nedges, GdkRectangle *workarea) {
    *workarea = *geometry;
    gint left = workarea->x;
    gint top = workarea->y;
    gint right = workarea->x + workarea->width;
    gint bottom = workarea->y + workarea->height;
    for (gsize i = 0; i < nedges; ++i) {
        GdkRectangle intersection;
        /* Always intersect the original monitor, never the accumulated workarea. */
        if (gdk_rectangle_intersect(geometry, &edges[i], &intersection)) {
            switch (i % 4) {
                case 0:
                    left = MAX(left, intersection.x + intersection.width);
                    break;
                case 1:
                    right = MIN(right, intersection.x);
                    break;
                case 2:
                    top = MAX(top, intersection.y + intersection.height);
                    break;
                case 3:
                    bottom = MIN(bottom, intersection.y);
                    break;
            }
        }
    }
    *workarea = (GdkRectangle){ left, top, MAX(0, right - left), MAX(0, bottom - top) };
}

static gint
scale_margin(gint margin, gint scale) {
    return margin / scale + (margin % scale != 0);
}

void
_xfw_workarea_x11_to_logical(const GdkRectangle *geometry,
                             const GdkRectangle *logical_geometry,
                             gint scale, GdkRectangle *workarea) {
    /* Round reserved margins outward, keeping an unreserved monitor identical
     * to its logical geometry even when its physical edges are not divisible. */
    gint left = MIN(logical_geometry->width, scale_margin(workarea->x - geometry->x, scale));
    gint top = MIN(logical_geometry->height, scale_margin(workarea->y - geometry->y, scale));
    gint right = scale_margin(geometry->x + geometry->width - workarea->x - workarea->width, scale);
    gint bottom = scale_margin(geometry->y + geometry->height - workarea->y - workarea->height, scale);
    *workarea = (GdkRectangle){ logical_geometry->x + left, logical_geometry->y + top,
                                MAX(0, logical_geometry->width - left - right),
                                MAX(0, logical_geometry->height - top - bottom) };
}

gboolean
_xfw_workarea_x11_client_applies(gboolean minimized, gboolean pinned,
                                 gint workspace, gint current_workspace,
                                 gboolean visible, const GdkRectangle edges[4]) {
    if (minimized || !visible || !(pinned || (workspace >= 0 && workspace == current_workspace))) {
        return FALSE;
    }
    for (guint i = 0; i < 4; ++i) {
        if (edges[i].width > 0 && edges[i].height > 0) {
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
_xfw_workarea_x11_select_strut(gboolean partial_present,
                               const gulong *partial, gsize npartial,
                               const gulong *legacy, gsize nlegacy,
                               gint root_width, gint root_height, GdkRectangle edges[4]) {
    if (partial_present) {
        return npartial == 12 && _xfw_workarea_x11_parse_strut(partial, npartial, root_width, root_height, edges);
    }
    return nlegacy == 4 && _xfw_workarea_x11_parse_strut(legacy, nlegacy, root_width, root_height, edges);
}

void
_xfw_workarea_x11_fallback(const GdkRectangle *geometry,
                           const GdkRectangle *workareas, gsize nworkareas,
                           gint workspace, GdkRectangle *workarea) {
    *workarea = *geometry;
    if (workareas != NULL && workspace >= 0 && (gsize)workspace < nworkareas) {
        GdkRectangle intersection;
        if (gdk_rectangle_intersect(geometry, &workareas[workspace], &intersection)) {
            *workarea = intersection;
        }
    }
}
