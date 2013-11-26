/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>
#include <clogger.h>
#include <libcork/core.h>
#include <libcork/ds.h>

#include "privateer.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Plugins
 */

pvt_declare_plugin(alpha);
pvt_declare_plugin(beta);
pvt_declare_plugin(gamma);

pvt_define_plugin(alpha)
{
}

pvt_define_plugin(beta)
{
    pvt_require_plugin(alpha);
}

pvt_define_plugin(gamma)
{
    pvt_require_plugin(beta);
}


pvt_declare_plugin(circular);

pvt_define_plugin(circular)
{
    pvt_require_plugin(gamma);
    pvt_require_plugin(circular);
}


/*-----------------------------------------------------------------------
 * Loading plugins
 */

#define load_plugin(ctx, name) \
    fail_if_error(pvt_ctx_add_static_plugin(ctx, pvt_static_plugin(name)));

static void
verify_plugin(struct pvt_ctx *ctx, const char *plugin_name, bool loaded)
{
    const struct pvt_plugin  *plugin;
    fail_if_error(plugin = pvt_ctx_get_plugin(ctx, plugin_name));
    fail_unless(pvt_plugin_loaded(plugin) == loaded,
                "Plugin %s should%s be loaded",
                plugin_name, loaded? "": " not");
}

START_TEST(test_load_alpha)
{
    struct pvt_ctx  *ctx;
    DESCRIBE_TEST;
    ctx = pvt_ctx_new();
    load_plugin(ctx, alpha);
    fail_if_error(pvt_ctx_load_plugins(ctx));
    verify_plugin(ctx, "alpha", true);
    pvt_ctx_free(ctx);
}
END_TEST

START_TEST(test_load_beta)
{
    struct pvt_ctx  *ctx;
    DESCRIBE_TEST;
    ctx = pvt_ctx_new();
    load_plugin(ctx, beta);
    fail_if_error(pvt_ctx_load_plugins(ctx));
    verify_plugin(ctx, "alpha", true);
    verify_plugin(ctx, "beta", true);
    pvt_ctx_free(ctx);
}
END_TEST

START_TEST(test_load_gamma)
{
    struct pvt_ctx  *ctx;
    DESCRIBE_TEST;
    ctx = pvt_ctx_new();
    load_plugin(ctx, gamma);
    fail_if_error(pvt_ctx_load_plugins(ctx));
    verify_plugin(ctx, "alpha", true);
    verify_plugin(ctx, "beta", true);
    verify_plugin(ctx, "gamma", true);
    pvt_ctx_free(ctx);
}
END_TEST

START_TEST(test_load_circular)
{
    struct pvt_ctx  *ctx;
    DESCRIBE_TEST;
    ctx = pvt_ctx_new();
    load_plugin(ctx, circular);
    fail_unless_error(pvt_ctx_load_plugins(ctx));
    verify_plugin(ctx, "circular", false);
    pvt_ctx_free(ctx);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("static-plugins");

    TCase  *tc_load = tcase_create("load");
    tcase_add_test(tc_load, test_load_alpha);
    tcase_add_test(tc_load, test_load_beta);
    tcase_add_test(tc_load, test_load_gamma);
    tcase_add_test(tc_load, test_load_circular);
    suite_add_tcase(s, tc_load);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    initialize_tests();
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    finalize_tests();

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
