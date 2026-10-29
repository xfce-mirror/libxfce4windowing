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

/* Tests must keep GLib assertions enabled in release builds. */
#undef G_DISABLE_ASSERT

#include "libxfce4windowing/xfw-workarea-x11.h"

static void
assert_rect(GdkRectangle actual, gint x, gint y, gint width, gint height) {
    g_assert_cmpint(actual.x, ==, x);
    g_assert_cmpint(actual.y, ==, y);
    g_assert_cmpint(actual.width, ==, width);
    g_assert_cmpint(actual.height, ==, height);
}

static void
panels(void) {
    GdkRectangle monitors[] = { { 0, 0, 1920, 1080 }, { 1920, 0, 1920, 1080 } };
    for (guint mask = 0; mask < 4; ++mask) {
        GdkRectangle edges[8], result;
        guint n = 0;
        for (guint i = 0; i < 2; ++i) {
            if (mask & (1 << i)) {
                gulong strut[12] = { 0, 0, 0, 37, 0, 0, 0, 0, 0, 0, i * 1920, i * 1920 + 1919 };
                g_assert_true(_xfw_workarea_x11_parse_strut(strut, 12, 3840, 1080, edges + n));
                n += 4;
            }
        }
        for (guint i = 0; i < 2; ++i) {
            _xfw_workarea_x11_apply(&monitors[i], edges, n, &result);
            assert_rect(result, i * 1920, 0, 1920, mask & (1 << i) ? 1043 : 1080);
        }
    }
}

static void
edges_and_offsets(void) {
    GdkRectangle edges[4], result;
    GdkRectangle geometry = { 100, 200, 800, 600 };
    gulong strut[12] = { 110, 120, 230, 240, 200, 799, 200, 799, 100, 899, 100, 899 };
    g_assert_true(_xfw_workarea_x11_parse_strut(strut, 12, 1000, 1000, edges));
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 110, 230, 770, 530);

    /* A monitor above another must not inherit its bottom reservation. */
    geometry = (GdkRectangle){ 0, 0, 1920, 1080 };
    gulong bottom[12] = { 0, 0, 0, 37, 0, 0, 0, 0, 0, 0, 0, 1919 };
    g_assert_true(_xfw_workarea_x11_parse_strut(bottom, 12, 3840, 1380, edges));
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 0, 0, 1920, 1080);
    geometry.y = 300;
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 0, 300, 1920, 1043);

    /* The single pixel at x=1920 belongs to the right monitor only. */
    bottom[10] = bottom[11] = 1920;
    g_assert_true(_xfw_workarea_x11_parse_strut(bottom, 12, 3840, 1380, edges));
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 0, 300, 1920, 1080);
    geometry.x = 1920;
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 1920, 300, 1920, 1043);
}

static void
parsing(void) {
    GdkRectangle edges[4], result, geometry = { 1920, 0, 1920, 1080 };
    gulong values[12] = { 0, 0, 0, 37 };
    g_assert_true(_xfw_workarea_x11_parse_strut(values, 4, 3840, 1080, edges));
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 1920, 0, 1920, 1043);
    g_assert_false(_xfw_workarea_x11_parse_strut(NULL, 0, 3840, 1080, edges));
    g_assert_false(_xfw_workarea_x11_parse_strut(values, 11, 3840, 1080, edges));
    g_assert_false(_xfw_workarea_x11_parse_strut(values, 13, 3840, 1080, edges));
    values[10] = 20;
    values[11] = 19;
    g_assert_false(_xfw_workarea_x11_parse_strut(values, 12, 3840, 1080, edges));
    /* The legacy parser independently accepts four depths. */
    g_assert_true(_xfw_workarea_x11_parse_strut(values, 4, 3840, 1080, edges));
    values[10] = 0;
    values[11] = G_MAXUINT32;
    g_assert_false(_xfw_workarea_x11_parse_strut(values, 12, 3840, 1080, edges));
    values[3] = G_MAXUINT32;
    g_assert_false(_xfw_workarea_x11_parse_strut(values, 4, 3840, 1080, edges));
    values[3] = 0;
    g_assert_true(_xfw_workarea_x11_parse_strut(values, 12, 3840, 1080, edges));
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 1920, 0, 1920, 1080);
}

static void
order_and_removal(void) {
    GdkRectangle geometry = { 0, 0, 100, 100 }, result, reverse_result;
    GdkRectangle edges[12], reversed[12];
    gulong a[12] = { 40, 0, 0, 0, 0, 99 };
    gulong b[12] = { 0, 0, 30, 0, 0, 0, 0, 0, 0, 20 };
    gulong c[12] = { 50, 0, 0, 0, 0, 99 };
    g_assert_true(_xfw_workarea_x11_parse_strut(a, 12, 100, 100, edges));
    g_assert_true(_xfw_workarea_x11_parse_strut(b, 12, 100, 100, edges + 4));
    g_assert_true(_xfw_workarea_x11_parse_strut(c, 12, 100, 100, edges + 8));
    for (guint i = 0; i < 3; ++i) {
        memcpy(reversed + 4 * i, edges + 4 * (2 - i), 4 * sizeof(GdkRectangle));
    }
    _xfw_workarea_x11_apply(&geometry, edges, 12, &result);
    _xfw_workarea_x11_apply(&geometry, reversed, 12, &reverse_result);
    assert_rect(result, 50, 30, 50, 70);
    g_assert_true(gdk_rectangle_equal(&result, &reverse_result));
    _xfw_workarea_x11_apply(&geometry, NULL, 0, &result);
    assert_rect(result, 0, 0, 100, 100);
    gulong full[4] = { 100, 100, 100, 100 };
    g_assert_true(_xfw_workarea_x11_parse_strut(full, 4, 100, 100, edges));
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 100, 100, 0, 0);
}

static void
scaling(void) {
    GdkRectangle geometry = { 101, 201, 801, 601 };
    GdkRectangle logical = { 50, 100, 400, 300 }, result = geometry;
    _xfw_workarea_x11_to_logical(&geometry, &logical, 2, &result);
    assert_rect(result, 50, 100, 400, 300);
    result = (GdkRectangle){ 102, 202, 798, 598 };
    _xfw_workarea_x11_to_logical(&geometry, &logical, 2, &result);
    assert_rect(result, 51, 101, 398, 298);
    logical = (GdkRectangle){ 0, 0, 0, 0 };
    result = (GdkRectangle){ 102, 202, 798, 598 };
    _xfw_workarea_x11_to_logical(&geometry, &logical, G_MAXINT, &result);
    assert_rect(result, 0, 0, 0, 0);
}

static void
workspace_policy(void) {
    GdkRectangle edges[4], result;
    GdkRectangle geometry = { 1920, 0, 1920, 1080 };
    GdkRectangle global = { 0, 0, 3840, 1043 };
    gulong values[12] = { 0, 0, 0, 37, 0, 0, 0, 0, 0, 0, 0, 1919 };
    g_assert_true(_xfw_workarea_x11_parse_strut(values, 12, 3840, 1080, edges));
    g_assert_true(_xfw_workarea_x11_client_applies(FALSE, FALSE, 1, 1, TRUE, edges));
    g_assert_true(_xfw_workarea_x11_client_applies(FALSE, TRUE, -1, 0, TRUE, edges));
    g_assert_false(_xfw_workarea_x11_client_applies(FALSE, FALSE, -1, -1, TRUE, edges));
    /* A reservation on the other monitor still selects direct mode screen-wide. */
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 1920, 0, 1920, 1080);
    for (guint scenario = 0; scenario < 4; ++scenario) {
        gboolean minimized = scenario == 0;
        gint workspace = scenario == 1 ? 1 : 0;
        gboolean visible = scenario != 2;
        if (scenario == 3) {
            values[3] = 0;
            g_assert_true(_xfw_workarea_x11_parse_strut(values, 12, 3840, 1080, edges));
        }
        g_assert_false(_xfw_workarea_x11_client_applies(minimized, FALSE, workspace, 0, visible, edges));
        _xfw_workarea_x11_fallback(&geometry, &global, 1, 0, &result);
        assert_rect(result, 1920, 0, 1920, 1043);
    }
}

static void
property_precedence(void) {
    GdkRectangle edges[4];
    gulong partial[12] = { 0, 0, 0, 37, 0, 0, 0, 0, 0, 0, 0, 1919 };
    gulong legacy[4] = { 0, 0, 0, 50 };
    g_assert_true(_xfw_workarea_x11_select_strut(FALSE, NULL, 0, legacy, 4, 3840, 1080, edges));
    assert_rect(edges[3], 0, 1030, 3840, 50);
    g_assert_true(_xfw_workarea_x11_select_strut(TRUE, partial, 12, legacy, 4, 3840, 1080, edges));
    assert_rect(edges[3], 0, 1043, 1920, 37);
    /* Present but unreadable, wrongly sized, or invalid PARTIAL blocks legacy. */
    g_assert_false(_xfw_workarea_x11_select_strut(TRUE, NULL, 0, legacy, 4, 3840, 1080, edges));
    g_assert_false(_xfw_workarea_x11_select_strut(TRUE, partial, 4, legacy, 4, 3840, 1080, edges));
    g_assert_false(_xfw_workarea_x11_select_strut(TRUE, partial, 11, legacy, 4, 3840, 1080, edges));
    partial[11] = G_MAXUINT32;
    g_assert_false(_xfw_workarea_x11_select_strut(TRUE, partial, 12, legacy, 4, 3840, 1080, edges));
    partial[3] = 0;
    g_assert_true(_xfw_workarea_x11_select_strut(TRUE, partial, 12, legacy, 4, 3840, 1080, edges));
    g_assert_false(_xfw_workarea_x11_client_applies(FALSE, TRUE, -1, 0, TRUE, edges));
    /* Removing PARTIAL exposes legacy; removing both leaves no reservation. */
    g_assert_true(_xfw_workarea_x11_select_strut(FALSE, NULL, 0, legacy, 4, 3840, 1080, edges));
    g_assert_false(_xfw_workarea_x11_select_strut(FALSE, NULL, 0, NULL, 0, 3840, 1080, edges));
}

static void
custom_layouts(void) {
    GdkRectangle edges[4], result;
    gulong strut[12] = { 0, 0, 0, 37, 0, 0, 0, 0, 0, 0, 0, 1919 };
    g_assert_true(_xfw_workarea_x11_parse_strut(strut, 12, 3840, 1080, edges));
    GdkRectangle geometry = { -1920, 0, 1920, 1080 };
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, -1920, 0, 1920, 1080);
    geometry.x = -100;
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, -100, 0, 1920, 1043);
    GdkRectangle logical = { -50, 0, 960, 540 };
    _xfw_workarea_x11_to_logical(&geometry, &logical, 2, &result);
    assert_rect(result, -50, 0, 960, 521);
    /* Overlapping monitors each receive the actual intersection. */
    geometry.x = 0;
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 0, 0, 1920, 1043);
    geometry.x = 960;
    _xfw_workarea_x11_apply(&geometry, edges, 4, &result);
    assert_rect(result, 960, 0, 1920, 1043);
}

static void
wm_properties(void) {
    GdkRectangle geometry = { 1920, 0, 1920, 1080 }, result;
    GdkRectangle global = { 0, 0, 3840, 1043 };
    _xfw_workarea_x11_fallback(&geometry, &global, 1, 0, &result);
    assert_rect(result, 1920, 0, 1920, 1043);
    _xfw_workarea_x11_fallback(&geometry, &global, 1, -1, &result);
    assert_rect(result, 1920, 0, 1920, 1080);
    _xfw_workarea_x11_fallback(&geometry, &global, 1, 1, &result);
    assert_rect(result, 1920, 0, 1920, 1080);
    _xfw_workarea_x11_fallback(&geometry, NULL, 0, 0, &result);
    assert_rect(result, 1920, 0, 1920, 1080);
    global = (GdkRectangle){ 0, 0, 100, 100 };
    _xfw_workarea_x11_fallback(&geometry, &global, 1, 0, &result);
    assert_rect(result, 1920, 0, 1920, 1080);
}

int
main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/workarea/panels", panels);
    g_test_add_func("/workarea/edges-offsets", edges_and_offsets);
    g_test_add_func("/workarea/parsing", parsing);
    g_test_add_func("/workarea/order-removal", order_and_removal);
    g_test_add_func("/workarea/scaling", scaling);
    g_test_add_func("/workarea/workspaces", workspace_policy);
    g_test_add_func("/workarea/wm-properties", wm_properties);
    g_test_add_func("/workarea/property-precedence", property_precedence);
    g_test_add_func("/workarea/custom-layouts", custom_layouts);
    return g_test_run();
}
